### Intermittent Producer Handling
Since v3.1.0 of the Producer C++ SDK by default we have automatic handling for intermittent streaming.  This comes at a cost of using 1 extra thread to manage the timer used by the SDK to automatically signal to the KVS back-end that a fragment can be closed out.  If on a prior release of the SDK you were manually setting fields like `stream_info_.streamCaps.frameOrderingMode` you no longer should do that.  An example of this scenario is if you stream for an amount of time, then you stop producing frames (for example the case of a security camera which is only uploading video/audio during a motion event) and then after an unknown amount of time (for example the next motion event) you start streaming again.  This is all automatically handled by the SDK now.

If for some reason you would like to opt-out of this functionality (which is enabled by default) then in the method `KinesisVideoProducer::create` right after we create `device_info` you can set the following:

```
device_info.clientInfo.automaticStreamingFlags = AUTOMATIC_STREAMING_ALWAYS_CONTINUOUS
```




