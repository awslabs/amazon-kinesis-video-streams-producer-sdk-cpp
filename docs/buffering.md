### Producer SDK buffering

Producer SDK PIC handles the frame buffering by separating the physical storage called Content Store from the logical view known as Content View. The Content Store is a simple storage based on an internal heap implementation. The DeviceInfo.StorageInfo https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1012 controls the type of the storage and the amount of memory to reserve. The Content Store is shared among all of the streams within the client object. Content View, on the other hand, contains the temporal logic of the frames that are in the buffer. The Content View is created per each stream. As the frames are pushed to the SDK using PutFrame API, the frames are being packaged and put in the Content Store and the reference with timestamp information is being produced into the Content View. 


![GitHub Logo](/docs/Content_View_Storage.png)
Format: ![Alt Text](url)


### Content Store

Content store is an abstraction of the underlying storage that can have different implementations. By default, the implementation is based on low-fragmentation, tightly packed heap which can provide good performance characteristics processing "rolling window"-like allocations of similar sizes with minimal waste of memory. Moreover, the content store abstraction allows for dynamic resizing and indirect mapping which are useful in cases of "hybrid" store chaining with spill-over (for example RAM-based heap with spill-over on eMMC-backed storage). 
Storage overflow callback (https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L1409) will be called when there is less than 5% of storage available.

### Content View

Content View can be thought of as a list of frame information that includes timestamps, flags and the handle to the actual storage. The new frames are produced into the Head (and the head moves forward) and the frames are being read from Current pointer. The last frame is pointed to by Tail pointer. The tail moves forward either when the SDK receives Persisted ACK or on buffer pressure when the duration window reaches the buffer duration that's specified in StreamInfo.StreamCaps https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L882. As the frames are pushed out of the tail, their content is being purged from the Content Store. If the Tail catches up with Current, a dropped frame callback will be called to notify the application of dropped frames. 

The production into the buffer is done at frame granularity (PutFrame takes an entire frame) whereas the Networking stack reading the frame bits operates on bit granularity. The MKV packaging format allows the frames to be "streamable" where the frame bits can be streamed out before the last frame of the fragment can be produced into the buffer, thus, making low-latency streaming possible.


### Controlling SDK buffering and callbacks

StreamInfo.streamCaps.bufferDuration - specifies the entire duration of the buffer keep in the content store before purging from the tail. As the frames fall off the Tail, DroppedFragmentReportFunc callback will be called if the frames have not been yet sent (aka. the Tail crosses the Current pointer). 


StreamInfo.streamCaps.maxLatency - specifies the duration in excess of which StreamLatencyPressureFunc callback will be called. The value should be anywhere between a couple of fragments duration to bufferDuration. If the value is 0 or greater than bufferDuration, no latency pressure callback will be issued. C Producer layer default logic will attempt to re-connect when this callback is fired potentially resulting in a better connection.


DeviceInfo.storageInfo.storageSize - specifies the overall content store size to allocate for storing all of the buffered frames for all of the streams (in case the for a given Client object). The default behavior on overflowing the storage is to evict the tail frames until there is enough storage available to put a new frame. As the content store fills up and the utilization reaches 95%, content store pressure callback will be issued.


### Dropping frames

The SDK drops frames from the Content View in the following scenarios

* If the stream is configured with persistence and the Persisted ACK is received.
* When the frames naturally fall off the tail as the new frames are being added and the delta between the new frame and the oldest Tail frame is greater than the buffer_duration.

In the first case, the ACK timestamp is mapped in the Content View timeline to find the Key-frame that the ACK applies to. The tail is then moved past to the next Key-frame to trim the workset as those frames are no longer needed (the fragment had already been durably persisted).

In the latter case, the frames that get dropped at the tail might or might not yet have been sent out. The Tail frame could be partially sent out. The default policy controls on how the frames are being dropped. If the Tail frame has been sent out, it will be dropped without calling the dropped frame callback (success case). In case it's been partially sent, it will be preserved and attempted to be sent to ensure syntactic correctness of the resulting MKV stream. The following frames will be dropped until the next Key-frame to free up buffer space.

The default policy applied to PutFrame API call on storage pressure is to attempt to evict the tail frames to make space for the newly produced frame.


### Stream staleness

KVS streaming abstracts the actual networking protocol and doesn't rely on the network-level ACKs. Moreover, the network level ACKs are still insufficient for guaranteeing the delivery of the video data to the backend. Stream staleness is calculated by measuring the time between the successive Buffering ACKs. If this time is above the threshold specified in StreamCaps (https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L941) the StreamConnectionStaleFunc callback will be executed. The default handler for this callback will reset the connection but the applications could choose to perform other actions if needed.
