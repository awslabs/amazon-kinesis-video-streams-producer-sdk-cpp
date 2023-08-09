### Producer SDK buffering

Producer SDK PIC handles the frame buffering by separating the physical storage called Content Store from the logical view known as Content View. The Content Store is a simple storage based on an internal heap implementation. The DeviceInfo.StorageInfo https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/032aa7843f58151f41fb0ab9b473a02338e6f76f/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1115 controls the type of the storage and the amount of memory to reserve. The Content Store is shared among all of the streams within the client object. Content View, on the other hand, contains the temporal logic of the frames that are in the buffer. The Content View is created per each stream. As the frames are pushed to the SDK using PutFrame API, the frames are being packaged and put in the Content Store and the reference with timestamp information is being produced into the Content View. 


![GitHub Logo](/docs/Content_View_Storage.png)


### Content Store

Content store is an abstraction of the underlying storage that can have different implementations. By default, the implementation is based on low-fragmentation, tightly packed heap which can provide good performance characteristics processing "rolling window"-like allocations of similar sizes with minimal waste of memory/fragmentation. Moreover, the content store abstraction allows for dynamic resizing and indirect mapping which are useful in cases of "hybrid" store chaining with spill-over (for example RAM-based heap with spill-over on eMMC-backed storage). 
Storage overflow callback (https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/032aa7843f58151f41fb0ab9b473a02338e6f76f/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1579) will be called when there is less than 5% of storage available.


### Content View

Content View can be thought of as a list of frame information that includes timestamps, flags and the handle to the actual storage. The new frames are produced into the Head (and the head moves forward) and the frames are being read from Current pointer. The last frame is pointed to by Tail pointer. The tail moves forward either when the SDK receives Persisted ACK or on buffer pressure when the duration window reaches the buffer duration that's specified in StreamInfo.StreamCaps https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/032aa7843f58151f41fb0ab9b473a02338e6f76f/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L982. As the frames are pushed out of the tail, their content is being purged from the Content Store. If the Tail catches up with Current, a dropped frame callback will be called to notify the application of dropped frames. 

The production into the buffer is done at frame granularity (PutFrame takes an entire frame) whereas the Networking stack reading the frame bits operates on bit granularity. The MKV packaging format allows the frames to be "streamable" where the frame bits can be streamed out before the last frame of the fragment can be produced into the buffer, thus, making low-latency streaming possible.


### Controlling SDK buffering and callbacks

Two main structures control most of the SDK behavior. DeviceInfo structure is applicable to the client object overall whereas each stream can be configured with StreamInfo structure. The values in the structure can affect how the SDK core behaves. At runtime, the SDK communicates with the application through callbacks. The following members affect the buffering logic.

StreamInfo.streamCaps.bufferDuration - specifies the entire duration of the buffer keep in the content store before purging from the tail. As the frames fall off the Tail, DroppedFragmentReportFunc callback will be called if the frames have not been yet sent (aka. the Tail crosses the Current pointer). 


StreamInfo.streamCaps.maxLatency - specifies the duration in excess of which StreamLatencyPressureFunc callback will be called. The value should be anywhere between a couple of fragments duration to bufferDuration. If the value is 0 or greater than bufferDuration, no latency pressure callback will be issued. C Producer layer default logic will attempt to reset the connection (re-connect) when this callback is fired potentially resulting in a better connection.


DeviceInfo.storageInfo.storageSize - specifies the overall content store size to allocate for storing all of the buffered frames for all of the streams (in case there are multiple streams for a given Client object). The default behavior on overflowing the storage is to evict the tail frames until there is enough storage available to put a new frame. As the content store fills up (due to buffering) storage pressure callback will be issued when the Content Store utilization reaches 95% (less than 5% available storage).


### Dropping frames

The SDK drops frames from the Content View in the following scenarios

* If the stream is configured with persistence and the Persisted ACK is received.
* When the frames naturally fall off the tail as the new frames are being added and the delta between the new frame and the oldest Tail frame is greater than the buffer_duration.
* When there is not enough physical storage available in the Content Store to store the new frame.

In the first case, the ACK timestamp is mapped in the Content View timeline to find the Key-frame that the ACK applies to. The tail is then moved past to the next Key-frame to trim the workset as those frames are no longer needed (the fragment had already been durably persisted).

In the latter cases, the stream said to have "pressures" - latency and storage respectively. The frames that get dropped at the tail might or might not yet have been sent out. The Tail frame could be partially sent out. The default policy controls on how the frames are being dropped. If the Tail frame has been sent out, it will be dropped without calling the dropped frame callback (success case). In case it's been partially sent, it will be preserved and attempted to be sent to ensure syntactic correctness of the resulting MKV stream. The following frames will be dropped until the next Key-frame to free up buffer space.

The default policy applied to PutFrame API call on storage pressure is to attempt to evict the tail frames to make space for the newly produced frame.


### Skipping frames and partial frames

Frames can be omitted by not only "dropping" them but also "skipping" them. There are the following cases when the frames can be skipped.

1) When the SDK encounters buffer pressures and needs to drop a tail frame, the tail frame might have been partially ingested and if the SDK drops this frame it will result in a syntactically invalid Fragment. The ingestion backend parser will generate an error ACK parsing the stream. For this purpose, the partially sent frames are not being dropped on the stream pressure and they are kept in the temporal buffer until they are fully ingested. The immediate next frames are the ones that are being dropped (on storage or temporal pressures). If the pressures continue, more of the consequent frames will be dropped and the partially sent frame will be kept in order to stream the remaining bits of the frame to keep the syntactic validity. The partial frame will be dropped if the session is terminated - for example as a result of re-connecting.
2) When the SDK starts a new session, by default it will skip the non-key frames and start the session from the next key-frame
3) When the SDK determines inherently "invalid" fragments as the backend flags them by sending certain types of Error ACKs (ex: INVALID_MKV_DATA) the SDK marks them as "skip" and will not submit them even in case of a rollback that moves the current back behond the "invalid" fragment frames.


### Stream staleness

KVS streaming abstracts the actual networking protocol and doesn't rely on the network-level ACKs. Moreover, the network level ACKs are still insufficient for guaranteeing the delivery of the video data to the backend. Stream staleness is calculated by measuring the time between the successive Buffering ACKs. If this time is above the threshold specified in StreamCaps (https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/032aa7843f58151f41fb0ab9b473a02338e6f76f/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1041) the StreamConnectionStaleFunc callback will be executed. The default handler for this callback will reset the connection but the applications could choose to perform other actions if needed.


### Stream rollback

KVS SDK is configured by default to re-connect/re-stream on error/disconnection as set in StreamInfo.StreamCaps.recoverOnError https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/032aa7843f58151f41fb0ab9b473a02338e6f76f/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1012 

During streaming, the connection could get disconnected or an error could be generated by the backend in a form of an error ACK. In these cases, the SDK attempts to drive the internal state machinery through the states and reconnect. Once reconnected, it will need to make a decision from where to restart the streaming in order to still provide durability guarantees and ensure that the frames in the internal buffer are persisted. 

1) Determine the timestamp on the internal Content View timeline on when the issue happens. For example, if it's a simple network disconnect then the Current from the content view is used whereas if there is an error ACK causing the termination of the connection and if we have a timestamp specified in the ACK then we set the time to the timestamp specified in the error ACK.
2) Check the reason for disconnection. Some error indicate that the stream has an issue (for example error ACKs) in which case the SDK determines that the processing host is still alive and the data is still in it's buffer but the fragment which errored is inherently can not be ingested even if it's retried so it needs to be skipped. There are other error class category that indicate a simple networking timeout for example, in which case we determine that the host might still be alive and the so-far ingested but not yet persisted data might still be in the ingestion host internal buffer. While, there are other cases that we determine for sure that the host itself is likely to be dead and the data in the buffer that hasn't been durably persisted is gone.
3) If the error is determined to be caused by a "dead" host then the rollback should roll all the way to the fragment that has a timestamp of last Persisted ACK fragments next timestamp within the buffer or the rollback duration - whichever is less. Rollback duration is specified in StreamInfo.StreamCaps.replayDuration https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/032aa7843f58151f41fb0ab9b473a02338e6f76f/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1034
4) If the error is determined to be caused by a connection issue then the host is likely to be "alive" and the not-yet persisted data is accessible from within the hosts buffer. In this case the rollback happens from the current position back until we 'replayDuration' or last Received ACK fragments next fragment. NOTE: in both 3) and 4) the Received and Persisted ACK fragment have been already ingested as the ACK timestamp for Persisted and Received ACKs is the timestamp of the first frame of the Fragment being ACK-ed.
5) Restarted stream will skip over the fragments which are marked as "skip". These are the fragments that have been determined to cause issues with the backend parsing (error ACK).
