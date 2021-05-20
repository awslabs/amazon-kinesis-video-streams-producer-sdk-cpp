/**
 * Kinesis Video Producer Static Credential Provider
 */
#define LOG_CLASS "StaticCredentialProvider"
#include "Include_i.h"

STATUS createStaticCredentialProvider(PCHAR accessKeyId, UINT32 accessKeyIdLen, PCHAR secretKey, UINT32 secretKeyLen, PCHAR sessionToken,
                                      UINT32 sessionTokenLen, UINT64 expiration, PAwsCredentialProvider* ppCredentialProvider)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PAwsCredentials pAwsCredentials = NULL;
    PStaticCredentialProvider pStaticCredentialProvider = NULL;

    CHK(ppCredentialProvider != NULL, STATUS_NULL_ARG);
    // Create the credentials object

    CHK_STATUS(
        createAwsCredentials(accessKeyId, accessKeyIdLen, secretKey, secretKeyLen, sessionToken, sessionTokenLen, expiration, &pAwsCredentials));

    pStaticCredentialProvider = (PStaticCredentialProvider) MEMCALLOC(1, SIZEOF(StaticCredentialProvider));
    CHK(pStaticCredentialProvider != NULL, STATUS_NOT_ENOUGH_MEMORY);

    pStaticCredentialProvider->pAwsCredentials = pAwsCredentials;
    pStaticCredentialProvider->credentialProvider.getCredentialsFn = getStaticCredentials;

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        freeStaticCredentialProvider((PAwsCredentialProvider*) &pStaticCredentialProvider);
        pStaticCredentialProvider = NULL;
    }

    // Set the return value if it's not NULL
    if (ppCredentialProvider != NULL) {
        *ppCredentialProvider = (PAwsCredentialProvider) pStaticCredentialProvider;
    }

    LEAVES();
    return retStatus;
}

STATUS freeStaticCredentialProvider(PAwsCredentialProvider* ppCredentialProvider)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PStaticCredentialProvider pStaticCredentialProvider = NULL;

    CHK(ppCredentialProvider != NULL, STATUS_NULL_ARG);

    pStaticCredentialProvider = (PStaticCredentialProvider) *ppCredentialProvider;

    // Call is idempotent
    CHK(pStaticCredentialProvider != NULL, retStatus);

    // Release the underlying AWS credentials object
    freeAwsCredentials(&pStaticCredentialProvider->pAwsCredentials);

    // Release the object
    MEMFREE(pStaticCredentialProvider);

    // Set the pointer to NULL
    *ppCredentialProvider = NULL;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS getStaticCredentials(PAwsCredentialProvider pCredentialProvider, PAwsCredentials* ppAwsCredentials)
{
    ENTERS();

    STATUS retStatus = STATUS_SUCCESS;

    PStaticCredentialProvider pStaticCredentialProvider = (PStaticCredentialProvider) pCredentialProvider;

    CHK(pStaticCredentialProvider != NULL && ppAwsCredentials != NULL, STATUS_NULL_ARG);

    *ppAwsCredentials = pStaticCredentialProvider->pAwsCredentials;

CleanUp:

    LEAVES();
    return retStatus;
}
