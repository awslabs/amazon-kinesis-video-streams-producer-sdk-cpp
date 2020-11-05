### KVS SDK handling of the media

Kinesis Video Streams end-to-end streaming is mostly content type agnostic. It handles any time encoded series, similar to audio and video. Some applications use GPS coordinates, others use Lidar or Radar streams. These applications integrate with the SDK via Producer interface which accepts an abstract Frame structure that can represent any time-encoded datum. The SDK as well as the KVS service overall have a few exceptions where the content type matters.

* H264/H265 video frames can be adapted from Annex-B format to AvCC and vice-versa.
* KVS SDK can auto-extract H264/H265 CPD (Codec Private Data) from an Annex-B Idr frames if the CPD has not been already specified by the application and NAL_ADAPTATION_ANNEXB_CPD_NALS flags have been specified in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L916. Many encoders that produce Annex-B format elementary streams include SPS/PPS (and in case of H265 VPS) NALs pre-pended to the Idr frames.
* KVS SDK can adapt CPD from Annex-B format to AvCC format for H264 and H265
* KVS SDK can adapt frames from Annex-B format to AvCC format and in the opposite direction for H264 and H265
* KVS SDK will automatically attempt to extract video pixel width and height (for a number of formats) and include PixelWidth and PixelHeight elements in the generated MKV (https://www.matroska.org/technical/elements.html) as some downstream consumers require it for video.
* KVS SDK will automatically attempt to extract and set Audio specific elements in the MKV for audio streams as those can be required by the downstream consumers.
* KVS SDK has a set of APIs to generate audio specific CPD.
* KVS HLS/MPEG-DASH/Console playback and clip generation require specific elementary stream and packaging formats - for example, the console playback requires H264/H265 elementary stream with CPD in AvCC format. More information on the supported formats and limitations can be found in AWS documentation.


#### Indexing, persistence and low-latency

KVS designed to handle real-time as well as offline scenarios. In both cases, the smallest granularity of data to be indexed is based on the abstract Fragment which is a collection abstract Frames that can be used/replayed individually. In case of H264 for example, the Fragment can correspond to a single GoP or a collection of GoPs. In case of audio, it could be a collection of audio samples a few seconds which comprise a fragment. The backend indexing and persistence happens on a per-fragment granularity. On the SDK-side, the integration happens on a per-frame basis as majority of the use cases have a media pipeline producing a frame at a time. The SDK integrating with the media pipeline would take in the frames (including frame data, flags and timestamps), proceed with on-the-fly packaging it into an MKV format and store the information in the buffer while the networking thread is uploading the content of the buffer. This means that the integration of an application with the SDK is on a frame granularity. The SDK uploads the content of the buffer on a bit-granularity.

In order to achieve a low-latency streaming case, the MKV structure should contain a "streamable" format. This means that the packaging structures would not know the size nor the duration of the fragments at the time of packaging/encoding as the bits would need to be sent out before the entire content of the fragment is generated in real-time. MKV format handles the streamability by using a sentinel value "unknown" for the sizes or durations. For example, the size of the Segment or Cluster element in MKV is set to unknown. This allows KVS to use MKV format in a streamable fashion allowing bit-level granularity and low-latency streaming while still having a fully defined structure allowing for indexing, persistence and ACKs.


#### Real-time vs Live/Offline scenarios of streaming

KVS service API has two categories of media APIs. Only GetMedia is designed to be used in a real-time end-to-end scenario combined with real-time PutMedia. 

![GitHub Logo](/docs/Realtime_and_Live.png)


In this scenario, 1 - 3 and 8 - 9 have a frame-level granularity, 4 - 7 have a bit-level granularity, 10 and 11 have a fragment-level granularity.

This means that the playback or analytics applications which are based on the Realtime path can have a low latency and they have no dependency on the fragment granularity, whereas the Live/Offline scenarios are based on indexing will have at least fragment duration latencies.


#### Frame timestamp
KVS SDK can be used to handle different types of timestamps. For more information about timestamps please refer to

![GitHub Logo](/docs/timestamps.md)

Many encoders produce proper timestamps that can be used directly. Setting StreamInfo.StreamCaps.frameTimecodes = TRUE ensures that the SDK will use the supplied timestamps with the frame structure. However, some encoders do not produce frame timestamps and in those cases the SDK can be configured to use GETTIME API to generate the timestamps whenever the frame is produced. Note: in this mode, the encoder should be "stable" which means it should produce frames at the regular intervals. 

KVS SDK requires that the PTS be monotonically increasing and the frames with added durations not to overlap. Ex: Timestamp (Fn) + duration <= Timestamp (Fn + 1)

Encoders can produce timestamp on different timelines. Some use timestamps starting from 0, others use system clock which normally counts ticks from the device boot while others use realtime wall clock absolute time. Those absolute times could be in ticks from the Unix epoch. Whatever the timestamp timeline is, if the SDK StreamInfo is specifying to use frame timestamps then those will be used in the final stream. This means that the Producer index will be based on these times and the consumer application requesting Producer timestamp selector will need to understand the producer timeline.

KVS SDK explicitly does not handle NTP sync, it's up to the device firmware or the application using the SDK to set up the system clock properly to ensure the default GETTIME returns the world clock. This is important not only for the media timeline but also for the authentication with AWS services.


#### Key-frame-fragmentation

KVS SDK is designed to integrate with any type of media pipeline. This includes the different types of encoders and scenarios. Some media types and the encoders (ex: h264) align well with the concept of the Fragment, mapping a single GoP (Group-of-pictures) to a single Fragment. Selecting StreamInfo.StreamCaps.keyFrameFragmentation = TRUE will produce MKV clusters (which will be the Fragments) according to the GoPs that the encoder generates. Thus, if the encoder is configured to produce Idr frames periodically (ex: every 4 seconds or in 25 fps, every 50th frame which would equal to every 2 seconds) then the frame marked as a key-frame on the Idr frame will drive the fragmentation in the SDK.

There are other encoding types (AAC for example) where there is no concept of local fragmentation. In this case, the application could choose to do exactly the same thing as in case of the h264 video and mark the frames as key-frames periodically to enforce the SDK to fragment. Alternatively, to make it easier, the SDK can be configured with selecting the following

* Every frame to be a key-frame (easy media pipeline integration)
* Set StreamInfo.StreamCaps.keyFrameFragmentation = FALSE
* Set StreamInfo.StreamCaps.fragmentDuration = 4 * HUNDREDS_OF_NANOS_IN_A_SECOND

In this case, the SDK will produce fragments 4 seconds long.

The above parameters can be used to optimize for application specific requirements. For example, in some advanced use cases where the encoder is configured to produce more frequent GoPs and the application desires longer fragment durations, the application could choose to mark not every Idr frame as a key-frame but combine a few GoPs into a single fragment by selectively marking Idr frames as key frames.



#### Examples of media types and the appropriate settings

Basic abstract Frame structure and the StreamInfo.StreamCaps can be used to model different types of media pipelines. Below are some examples of different selections.

1) H264 elementary stream produced by a camera in realtime needed for HLS/MPEG-DASH playback. In this case the encoder produces Annex-B format NALus for the elementary H264 and the HLS/MPEG-DASH playback requires AvCC format. Set the StreamInfo.streamingMode to Realtime, set the encoder to produce an I-frame every 2 - 4 seconds. Set StreamInfo.StreamCaps.frameTimestamps = TRUE, keyFrameFragmentation = TRUE, nalAdaptationMode = ADAPT_FRAME_NAL | ADAPT_CPD_ANNEXB. The settings will let the CPD be extracted from the I-frame and adapted from Annex-B to AvCC mode. The frames will also be adapted. The resulting stream can be consumed in realtime or played back using MPEG-DASH.

2) Audio AAC streaming. In case of AAC every frame is self-contained so can be an I-frame (key-frame). StreamInfo.streamingMode to Realtime. Set StreamInfo.StreamCaps.frameTimestamps = FALSE, keyFrameFragmentation = FALSE, fragmentDuration = 4 * HUNDREDS_OF_NANOS_IN_A_SECOND, nalAdaptationMode = NAL_ADAPTION_MODE_NONE. Set the KEY_FRAME_FLAG on every frame. This will let the SDK to use the system clock to timestamp the frames as they get produced, each frame is a key-frame but the fragments will have the fragmentDuration length.

