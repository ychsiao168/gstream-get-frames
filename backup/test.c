//==============================================================================
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------
#define IS_JETSON (1)   // 1: jetson board, 0: other platform

#if IS_JETSON
#define LAUNCH_CMD  "filesrc location=jellyfish-5-mbps-hd-h264.mkv ! matroskademux ! h264parse ! nvv4l2decoder ! video/x-raw(memory:NVMM) ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink"
#define LAUNCH_CMD2 "filesrc location=jellyfish-5-mbps-hd-h264.mkv ! decodebin ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink"
#else
#define LAUNCH_CMD  "filesrc location=jellyfish-5-mbps-hd-h264.mkv ! matroskademux ! h264parse ! avdec_h264 ! videoconvert ! appsink name=mysink"
#define LAUNCH_CMD2 "filesrc location=jellyfish-5-mbps-hd-h264.mkv ! decodebin ! videoconvert ! appsink name=mysink"

#endif
//------------------------------------------------------------------------------
//  Local Data Structures
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Local Prototypes
//------------------------------------------------------------------------------
static GstFlowReturn new_sample(GstAppSink *sink, gpointer user_data);
/*##############################################################################
##  Public Function Implementation                                            ##
##############################################################################*/
//------------------------------------------------------------------------------
//
//
//
//
//------------------------------------------------------------------------------
int main(int argc, const char *argv[])
{
    // vars
    GMainLoop *main_loop = NULL;
    GstPipeline *gst_pipeline = NULL;
    GError *error = NULL;
    GstElement *appsink;
    GstAppSinkCallbacks callbacks = {NULL, NULL, new_sample};

    // init
    gst_init(NULL, NULL);

    // launch pipeline
    gst_pipeline = (GstPipeline *)gst_parse_launch(LAUNCH_CMD, &error);
    gst_element_set_state((GstElement *)gst_pipeline, GST_STATE_PLAYING);

    // appsink
    appsink = gst_bin_get_by_name(GST_BIN(gst_pipeline), "mysink");
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, NULL, NULL);

    // check error
    if (gst_pipeline == NULL)
    {
        g_print("Failed to parse launch: %s\n", error->message);
        return -1;
    }
    if (error)
        g_error_free(error);

    // main loop
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // free
    gst_element_set_state((GstElement *)gst_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(gst_pipeline));
    g_main_loop_unref(main_loop);

    g_print("Done\n");
    return 0;
}

/*##############################################################################
##  Static Function Implementation                                            ##
##############################################################################*/
//------------------------------------------------------------------------------
//
//
//
//
//------------------------------------------------------------------------------
static GstFlowReturn new_sample(GstAppSink *sink, gpointer user_data)
{
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;

    /* Retrieve the buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);

    if (sample)
    {
        buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &map, GST_MAP_READ);
        g_print("frame size=%6lu, data=0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X\n",
                map.size,
                *(guint16 *)&map.data[0], *(guint16 *)&map.data[1], *(guint16 *)&map.data[2], *(guint16 *)&map.data[3],
                *(guint16 *)&map.data[4], *(guint16 *)&map.data[5], *(guint16 *)&map.data[6], *(guint16 *)&map.data[7]);
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}