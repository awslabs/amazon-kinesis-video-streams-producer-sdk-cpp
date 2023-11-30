#include "gstkvssink.h" //import this first, or will cause build error on Mac
#include <gst/check/gstcheck.h>
#include <string>

using namespace std;

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS(
                                                                           "video/x-h264,stream-format=avc,alignment=au"
                                                                   ));
static char const *accessKey;
static char const *secretKey;

static GstElement *
setup_kinesisvideoproducersink(void)
{
    GstElement *kinesisvideoproducersink;
    GST_DEBUG ("Setup kinesisvideoproducersink");
    kinesisvideoproducersink = gst_check_setup_element ("kvssink");
    g_object_set(G_OBJECT (kinesisvideoproducersink),
                 "access-key", accessKey,
                 "secret-key", secretKey,
                 NULL);
    return kinesisvideoproducersink;
}

static void
cleanup_kinesisvideoproducersink(GstElement * kinesisvideoproducersink)
{

    gst_check_teardown_src_pad (kinesisvideoproducersink);
    gst_check_teardown_element (kinesisvideoproducersink);
}

GST_START_TEST(kvsproducersinktestplay)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_PLAYING), GST_STATE_CHANGE_ASYNC);
        cleanup_kinesisvideoproducersink(pElement);

    }
GST_END_TEST;

GST_START_TEST(kvsproducersinktestpause)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_PAUSED), GST_STATE_CHANGE_ASYNC);
        cleanup_kinesisvideoproducersink(pElement);

    }
GST_END_TEST;

GST_START_TEST(kvsproducersinktestplaytopausetoplay)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        GstPad *srcpad;

        srcpad = gst_check_setup_src_pad_by_name (pElement, &srctemplate, "sink");

        gst_pad_set_active (srcpad, TRUE);

        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_NULL_TO_READY), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_READY_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        // dummy cpd. Also the caps event need to be sent after the kvssink is in paused state in order for it to received it
        gst_pad_push_event(srcpad, gst_event_new_caps (gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au,codec_data=abc")));
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PAUSED_TO_PLAYING), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PLAYING_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        cleanup_kinesisvideoproducersink(pElement);

    }
GST_END_TEST;

GST_START_TEST(kvsproducersinkteststop)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
        cleanup_kinesisvideoproducersink(pElement);

    }
GST_END_TEST;

GST_START_TEST(check_properties_are_passed_correctly)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        string TEST_STREAM_NAME = "test-stream";
        guint TEST_RETENTION_PERIOD = 2;
        STREAMING_TYPE TEST_STREAMING_TYPE = STREAMING_TYPE_NEAR_REALTIME;
        string TEST_CONTENT_TYPE = "content";
        guint TEST_MAX_LATENCY = 20;
        guint TEST_FRAGMENT_DURATION = 2;
        guint TEST_TIMECODE_SCALE = 10;
        gboolean TEST_KEY_FRAME_FRAGMENTATION = true;
        gboolean TEST_FRAME_TIMECODES = false;
        gboolean TEST_ABSOLUTE_FRAGMENT_TIMES = true;
        gboolean TEST_FRAGMENT_ACKS = false;
        gboolean TEST_RESTART_ON_ERROR = true;
        gboolean TEST_RECALCULATE_METRICS = false;
        guint TEST_FRAMERATE = 25;
        guint TEST_AVG_BANDWIDTH_BPS = 100;
        guint TEST_BUFFER_DURATION = 20;
        guint TEST_REPLAY_DURATION = 20;
        guint TEST_CONNECTION_STALENESS = 20;
        string TEST_CODEC_ID = "codec_id";
        string TEST_TRACK_NAME = "trackname";
        string TEST_ACCESS_KEY = "access_key";
        string TEST_SECRET_KEY = "secret_key";
        guint TEST_ROTATION_PERIOD = 20;
        string TEST_LOG_CONFIG = "test_path/test_log_config";

        g_object_set(G_OBJECT (pElement),
                     "stream-name", TEST_STREAM_NAME.c_str(),
                     "retention-period", TEST_RETENTION_PERIOD,
                     "streaming-type", TEST_STREAMING_TYPE,
                     "content-type", TEST_CONTENT_TYPE.c_str(),
                     "max-latency", TEST_MAX_LATENCY,
                     "fragment-duration", TEST_FRAGMENT_DURATION,
                     "timecode-scale", TEST_TIMECODE_SCALE,
                     "key-frame-fragmentation", TEST_KEY_FRAME_FRAGMENTATION,
                     "frame-timecodes", TEST_FRAME_TIMECODES,
                     "absolute-fragment-times", TEST_ABSOLUTE_FRAGMENT_TIMES,
                     "fragment-acks", TEST_FRAGMENT_ACKS,
                     "restart-on-error", TEST_RESTART_ON_ERROR,
                     "recalculate-metrics", TEST_RECALCULATE_METRICS,
                     "framerate", TEST_FRAMERATE,
                     "avg-bandwidth-bps", TEST_AVG_BANDWIDTH_BPS,
                     "buffer-duration", TEST_BUFFER_DURATION,
                     "replay-duration", TEST_REPLAY_DURATION,
                     "connection-staleness", TEST_CONNECTION_STALENESS,
                     "codec-id", TEST_CODEC_ID.c_str(),
                     "track-name", TEST_TRACK_NAME.c_str(),
                     "access-key", TEST_ACCESS_KEY.c_str(),
                     "secret-key", TEST_SECRET_KEY.c_str(),
                     "rotation-period", TEST_ROTATION_PERIOD,
                     "log-config", TEST_LOG_CONFIG.c_str(),
                     NULL);

        guint uint_val;
        gchar *str_val;
        gint int_val;

        g_object_get(G_OBJECT (pElement), "stream-name", &str_val, NULL);
        assert_equals_string(TEST_STREAM_NAME.c_str(), str_val);
        g_object_get(G_OBJECT (pElement), "retention-period", &uint_val, NULL);
        assert_equals_uint64(TEST_RETENTION_PERIOD, uint_val);
        g_object_get(G_OBJECT (pElement), "streaming-type", &uint_val, NULL);
        assert_equals_uint64(TEST_STREAMING_TYPE, uint_val);
        g_object_get(G_OBJECT (pElement), "content-type", &str_val, NULL);
        assert_equals_string(TEST_CONTENT_TYPE.c_str(), str_val);
        g_object_get(G_OBJECT (pElement), "max-latency", &uint_val, NULL);
        assert_equals_uint64(TEST_MAX_LATENCY, uint_val);
        g_object_get(G_OBJECT (pElement), "fragment-duration", &uint_val, NULL);
        assert_equals_uint64(TEST_FRAGMENT_DURATION, uint_val);
        g_object_get(G_OBJECT (pElement), "timecode-scale", &uint_val, NULL);
        assert_equals_uint64(TEST_TIMECODE_SCALE, uint_val);
        g_object_get(G_OBJECT (pElement), "key-frame-fragmentation", &int_val, NULL);
        assert_equals_int(TEST_KEY_FRAME_FRAGMENTATION, int_val);
        g_object_get(G_OBJECT (pElement), "frame-timecodes", &int_val, NULL);
        assert_equals_int(TEST_FRAME_TIMECODES, int_val);
        g_object_get(G_OBJECT (pElement), "absolute-fragment-times", &int_val, NULL);
        assert_equals_int(TEST_ABSOLUTE_FRAGMENT_TIMES, int_val);
        g_object_get(G_OBJECT (pElement), "fragment-acks", &int_val, NULL);
        assert_equals_int(TEST_FRAGMENT_ACKS, int_val);
        g_object_get(G_OBJECT (pElement), "restart-on-error", &int_val, NULL);
        assert_equals_int(TEST_RESTART_ON_ERROR, int_val);
        g_object_get(G_OBJECT (pElement), "recalculate-metrics", &int_val, NULL);
        assert_equals_int(TEST_RECALCULATE_METRICS, int_val);
        g_object_get(G_OBJECT (pElement), "framerate", &uint_val, NULL);
        assert_equals_uint64(TEST_FRAMERATE, uint_val);
        g_object_get(G_OBJECT (pElement), "avg-bandwidth-bps", &uint_val, NULL);
        assert_equals_uint64(TEST_AVG_BANDWIDTH_BPS, uint_val);
        g_object_get(G_OBJECT (pElement), "buffer-duration", &uint_val, NULL);
        assert_equals_uint64(TEST_BUFFER_DURATION, uint_val);
        g_object_get(G_OBJECT (pElement), "replay-duration", &uint_val, NULL);
        assert_equals_uint64(TEST_REPLAY_DURATION, uint_val);
        g_object_get(G_OBJECT (pElement), "connection-staleness", &uint_val, NULL);
        assert_equals_uint64(TEST_CONNECTION_STALENESS, uint_val);
        g_object_get(G_OBJECT (pElement), "codec-id", &str_val, NULL);
        assert_equals_string(TEST_CODEC_ID.c_str(), str_val);
        g_object_get(G_OBJECT (pElement), "track-name", &str_val, NULL);
        assert_equals_string(TEST_TRACK_NAME.c_str(), str_val);
        g_object_get(G_OBJECT (pElement), "access-key", &str_val, NULL);
        assert_equals_string(TEST_ACCESS_KEY.c_str(), str_val);
        g_object_get(G_OBJECT (pElement), "secret-key", &str_val, NULL);
        assert_equals_string(TEST_SECRET_KEY.c_str(), str_val);
        g_object_get(G_OBJECT (pElement), "rotation-period", &uint_val, NULL);
        assert_equals_uint64(TEST_ROTATION_PERIOD, uint_val);
        g_object_get(G_OBJECT (pElement), "log-config", &str_val, NULL);
        assert_equals_string(TEST_LOG_CONFIG.c_str(), str_val);

        cleanup_kinesisvideoproducersink(pElement);

    }
GST_END_TEST;

GST_START_TEST(check_playing_to_paused_and_back_to_playing)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        GstPad *srcpad;

        srcpad = gst_check_setup_src_pad_by_name (pElement, &srctemplate, "sink");

        gst_pad_set_active (srcpad, TRUE);

        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_NULL_TO_READY), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_READY_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        gst_pad_push_event(srcpad, gst_event_new_caps (gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au,codec_data=abc")));
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PAUSED_TO_PLAYING), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PLAYING_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PAUSED_TO_PLAYING), GST_STATE_CHANGE_SUCCESS);
        cleanup_kinesisvideoproducersink(pElement);
    }
GST_END_TEST;

//Verify all State change events and direct state set events.

Suite *gst_kinesisvideoproducer_suite(void) {
    Suite *s = suite_create("GstKinesisVideoSinkPlugin");
    TCase *tc = tcase_create("AllStateChangeTests");
    accessKey = getenv(ACCESS_KEY_ENV_VAR);
    secretKey = getenv(SECRET_KEY_ENV_VAR);

    tcase_add_test(tc, kvsproducersinktestplay);
    tcase_add_test(tc, kvsproducersinktestpause);
    tcase_add_test(tc, kvsproducersinktestplaytopausetoplay);
    tcase_add_test(tc, kvsproducersinkteststop);
    tcase_add_test(tc, check_properties_are_passed_correctly);
    tcase_add_test(tc, check_playing_to_paused_and_back_to_playing);
    suite_add_tcase(s, tc);
    return s;
}

GST_CHECK_MAIN(gst_kinesisvideoproducer);