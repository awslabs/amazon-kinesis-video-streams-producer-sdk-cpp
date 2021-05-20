/**
 * Helper functionality for CURL
 */
#define LOG_CLASS "CurlCall"
#include "../Include_i.h"

STATUS blockingLwsCall(PRequestInfo pRequestInfo, PCallInfo pCallInfo)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PCHAR pHostStart, pHostEnd;
    CHAR path[MAX_URI_CHAR_LEN + 1];
    struct lws_context* lwsContext = NULL;
    struct lws_context_creation_info creationInfo;
    struct lws_client_connect_info connectInfo;
    struct lws* clientLws = NULL;
    volatile INT32 retVal = 0;
    struct lws_protocols lwsProtocols[2];

    CHK(pRequestInfo != NULL && pCallInfo != NULL, STATUS_NULL_ARG);

    // Prepare the signaling channel protocols array
    MEMSET(lwsProtocols, 0x00, SIZEOF(lwsProtocols));
    lwsProtocols[0].name = HTTPS_SCHEME_NAME;
    lwsProtocols[0].callback = lwsIotCallbackRoutine;
    lwsProtocols[1].name = NULL;
    lwsProtocols[1].callback = NULL;

    // Prepare the LWS context
    MEMSET(&creationInfo, 0x00, SIZEOF(struct lws_context_creation_info));
    creationInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    creationInfo.port = CONTEXT_PORT_NO_LISTEN;
    creationInfo.protocols = lwsProtocols;
    creationInfo.timeout_secs = pRequestInfo->completionTimeout / HUNDREDS_OF_NANOS_IN_A_SECOND;
    creationInfo.gid = -1;
    creationInfo.uid = -1;
    creationInfo.fd_limit_per_thread = 1 + 1 + 1;
    creationInfo.client_ssl_ca_filepath = pRequestInfo->certPath;
    creationInfo.client_ssl_cert_filepath = pRequestInfo->sslCertPath;
    creationInfo.client_ssl_private_key_filepath = pRequestInfo->sslPrivateKeyPath;

    CHK(NULL != (lwsContext = lws_create_context(&creationInfo)), STATUS_IOT_CREATE_LWS_CONTEXT_FAILED);

    // Execute the LWS REST call
    MEMSET(&connectInfo, 0x00, SIZEOF(struct lws_client_connect_info));
    connectInfo.context = lwsContext;
    connectInfo.ssl_connection = LCCSCF_USE_SSL;
    connectInfo.port = DEFAULT_SSL_PORT_NUMBER;

    CHK_STATUS(getRequestHost(pRequestInfo->url, &pHostStart, &pHostEnd));

    // Store the path
    STRNCPY(path, pHostEnd, MAX_URI_CHAR_LEN);
    path[MAX_URI_CHAR_LEN] = '\0';

    // NULL terminate the host
    *pHostEnd = '\0';

    connectInfo.address = pHostStart;
    connectInfo.path = path;
    connectInfo.host = connectInfo.address;
    connectInfo.method = HTTP_REQUEST_VERB_GET_STRING;
    connectInfo.protocol = lwsProtocols[0].name;
    connectInfo.pwsi = &clientLws;

    connectInfo.opaque_user_data = (PVOID) pCallInfo;

    CHK(NULL != lws_client_connect_via_info(&connectInfo), STATUS_IOT_CREATE_LWS_CONTEXT_FAILED);

    while (retVal >= 0 && !ATOMIC_LOAD_BOOL(&pCallInfo->pRequestInfo->terminating)) {
        retVal = lws_service(lwsContext, 0);
    }

CleanUp:

    if (lwsContext != NULL) {
        // Trigger termination
        ATOMIC_STORE_BOOL(&pCallInfo->pRequestInfo->terminating, TRUE);

        // Cancel the ongoing service if any
        lws_cancel_service(lwsContext);

        // Destroy the context
        lws_context_destroy(lwsContext);
    }

    LEAVES();
    return retStatus;
}

INT32 lwsIotCallbackRoutine(struct lws* wsi, enum lws_callback_reasons reason, PVOID user, PVOID pDataIn, size_t dataSize)
{
    UNUSED_PARAM(user);
    STATUS retStatus = STATUS_SUCCESS;
    PVOID customData;
    INT32 status, size, retValue = 0;
    PCHAR pCurPtr, pBuffer;
    CHAR buffer[IOT_LWS_SEND_BUFFER_SIZE];
    PBYTE pEndPtr;
    PBYTE* ppStartPtr;
    PCallInfo pCallInfo;
    PRequestInfo pRequestInfo = NULL;
    PSingleListNode pCurNode;
    UINT64 item;
    UINT32 headerCount;
    PRequestHeader pRequestHeader;

    customData = lws_get_opaque_user_data(wsi);
    pCallInfo = (PCallInfo) customData;

    // Early return
    CHK(pCallInfo != NULL, retStatus);
    lws_set_log_level(LLL_NOTICE | LLL_INFO | LLL_WARN | LLL_ERR, NULL);

    pRequestInfo = pCallInfo->pRequestInfo;
    pBuffer = buffer + LWS_PRE;

    DLOGV("HTTPS callback with reason %d", reason);

    switch (reason) {
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            pCurPtr = pDataIn == NULL ? "(None)" : (PCHAR) pDataIn;
            DLOGW("Client connection failed. Connection error string: %s", pCurPtr);
            STRNCPY(pCallInfo->errorBuffer, pCurPtr, CALL_INFO_ERROR_BUFFER_LEN);
            ATOMIC_STORE_BOOL(&pRequestInfo->terminating, TRUE);
            lws_cancel_service(lws_get_context(wsi));

            break;

        case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
            DLOGD("Client http closed");
            ATOMIC_STORE_BOOL(&pRequestInfo->terminating, TRUE);
            lws_cancel_service(lws_get_context(wsi));

            break;

        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
            status = lws_http_client_http_response(wsi);
            DLOGD("Connected with server response: %d", status);
            pCallInfo->callResult = getServiceCallResultFromHttpStatus((UINT32) status);

            // Store the Request ID header
            if ((size = lws_hdr_custom_copy(wsi, pBuffer, SIZEOF(buffer), KVS_REQUEST_ID_HEADER_NAME ":",
                                            SIZEOF(KVS_REQUEST_ID_HEADER_NAME) * SIZEOF(CHAR))) > 0) {
                pBuffer[size] = '\0';
                DLOGI("Request ID: %s", pBuffer);
            }

            break;

        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
            DLOGD("Received client http read: %d bytes", (INT32) dataSize);
            lwsl_hexdump_debug(pDataIn, dataSize);

            if (dataSize != 0) {
                CHK(NULL != (pCallInfo->responseData = (PCHAR) MEMALLOC(dataSize)), STATUS_NOT_ENOUGH_MEMORY);
                MEMCPY(pCallInfo->responseData, pDataIn, dataSize);
                pCallInfo->responseDataLen = (UINT32) dataSize;
            }

            break;

        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
            DLOGD("Received client http");
            size = SIZEOF(buffer);
            if (lws_http_client_read(wsi, &pBuffer, &size) < 0) {
                retValue = -1;
            }

            break;

        case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
            DLOGD("Http client completed");
            ATOMIC_STORE_BOOL(&pRequestInfo->terminating, TRUE);
            lws_cancel_service(lws_get_context(wsi));

            break;

        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
            DLOGD("Client append handshake header\n");
            CHK_STATUS(singleListGetNodeCount(pRequestInfo->pRequestHeaders, &headerCount));
            ppStartPtr = (PBYTE*) pDataIn;
            pEndPtr = *ppStartPtr + dataSize - 1;

            // Iterate through the headers
            while (headerCount != 0) {
                CHK_STATUS(singleListGetHeadNode(pRequestInfo->pRequestHeaders, &pCurNode));
                CHK_STATUS(singleListGetNodeData(pCurNode, &item));

                pRequestHeader = (PRequestHeader) item;

                // Append the colon at the end of the name
                if (pRequestHeader->pName[pRequestHeader->nameLen - 1] != ':') {
                    STRCPY(pBuffer, pRequestHeader->pName);
                    pBuffer[pRequestHeader->nameLen] = ':';
                    pBuffer[pRequestHeader->nameLen + 1] = '\0';
                    pRequestHeader->pName = pBuffer;
                    pRequestHeader->nameLen++;
                }

                DLOGV("Appending header - %s %s", pRequestHeader->pName, pRequestHeader->pValue);

                status = lws_add_http_header_by_name(wsi, (PBYTE) pRequestHeader->pName, (PBYTE) pRequestHeader->pValue, pRequestHeader->valueLen,
                                                     ppStartPtr, pEndPtr);
                if (status != 0) {
                    retValue = 1;
                    CHK(FALSE, retStatus);
                }

                // Remove the head
                CHK_STATUS(singleListDeleteHead(pRequestInfo->pRequestHeaders));
                MEMFREE(pRequestHeader);

                // Decrement to iterate
                headerCount--;
            }

            lws_client_http_body_pending(wsi, 1);
            lws_callback_on_writable(wsi);

            break;

        case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
            DLOGD("Sending the body %.*s, size %d", pRequestInfo->bodySize, pRequestInfo->body, pRequestInfo->bodySize);
            MEMCPY(pBuffer, pRequestInfo->body, pRequestInfo->bodySize);

            size = lws_write(wsi, (PBYTE) pBuffer, (SIZE_T) pRequestInfo->bodySize, LWS_WRITE_TEXT);

            if (size != pRequestInfo->bodySize) {
                DLOGW("Failed to write out the body of POST request entirely. Expected to write %d, wrote %d", pRequestInfo->bodySize, size);
                if (size > 0) {
                    // Schedule again
                    lws_client_http_body_pending(wsi, 1);
                    lws_callback_on_writable(wsi);
                } else {
                    // Quit
                    retValue = 1;
                }
            } else {
                lws_client_http_body_pending(wsi, 0);
            }

            break;

        default:
            break;
    }

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        if (pRequestInfo != NULL) {
            ATOMIC_STORE_BOOL(&pRequestInfo->terminating, TRUE);
        }

        lws_cancel_service(lws_get_context(wsi));

        return -1;
    } else {
        return retValue;
    }
}