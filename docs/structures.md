### Producer SDK layers

Producer SDK is architected in layers where each layer is designed for a specific integration scenario. The lower layers provide most flexibility and portability whereas higher layers provide for easier integration. 
The core or PIC (Platform Independent Code) contains the main business logic which is agnostic of the networking and the media pipeline. Its primary responsibility is to abstract the buffering, state machine logic, etc.. for diverse set of platforms compilers. It provides low-level platform integration via default macros - such as MEMALLOC which can be overwritten for a given platform/scenario. The higher-level integration is achieved via callbacks (e.g. networking calls, auth, etc...) which are covered below. 

Main platform-level abstraction happens via macros which are defined in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/common/include/com/amazonaws/kinesis/video/common/CommonDefs.h and either mapped directly or via global function pointers which are defined per platform in Utils sub-project.

A layer above PIC is the Producer interface. This layer (C and C++ Producer SDK) provides networking support and authentication capabilities to integrate with the AWS Kinesis Video Streams service. 

These abstraction also allow for independent scripted testing for networking conditions and media pipelines in isolation.

JNI layer allows the integration with Java SDK and expose Java Producer interface on top of which MediaSoure interface is built.

The SDK also has some integration with "known" media pipelines in a form of GStreamer kvssink plugin, which takes care of the integration with the actual frames, similar to Android/Java MediaSource interface.

![GitHub Logo](/docs/Layering_and_Interfaces.png)

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


#### Fragment

While the integration of the media pipeline with the SDK (Producer interface) happens at the Frame granularity, the indexing and persistence in the KVS Service side happens at the Fragment granularity. Fragment is an abstraction - a collection of a set of frames which can be independently reproduced. Fragments durations should be within 1 - 10 seconds long. Some media types naturally have similar concepts - for example h264/h265 (and similar ones) have a concept of GoP (Group-of-Pictures) which includes an Idr frame followed by an I/P/B frames. Having the CPD (Codec Private Data) a decoder could unpack and playback a single fragment by itself. Most of the applications would map a single GoP to a single Fragment. Fragments are represented internally as MKV Clusters (https://www.matroska.org/). Other codecs do not have a concept of a Fragment built-in. For example, AAC audio encoder of M-JPG video encoding has same types of frames. In this case, the application could still integrate with the SDK as defined in below (see Key-frame-fragmentation).

The packaging which drives the fragmentation in the backend service is a high-level concept and should not affect the end-to-end elementary stream producing and consuming as shown in the following diagram:

![GitHub Logo](/docs/Realtime_e2e.png)


#### Codec Private Data

While the Fragments are self-contained, many encoders require a special set of bits which are required to get more information in order to reproduce the fragments. Collectively, the set of bits to configure the encoder is known as Codec Private Data or CPD. In case of H264, this is an SPS/PPS combination (see H264 spec) and in case of H265 it's VPS/SPS/PPS combination. Some audio encoders require channel configuration which is mapped as a CPD. Each one of the encoders will have it's specific format and meaning for the CPD. 

The KVS playback/reader API require CPD to be present for H264/H265 and AAC/PCM playback. The SDK appends the CPD in the SegmentInfo of the generated MKV header which is followed by the actual cluster and frame bits in a streamable fashion. The KVS service backend extracts the CPD and pre-pends it to every fragment, thus making each fragment a self-contained file. This is important to remember as GetMedia API consumer application should "strip" the following MKV headers in order to feed into most of the downstream media pipelines or processing engines. Consumer Parser Library contains a sample called OutputSegmentMerger which demonstrates how GetMedia output can be parsed and the interim MKV headers can be removed in order to generate a continuous MKV file.


#### Streaming session

KVS Stream is the managed AWS resource that can be created, tagged, updated, deleted, etc... Streams could be sparsely filled and some might have no fragments yet produced at all. Multiple streaming sessions could produce into the stream over time. Each streaming session is a single PutMedia request https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/API_dataplane_PutMedia.html

KVS SDK issuing a PutMedia API call packages a single MKV header with following single or multiple fragments. 

#### Fragment metadata

KVS SDK supports FragmentMetadata which is defined in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1218. It represents a key/value pair that can be inserted into the stream as the frames are being produced. It could contain arbitrary information specific to the application itself. 

Fragment metadata is implemented as MKV tags and as such they can not be inserted into the cluster or it will violate MKV syntax. The metadata is produced by calling putKinesisVideoFragmentMetadata API. The metadata will be collected internally and will be applied after the cluster is closed.

There are two types of fragment metadata - single time and persistent. The single will be inserted into the stream once while the persistent will be applied to every fragment.


### Optimizing for different scenarios

KVS is designed to handle a variety of streaming scenarios. Applications can control various behaviors by setting the appropriate parameters in the structures that are being passed in.


#### Realtime and Offline streaming modes

The SDK can be configured to stream in Realtime or Offline mode. The Realtime mode is intended to be used with a media pipeline that produces frames at the frame rate that's advertised in the StreamInfo structure. The behavior of the Realtime mode on the temporal or storage pressure is to evict the tail frames without blocking. The Offline mode is primarily designed for the case where the frames are produced at a much faster rate than the frame rate that's specified in the StreamInfo - for example, loading media from a file and streaming it. In this case, the frames are produced at the CPU/IO pipeline speed and will quickly saturate the buffer. The SDK in this mode blocks the media pipeline thread that's producing frames and awaits for the availability - either storage or temporal. As the frames are uploaded to the KVS backend, the Persisted ACKs will free the buffering space, unblocking the media thread to produce more frames into the buffer. This allows uploading of the media at the Network speed. Note: Offline mode requires persistence to be enabled on the stream by specifying a non-zero retention period.

The parameter is controlled by: the STREAMING_TYPE enum selection in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L885

When using Offline mode with a single client object multiple stream configuration, it is possible to have some of the streams become "starved" when the streams are blocked on the content store availability (physical storage which is shared amongs the streams rather than the temporal view). In this scenario, when a few streams get blocked on the availability of the storage, a stream that has faster ACKs delievered will "win" and the other streams will "starve" and the putFrame thread that's blocked will eventually timeout. It is therefore recommended to have a single client/single stream configuration.

The default putFrame timeout is specified as 15 seconds. This can be modified by specifying the timeout value other than 0 in offlineBufferAvailabilityTimeout member in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1048


#### Fragment ACKs

During streaming the SDK is uploading frame data and receiving a stream of ACKs. KVS is using application ACKs instead of relying on the transport/network ACKs for the following reasons:

* KVS business level logic is separated from the networking layer and as such we can't rely on it due to different network layer implementations of ACKs (nacks) or the lack of thereof.
* KVS has different types of acknowledgments (listed below) and a single networking nack won't work
* KVS needs to have a Fragment level and not a network packet level granularity
* Network packets can be nack-ed from the intermediate GW or load balancers and never reach the KVS ingestion backend.

The fragment ACKs are defined in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L794

The different types of fragment ACKs are 

* FRAGMENT_ACK_TYPE_BUFFERING - this ACK is sent by the ingestion backend as soon as the backend starts parsing the fragment
* FRAGMENT_ACK_TYPE_RECEIVED - the ingestion backend sends this ACK as soon as the fragment is successfully parsed and a fragment sequence number is generated.
* FRAGMENT_ACK_TYPE_PERSISTED - the ingestion backend sends this ACK after the fragment has been durably persisted and indexed. 
* FRAGMENT_ACK_TYPE_ERROR - this ACK type is an indication of an error during the ingestion. The different error types are listed in the AWS documentation. It's important to note that the ingestion backend itself will terminate the current connection. 
* FRAGMENT_ACK_TYPE_IDLE - sent periodically to keep the connection alive - not currently used.

KVS serializes the ACKs according to the fragment ingestion order. There is no hard guarantee however in the order of the types of ACKs but in general, the buffering ACK will follow by received and then persisted ACK. 

Receiving persisted ACK, the SDK will purge the tail from the content view and content store up-to and including the fragment at the timestamp referenced by the ACK. The ACK timestamp in this case is the timestamp of the first frame in the fragment.

Fragment ACK timestamp is the timestamp of the first frame in the fragment when parsed out. Error ACKs may in some cases have no timestamps (for example if the backend fails to parse the stream). 

In case an error ACK is received, the SDK will attempt to re-stream (more info on re-streaming is available in buffering.md document). 
