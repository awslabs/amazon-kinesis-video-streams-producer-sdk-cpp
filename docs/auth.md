### KVS SDK Auth integration

KVS SDK defines a generic Credential Provider object defined in https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/common/Include.h#L452 which is used by the core of the SDK to get the credentials whenever they are needed. The abstract AwsCredentialProvider object contains a single API called GetCredentialsFunc which is called by the core SDK to fetch/refresh the credentials.

The Credential Provider returns an AWS Credentials object defined in https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/common/Include.h#L296 which will be used for the backend call authentication. It contains fields for Access Key ID, Secret Key and an optional Session Token. The credentials object also contains UINT64 expiration field which is the absolute time in the 100ns in the Producer device system clock when the credentials will expire. The core of the SDK will refresh the credentials whenever they are needed to be refreshed for the backend API calls by calling GetCredentialsFunc on the Credential Provider object.

Kinesis Video Streams uses IAM policies/roles for access control as described in AWS documentation: https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/how-iam.html

KVS can integrate with a variety of authentication providers via the Credential Provider object. Currently, the SDK defines the following Credential Providers. An application can instantiate the appropriate provider by calling create<Name>CredentialProvider API and submit it to the SDK. The provider can be freed on application termination after the core of the SDK has been freed by calling the corresponding free<Name>CredentialProvider API.

#### Static credential provider

Static credential provider is used whenever the application needs to integrate with a "persistent" Access Key ID/Secret Key pair. It doesn't have any expiration and will vend out the same credentials with every call.

https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/common/Include.h#L500

#### IoT based credential provider

Most of the devices will likely integrate with IoT certificates model during the provisioning to avoid "baking" the static credentials into the device firmware. More information on how to generate the IoT certificate can be found in AWS documentation: https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/how-iot.html

Using IoT certificates for the authZ integration instead of storing AWS access key id and secret access key is the preferred way of supplying credentials. This is based on the security considerations as a loss of a single device with the credentials might proliferate the loss to the every other device using access key/secret key. Some integration scenarios use similar credential providers which do not hard-code the keys but have a mechanism of retrieving temporary credentials in a form of access key id, secret access key and the session token. 

Producer SDK defines a few constructor APIs for creating IoT based credentials provider: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/common/Include.h#L525

* createCurlIotCredentialProvider - IoT credential provider that makes backend calls using libCurl client
* createLwsIotCredentialProvider - IoT credential provider that makes backend calls using libWebSockets client

Same APIs but the ones below are taking in a function to retrieve the current time which is necessary for higher-layers of the SDK or when applications redefine the system clock functionality.

* createCurlIotCredentialProviderWithTime
* createLwsIotCredentialProviderWithTime

IoT credential provider makes a synchronous RESTful call to IoT endpoint in order to retrieve or refresh the credentials when needed. The first call will be made during the construction of the provider object which makes the construction of the object "non-prompt". The IoT credential provider implements a simple caching policy described below and returns the cached credentials whenever possible instead of actually making backend calls.

IoT credential provider object is freed calling a single freeIotCredentialProvider API.

By default, C, C++ Producers will compile against the Curl version as the streaming is based on libCurl and the WebRtc will compile against the LWS version as the Signaling in WebRtc is already based on libWebSockets.

#### File based credential provider

Some applications have a separate authentication module running in a different process. The auth module will periodically retrieve the new credentials (potentially integrating with IAM directly) but it's hard to share these credentials via an inter-process communication in a cross-platform fashion. For this, the SDK implements a file-based credential provider. The credentials are shared via a file which can be on a persistent media or a memory mapped file. The auth module will produce the up-to-date credentials into the file and the SDK file based credential provider will read from the file and use the credentials.

Just like the IoT provider case, the Producer SDK defines a few constructor APIs for creating file based credentials provider: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/common/Include.h#L602

* createFileCredentialProvider - File based credential provider that reads the credentials from a specified file which gets periodically updated by an authentication module.
* createFileCredentialProviderWithTime - Same API which also takes a time functionality.

Similar to IoT credential provider, the file based credentials provider implements a caching policy in order to not read the credential file with each call.

File based credential provider object is freed calling a single freeFileCredentialProvider API.

#### Credentials Caching policy

Credential providers like IoT provider perform "non-prompt" operation to fetch/refresh the credentials (i.e. the IoT credential provider will have to make a RESTful call to the IoT endpoint to retrieve those which is done in a synchronous manner). The low-level SDK core fetches the credentials multiple times and the credential provider will cache the retrieved credentials and submit the cached credentials instead of fetching the credentials for every callback call. 

The providers will force-refresh the credentials if there is less than 38 seconds remaining from the cached credentials expiration. This constant is defined as: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/source/Common/IotCredentialProvider.h#L25

#### AWS public certificate integration

AWS public certificate pem file is located at https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/certs/cert.pem

Applications should include this pem file in their public certificate store in order for the networking client (libCurl or libWebSockets) to establish trust as the networking clients are configured to validate the server certificates. Each platform has their own default certificate paths - ex: on Linux based OS-es it's in /etc/ssl/, however, the applications could also choose to install the pem file in other locations and specify the path in the public APIs.

C Producer: CA cert path can be specified in the CallbackProvider constructor APIs - ex: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h#L297

C++ Producer: By overriding getCertPath() virtual member function in the DeviceInfoProvider class https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/src/DeviceInfoProvider.h#L37 

The C++ CMakeList.txt file also specifies the path https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/CMakeLists.txt#L53

NOTE: The certificate path could point to the fully qualified path of the pem file or to the directory in which the pem files are located.
