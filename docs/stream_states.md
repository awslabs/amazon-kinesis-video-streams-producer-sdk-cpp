### Stream state machine and state transitions

KVS Produder SDK has a state machine described in the below image to drive the stream through it's lifecycle. The core stream states are represented below.  


![GitHub Logo](/docs/Stream_state_machine_states.png)

There are other internal states that are not as important in the context of the public APIs. Similarly, there is a Client level state machine (omitted) that handles the authentication integration and Client object lifecycle.


When the client calls 'createKinesisVideoStreamSync' API the format state machine is created to iterate the stream through it's states all the way to the "Ready" state. The DescribeStream state could move to CreateStream state if the stream is not created. The TagStream state is "optional" so it will skip the state on errors returned from the ingestion backend but will retry on errors that might occur on the client or on the wire. 

Most of the states are "retriable" on some of the errors which means that the state machine will not step out of the state and will attempt to retry it. Some non-retriable errors include authentication errors, bad argument errors, etc.. The states are configured to retry up-to 5 times at the core state machinery level and return an error. The error will be propagated either to the caller (in case an API call initiated the state transition) or via an Error callback API in case if this is a "dynamic" error - for example when an error occures during the streaming or during the token rotation. Higher layers of the SDK (C Producers Continuous Retry Policy) will catch errors and retry entire state machine by issuing a reset API call to keep iterating. We are working on adding other high-level policy engines.

The stream will stay in the Ready state until a frame is produced into the stream by the application calling 'putKinesisVideoFrame' API. The state machine will be moved to PutStream state where a PutMedia call will be issued and as soon as the result is back the state machine will move to the Streaming state. 

The stream can move to Stopped state on an error, disconnect or when the application calls stop API. In the first two cases, if the stream is configured with "retry" logic, it will move to an appropriate state to retry.


### Asynchronous API threading

The state machine transitions are highly asynchronous at the core. The createKinesisVideoStreamSync API internally calls createKinesisVideoStream API which is asynchronous and awaits until StreamReady callback is fired or an error or a timeout occurs. The timeout for the sync API is configured via DeviceInfo.ClientInfo API. The thread calling createKinesisVideoStreamS API will end up calling DescribeStream callback which higher layer networking callback provider will use to issue an async call to DescribeStream backend API. The thread then immediately returns after scheduling the call and the async API returns. The result of the network call with either success or a failure will be calling to notification event API DescribeStreamResult using the network thread. The thread then will drive the state machinery and issue another async callback (for example CreateStream) which will schedule the call on the network stack and without blocking will return "promptly", finishing the DescribeResultEvent API.

The entire KVS PIC is "liveless" which means it does not create or own threads. It's a static library that only acts when something "pokes" at it. The "poking" happens when an API is called or an event happens.

During the run time, for a given Stream there are usually two threads - one is the media thread that produces frames and the other is the networking thread reading the packaged frame bits or issuing calls to the backend APIs. All of these threads can be used whenever the state machine needs it to drive the states.

The resulting codebase is highly portable, super fast and very small, at the expense of intricate interlocking and strict rules of adherence to the state machine transitions.


#### Continuation model using caller threads

As described, the core of the KVS Producer SDK - PIC does not have it's own threads. Below is a state transition diagram describing the caller threads. To NOTE: each thread execution is "prompt" which means that the thread is not blocked. This is useful feature which would allow UI or main threads to call PIC APIs or events. Similarly, the PIC can be called from the networking or encoder threads. PIC does not have any thread affinity making custom applications highly scalable.


![GitHub Logo](/docs/Continuation_model.png)