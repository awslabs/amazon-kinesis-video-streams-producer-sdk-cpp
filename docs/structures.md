### Producer SDK layers

Producer SDK is architected in layers where each layer is designed for a specific integration scenario. The lower layers provide most flexibility and portability whereas higher layers provide for easier integration. 
The core or PIC (Platform Independent Code) contains the main business logic which is agnostic of the networking and the media pipeline. Its primary responsibility is to abstract the buffering, state machine logic, etc.. for diverse set of platforms compilers. It provides low-level platform integration via default macros - such as MEMALLOC which can be overwritten for a given platform/scenario. The higher-level integration is achieved via callbacks (e.g. networking calls, auth, etc...) which are covered below. 

Main platform-level abstraction happens via macros which are defined in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/common/include/com/amazonaws/kinesis/video/common/CommonDefs.h and either mapped directly or via global function pointers which are defined per platform in Utils sub-project.

A layer above PIC is the Producer interface. This layer (C and C++ Producer SDK) provides networking support and authentication capabilities to integrate with the AWS Kinesis Video Streams service. 

These abstraction also allow for independent scripted testing for networking conditions and media pipelines in isolation.

JNI layer allows the integration with Java SDK and expose Java Producer interface on top of which MediaSoure interface is built.

The SDK also has some integration with "known" media pipelines in a form of GStreamer kvssink plugin, which takes care of the integration with the actual frames, similar to Android/Java MediaSource interface.


+=======================================+
|   GStreamer   |   Android/Java client |   <- MediaSource interfaces
+=======================================+
|   C++ SDK     |                       |
+================    JNI/Java adapter   +   <- Producer interfaces
|   C SDK       |                       | 
+=======================================+
|              PIC                      |   <- Core logic
+=======================================+

Many real-life devices and applications have their own custom media pipeline which can be integrated with the KVS Producer interface with relative ease, needing codec configuration and frame data from the media pipeline. SDK provides capabilities of extracting or generating the CPD (Codec Private Data) for some media types listed below.

The structures described below have their equivalents in different layers of the SDK but semantically they are the same across the layers as they get converted to the ones defined in PIC.


### High-level object abstractions: Client and Streams

KVS Producer SDKs have a single main object called "Client". This object represents the streaming device and is abstracted by a handle: https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L458. 
Each client can have one or more Stream objects. Stream object represents the actual media stream the Client object will create, configure and start streaming. Stream object is abstracted by a stream handle: https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L478

Public APIs operating on the client or the streams will take these handles that are returned upon successfull "Create" call.

During Streaming, each stream can have multiple logical streaming sessions, representing a single PutMedia call. The streaming sessions are created on token rotation or re-streaming case caused by an error or re-connect triggered by the application or higher-level logic in Continuous Retry Callback provider (default) as a response to latency pressures and when connection staleness is detected.


### Configuration and defaults

#### DeviceInfo

DeviceInfo structure applies to the Client object and is primarily responsible for the content store configuration (StorageInfo structure) and some other client-level behaviors (ClientInfo structure) like timeouts, logging verbosity, etc. https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1063

C Producer has a helper APIs defined in https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h

* createDefaultDeviceInfo - creates a DeviceInfo object filled with defaults. The API will set in-memory storage type with 128MB allocation. It will also set default logging level from the environment variable if defined or WARN level if not. The default timeouts will be applied.
* setDeviceInfoStorageSize - sets the storage size in the structure to the value specified by the application. Used when the application needs to override the default value (for example, based on the availability).
* setDeviceInfoStorageSizeBasedOnBitrateAndBufferDuration - sets the storage size based on average bitrage and the max duration to store when buffering. 

C++ Producer has a set of defaults which are slightly different: https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/blob/master/src/DefaultDeviceInfoProvider.cpp#L25

Applications can choose to specify the maximal amount of memory they can allow the content store to grow to or optimize the memory per the max duration they will require. NOTE: the content store is shared among all of the streams and it should accommodate frames from these streams in case of a network pressure event. 

DeviceInfo is supplied to createKinesisVideoClient API which will be used to create and configure the Client object.


#### StreamInfo

StreamInfo structure https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L979 is supplied to createKinesisVideoStream API and contains information needed to create and configure a particular stream. This includes items needed for AWS integration such as the stream name, AWS Tags to be added/updated, AWS KMS key (if not using default key), retention period and stream specific information in SttreamCaps https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L880.
StreamCaps is responsible for supplying configuration information such as the streaming type, content type, various time duration, track-level information and policies. The comments in the structure field declarations contain detailed information.

C Producer has a set of helper APIs to create and configure a StreamInfo object for different cases:
* createRealtimeVideoStreamInfoProvider - creates and configures StreamInfo object for a realtime streaming scenario
* createOfflineVideoStreamInfoProvider - creates and configures StreamInfo object for an offline streaming scenario
* createRealtimeAudioVideoStreamInfoProvider - creates and configures StreamInfo for a realtime streaming of multi-track audio and video
* createOfflineAudioVideoStreamInfoProvider - creates and configures StreamInfo for an offline streaming of multi-track audio and video
* setStreamInfoBasedOnStorageSize - helper API to set the stream info buffer value based on the available storage size

The above APIs will create and configure the defaults but the application can choose to further configure and overwrite the values based on the application needs.


#### ClientCallbacks

ClientCallback structure defined in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1860 is supplied to createKinesisVideoClient API. It contains a single 64bit field custom data and a set of mandatory and optional callback functions which will be called by the core for various operations and notifications and supplied the specified custom data. The callback function prototypes contain comments describing what they are for, when they will be called and the descriptions of the parameters supplied.

The callback functionality can be split into 5 categories. Those are defined in the C producer layer as:

* PlatformCallbacks - contains platform level functinality, including thread, synchronization, time and logging functionality that can be overwritten: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h#L116
* ProducerCallbacks - contains producer client level functionality, including storage pressure callbacks: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h#L147
* StreamCallbacks - contains stream level functionality, including buffering pressure callbacks, runtime error callbacks, fragment ACKs, etc... These are used also by the C Producer Continuous Callback Provider (default callbacks) for re-streaming and default handling on staleness and pressures: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h#L168
* AuthCallbacks - contains security integration functionality. Some of the callbacks will not be executed due to not-yet-implemented self-provisioning logic. These are useful for IoT-based, File-based credentials, etc integration solution: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h#L198
* ApiCallbacks - contains the AWS public API integration solution. This is supplied by the default callback provider and can be also monitored and overwritten by the application, for example, for supplying different endpoints: https://github.com/awslabs/amazon-kinesis-video-streams-producer-c/blob/master/src/include/com/amazonaws/kinesis/video/cproducer/Include.h#L221

The C Producer has a set of APIs to create a default callback provider and then add specific providers in a chain that will be called sequentially one after the other to perform certain tasks. For example, the default StreamLatencyPressure callback supplied by the Continuous Retry logic can be expanded by the application by apprending it's own implementation in the processing chain. All callback structures with the exception of the PlatformCallbacks are chains and have Add API whereas PlatformCallbacks have Set API to replace a particular default implementation.


CreateCallbackProvider set of APIs return a pointer to newly created object that encapsulates ClientCallbacks structure which contains all of the callbacks flat that can be directly supplied to PIC createKinesisVideoClient API. 


#### Frame

Frame structure defined in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/mkvgen/include/com/amazonaws/kinesis/video/mkvgen/Include.h#L279 reoresents a single frame that the media pipeline produces into the Producer SDK using putKinesisVideoFrame API. It's an agstraction that encapsulates flags, timestamps, track ID and the frame data itself. Frame format is specific to the content type itself, only CPD extraction routine and the adaptation logic are aware of specifics of the frame format, the rest of the SDK treats the content of the frame data as a binary blob. 

Using Frame abstraction allows applications to represent frames from a variety of content types and not only audio/video frames. Frame flags represent either a key-frame or non-key-frame, representing I-frames or P-frames respectively which, from the perspective of the SDK are needed to drive the fragmentation.

NOTE: The frame data is copied upon the call of the putKinesisVideoFrame API so the media pipeline can dereference the frame bits.


















