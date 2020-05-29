#ifndef __KVS_SINK_STREAM_CALLBACK_PROVIDER_H__
#define __KVS_SINK_STREAM_CALLBACK_PROVIDER_H__

#include "gstkvssink.h"
#include "Logger.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
    class KvsSinkStreamCallbackProvider : public StreamCallbackProvider {
        std::shared_ptr<KvsSinkCustomData> data;
    public:
        KvsSinkStreamCallbackProvider(std::shared_ptr<KvsSinkCustomData> data) : data(data) {}

        UINT64 getCallbackCustomData() override {
            return reinterpret_cast<UINT64> (data.get());
        }

        StreamConnectionStaleFunc getStreamConnectionStaleCallback() override {
            return streamConnectionStaleHandler;
        };

        StreamErrorReportFunc getStreamErrorReportCallback() override {
            return streamErrorReportHandler;
        };

        DroppedFrameReportFunc getDroppedFrameReportCallback() override {
            return droppedFrameReportHandler;
        };

        StreamLatencyPressureFunc getStreamLatencyPressureCallback() override {
            return streamLatencyPressureHandler;
        }

        StreamClosedFunc getStreamClosedCallback() override {
            return streamClosedHandler;
        }

    private:
        static STATUS
        streamConnectionStaleHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                     UINT64 last_buffering_ack);

        static STATUS
        streamErrorReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle, UINT64 errored_timecode,
                                 STATUS status_code);

        static STATUS
        droppedFrameReportHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                  UINT64 dropped_frame_timecode);

        static STATUS
        streamLatencyPressureHandler(UINT64 custom_data, STREAM_HANDLE stream_handle,
                                  UINT64 current_buffer_duration);

        static STATUS
        streamClosedHandler(UINT64 custom_data, STREAM_HANDLE stream_handle, UPLOAD_HANDLE upload_handle);
    };
}
}
}
}


#endif //__KVS_SINK_STREAM_CALLBACK_PROVIDER_H__
