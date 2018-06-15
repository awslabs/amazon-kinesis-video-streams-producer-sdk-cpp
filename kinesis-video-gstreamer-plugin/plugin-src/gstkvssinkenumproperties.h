#ifndef __GST_KVS_SINK_ENUM_PROPERTIES_H__
#define __GST_KVS_SINK_ENUM_PROPERTIES_H__

typedef enum {
    KVS_SINK_TIMESTAMP_PTS_ONLY,
    KVS_SINK_TIMESTAMP_DTS_ONLY,
    KVS_SINK_TIMESTAMP_DEFAULT, //uses incoming timesttamps from stream and will be used if either of them are valid
} GstKvsSinkFrameTimestamp;

#endif //__GST_KVS_SINK_ENUM_PROPERTIES_H__
