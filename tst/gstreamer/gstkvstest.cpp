#include "gstkvssink.h" //import this first, or will cause build error on Mac
#include "Util/KvsSinkResolution.h"
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
static char const *sessionToken;

static GstElement *
setup_kinesisvideoproducersink(void)
{
    cout << "setup_kinesisvideoproducersink() start" << endl;

    GstElement *kinesisvideoproducersink;
    kinesisvideoproducersink = gst_check_setup_element ("kvssink");
    fail_unless(kinesisvideoproducersink != nullptr, "Failed to create kvssink element (is GST_PLUGIN_PATH set?)");

    g_object_set(G_OBJECT (kinesisvideoproducersink),
                 "access-key", accessKey,
                 "secret-key", secretKey,
                 "session-token", sessionToken,
                 NULL);

    // https://gitlab.freedesktop.org/gstreamer/gst-docs/-/issues/91
    // Use gst_element_request_pad_simple in newer GStreamer versions
    GstPad *sinkpad = gst_element_get_request_pad(kinesisvideoproducersink, "video_%u");
    fail_unless(sinkpad != nullptr, "Failed to request video pad");
    gst_object_unref(sinkpad);

    cout << "setup_kinesisvideoproducersink() end" << endl;
    return kinesisvideoproducersink;
}

static void
cleanup_kinesisvideoproducersink(GstElement * kinesisvideoproducersink)
{
    cout << "cleanup_kinesisvideoproducersink() start" << endl;

    GstPad *sinkpad = gst_element_get_static_pad(kinesisvideoproducersink, "video_0");
    if (sinkpad) {
        gst_element_release_request_pad(kinesisvideoproducersink, sinkpad);
        gst_object_unref(sinkpad);
    }

    gst_check_teardown_element (kinesisvideoproducersink);

    fail_unless_equals_int(STATUS_SUCCESS, RESET_INSTRUMENTED_ALLOCATORS());

    cout << "cleanup_kinesisvideoproducersink() end" << endl;
}

GST_START_TEST(kvsproducersinktestplayandstop)
    {
        GstElement *pElement = setup_kinesisvideoproducersink();

        // Set up source pad
        GstPad *srcpad = gst_check_setup_src_pad_by_name(pElement, &srctemplate, "video_0");
        fail_unless(srcpad != nullptr, "Failed to setup source pad");
        gst_pad_set_active(srcpad, TRUE);

        // Set to PLAYING state (NULL -> PLAYING)
        fail_unless_equals_int(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pElement, GST_STATE_PLAYING));

        // Set back to NULL state (PLAYING -> NULL)
        fail_unless_equals_int(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pElement, GST_STATE_NULL));

        gst_pad_set_active(srcpad, FALSE);
        cleanup_kinesisvideoproducersink(pElement);
    }
GST_END_TEST;

GST_START_TEST(kvsproducersinktestplaytopausetoplay)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        GstPad *srcpad;

        srcpad = gst_check_setup_src_pad_by_name (pElement, &srctemplate, "video_0");

        gst_pad_set_active (srcpad, TRUE);

        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_NULL_TO_READY), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_READY_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        // dummy cpd. Also the caps event need to be sent after the kvssink is in paused state in order for it to received it
        gst_pad_push_event(srcpad, gst_event_new_caps (gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au,codec_data=abc")));
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PAUSED_TO_PLAYING), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PLAYING_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);

        gst_pad_set_active (srcpad, FALSE);

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

GST_START_TEST(check_kvssink_null_stream_name_fails_init_play)
    {
        GstElement *kinesisvideoproducersink;

        // Setup
        kinesisvideoproducersink = gst_check_setup_element ("kvssink");
        fail_unless(kinesisvideoproducersink != nullptr, "Failed to create kvssink element");

        // Set stream-name to null. Only possible programmatically, not through gst-parse-launch-1.0 command
        g_object_set(G_OBJECT (kinesisvideoproducersink),
            "stream-name", nullptr,
            NULL);

        // Test - Initialization of the client & stream occurs here
        fail_unless_equals_int(GST_STATE_CHANGE_FAILURE, gst_element_set_state(kinesisvideoproducersink, GST_STATE_PLAYING));

        // Teardown
        fail_unless_equals_int(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(kinesisvideoproducersink, GST_STATE_NULL));
        gst_check_teardown_element(kinesisvideoproducersink);
    }
GST_END_TEST;

GST_START_TEST(check_kvssink_no_pads_fails_to_play)
    {
        GstElement *kinesisvideoproducersink;

        // Setup
        kinesisvideoproducersink = gst_check_setup_element ("kvssink");
        fail_unless(kinesisvideoproducersink != nullptr, "Failed to create kvssink element");

        g_object_set(G_OBJECT (kinesisvideoproducersink),
            "stream-name", "test-stream",
            NULL);

        // Test - Initialization of the client & stream occurs here
        // Expecting kvssink->content_type == nullptr since no pads attached
        fail_unless_equals_int(GST_STATE_CHANGE_FAILURE, gst_element_set_state(kinesisvideoproducersink, GST_STATE_PLAYING));

        // Teardown
        fail_unless_equals_int(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(kinesisvideoproducersink, GST_STATE_NULL));
        gst_check_teardown_element(kinesisvideoproducersink);
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
        g_free(str_val);
        g_object_get(G_OBJECT (pElement), "retention-period", &uint_val, NULL);
        assert_equals_uint64(TEST_RETENTION_PERIOD, uint_val);
        g_object_get(G_OBJECT (pElement), "streaming-type", &uint_val, NULL);
        assert_equals_uint64(TEST_STREAMING_TYPE, uint_val);
        g_object_get(G_OBJECT (pElement), "content-type", &str_val, NULL);
        assert_equals_string(TEST_CONTENT_TYPE.c_str(), str_val);
        g_free(str_val);
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
        g_free(str_val);
        g_object_get(G_OBJECT (pElement), "track-name", &str_val, NULL);
        assert_equals_string(TEST_TRACK_NAME.c_str(), str_val);
        g_free(str_val);
        g_object_get(G_OBJECT (pElement), "access-key", &str_val, NULL);
        assert_equals_string(TEST_ACCESS_KEY.c_str(), str_val);
        g_free(str_val);
        g_object_get(G_OBJECT (pElement), "secret-key", &str_val, NULL);
        assert_equals_string(TEST_SECRET_KEY.c_str(), str_val);
        g_free(str_val);
        g_object_get(G_OBJECT (pElement), "rotation-period", &uint_val, NULL);
        assert_equals_uint64(TEST_ROTATION_PERIOD, uint_val);
        g_object_get(G_OBJECT (pElement), "log-config", &str_val, NULL);
        assert_equals_string(TEST_LOG_CONFIG.c_str(), str_val);
        g_free(str_val);

        cleanup_kinesisvideoproducersink(pElement);

    }
GST_END_TEST;

// Exercises the region precedence logic directly via the pure resolveRegion helper, so the full
// property/env/ignore-flag matrix is verified deterministically without mutating the process
// environment (not portable) or making network calls. Each row is one scenario; env_region == nullptr
// models an unset AWS_DEFAULT_REGION, and a non-null value models the env var being present.
//
// The aws-region property carries the us-west-2 default (DEFAULT_REGION), so a case where the env var
// is unset and the property is left at that default is what produces the documented "defaults to
// us-west-2" behavior (region lookup step 3).
GST_START_TEST(check_region_env_override_resolution)
    {
        struct RegionCase {
            const char *description;
            const char *property_region; // value of the aws-region property
            const char *env_region;      // snapshot of AWS_DEFAULT_REGION (nullptr == unset)
            bool ignore_env;             // value of the ignore-region-env property
            const char *expected;        // region resolveRegion should return
        };

        const RegionCase cases[] = {
            // env set + property set, default flag -> env wins
            {"env overrides property by default",        "us-east-1", "us-west-1", false, "us-west-1"},
            // property set, env unset, default flag     -> property used
            {"property used when env unset",             "us-east-1", nullptr,     false, "us-east-1"},
            // neither explicitly set (property at us-west-2 default), env unset -> us-west-2 fallback
            {"falls back to us-west-2 when neither set",  "us-west-2", nullptr,     false, "us-west-2"},
            // env set + property set, ignore flag       -> property wins despite env
            {"ignore flag makes property win over env",  "us-east-1", "us-west-1", true,  "us-east-1"},
            // property set, env unset, ignore flag      -> property used
            {"ignore flag with env unset uses property", "us-east-1", nullptr,     true,  "us-east-1"},
            // neither explicitly set, ignore flag       -> us-west-2 fallback still holds
            {"ignore flag preserves us-west-2 fallback", "us-west-2", nullptr,     true,  "us-west-2"},
        };

        for (const RegionCase &c : cases) {
            const char *env_display = c.env_region ? c.env_region : "(unset)";
            cout << "resolveRegion case: " << c.description
                 << " [property=" << c.property_region
                 << ", env=" << env_display
                 << ", ignore_env=" << (c.ignore_env ? "true" : "false")
                 << ", expected=" << c.expected << "]" << endl;

            string resolved = kvs_sink_util::resolveRegion(c.property_region, c.env_region, c.ignore_env);
            fail_unless(resolved == c.expected,
                        "resolveRegion failed for case \"%s\": property=%s, env=%s, ignore_env=%s -> expected \"%s\" but got \"%s\"",
                        c.description, c.property_region, env_display,
                        c.ignore_env ? "true" : "false", c.expected, resolved.c_str());
        }
    }
GST_END_TEST;

// Exercises the credential env precedence logic directly via the pure shouldUseEnvVar helper.
// Each row is one scenario; env_value == nullptr models an unset credential env var (e.g.
// AWS_ACCESS_KEY_ID), and a non-null value models it being present. A true result means the env var
// is consulted; false means the corresponding property wins.
GST_START_TEST(check_credentials_env_override_resolution)
    {
        struct CredentialCase {
            const char *description;
            bool ignore_env;         // value of the ignore-credentials-env property
            const char *env_value;   // snapshot of the credential env var (nullptr == unset)
            bool expect_use_env;     // whether the env var should be consulted
        };

        const CredentialCase cases[] = {
            // env set, default flag  -> env is consulted (existing behavior)
            {"env used when present by default", false, "AKIDEXAMPLE", true},
            // env unset, default flag -> nothing to consult, property/other providers used
            {"env ignored when unset",           false, nullptr,       false},
            // env set, ignore flag   -> env never consulted, property wins
            {"ignore flag skips present env",    true,  "AKIDEXAMPLE", false},
            // env unset, ignore flag -> still not consulted
            {"ignore flag with env unset",       true,  nullptr,       false},
        };

        for (const CredentialCase &c : cases) {
            const char *env_display = c.env_value ? c.env_value : "(unset)";
            cout << "shouldUseEnvVar case: " << c.description
                 << " [ignore_env=" << (c.ignore_env ? "true" : "false")
                 << ", env=" << env_display
                 << ", expect_use_env=" << (c.expect_use_env ? "true" : "false") << "]" << endl;

            bool use_env = kvs_sink_util::shouldUseEnvVar(c.ignore_env, c.env_value);
            fail_unless(use_env == c.expect_use_env,
                        "shouldUseEnvVar failed for case \"%s\": ignore_env=%s, env=%s -> expected %s but got %s",
                        c.description, c.ignore_env ? "true" : "false", env_display,
                        c.expect_use_env ? "true" : "false", use_env ? "true" : "false");
        }
    }
GST_END_TEST;

GST_START_TEST(check_playing_to_paused_and_back_to_playing)
    {
        GstElement *pElement =
                setup_kinesisvideoproducersink();
        GstPad *srcpad;

        srcpad = gst_check_setup_src_pad_by_name (pElement, &srctemplate, "video_0");

        gst_pad_set_active (srcpad, TRUE);

        fail_unless_equals_int(gst_element_set_state(pElement, GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_NULL_TO_READY), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_READY_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        gst_pad_push_event(srcpad, gst_event_new_caps (gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au,codec_data=abc")));
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PAUSED_TO_PLAYING), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PLAYING_TO_PAUSED), GST_STATE_CHANGE_SUCCESS);
        fail_unless_equals_int(gst_element_change_state(pElement, GST_STATE_CHANGE_PAUSED_TO_PLAYING), GST_STATE_CHANGE_SUCCESS);

        gst_pad_set_active (srcpad, FALSE);

        cleanup_kinesisvideoproducersink(pElement);
    }
GST_END_TEST;

GST_START_TEST(test_check_credentials)
    {
        CHAR missingVars[128] = {0};
        SIZE_T missingVarsLen = 0;

        if (accessKey[0] == '\0') {
            SNPRINTF(missingVars + missingVarsLen, SIZEOF(missingVars) - missingVarsLen, "%s", ACCESS_KEY_ENV_VAR);
            missingVarsLen = STRNLEN(missingVars, SIZEOF(missingVars));
        }

        if (secretKey[0] == '\0') {
            if (missingVarsLen > 0) {
                STRNCAT(missingVars, " and ", SIZEOF(missingVars) - missingVarsLen - 1);
                missingVarsLen = STRNLEN(missingVars, SIZEOF(missingVars));
            }
            STRNCAT(missingVars, SECRET_KEY_ENV_VAR, SIZEOF(missingVars) - missingVarsLen - 1);
            missingVarsLen = STRNLEN(missingVars, SIZEOF(missingVars));
        }

        ck_abort_msg("Required environment variable%s %s %s not set",
                     STRCHR(missingVars, ' ') ? "s" : "",
                     missingVars,
                     STRCHR(missingVars, ' ') ? "are" : "is");
    }
GST_END_TEST;

//Verify all State change events and direct state set events.

Suite *gst_kinesisvideoproducer_suite(void) {
    Suite *s = suite_create("GstKinesisVideoSinkPlugin");
    TCase *tc = tcase_create("AllStateChangeTests");
    tcase_set_timeout(tc, 15);  // 15 second timeout per test

    accessKey = GETENV(ACCESS_KEY_ENV_VAR);
    secretKey = GETENV(SECRET_KEY_ENV_VAR);
    sessionToken = GETENV(SESSION_TOKEN_ENV_VAR);

    accessKey = accessKey ? accessKey : "";
    secretKey = secretKey ? secretKey : "";
    sessionToken = sessionToken ? sessionToken : "";

    // Resolution-logic tests. These exercise the pure precedence helpers only; they don't contact
    // AWS, so they run regardless of whether AWS credentials are present in the environment.
    TCase *tc_resolution = tcase_create("EnvOverrideResolution");
    tcase_add_test(tc_resolution, check_region_env_override_resolution);
    tcase_add_test(tc_resolution, check_credentials_env_override_resolution);
    suite_add_tcase(s, tc_resolution);

    // Check if required environment variables are set
    // Note: Session token can be empty if permanent credentials are used
    if (accessKey[0] == '\0' || secretKey[0] == '\0') {
        TCase *tc_env = tcase_create("Credentials check");
        tcase_add_test_raise_signal(tc_env, test_check_credentials, SIGABRT);
        suite_add_tcase(s, tc_env);

        // Return the suite with only this failing test
        return s;
    }

    tcase_add_test(tc, check_kvssink_null_stream_name_fails_init_play);
    tcase_add_test(tc, check_kvssink_no_pads_fails_to_play);
    tcase_add_test(tc, kvsproducersinktestplayandstop);
    tcase_add_test(tc, kvsproducersinktestplaytopausetoplay);
    tcase_add_test(tc, kvsproducersinkteststop);
    tcase_add_test(tc, check_properties_are_passed_correctly);
    tcase_add_test(tc, check_playing_to_paused_and_back_to_playing);
    suite_add_tcase(s, tc);
    return s;
}

GST_CHECK_MAIN(gst_kinesisvideoproducer);