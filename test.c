// pipeline:
//      filesrc -> matroskademux -> h264parse ->
//      avdec_h264/nvv4l2decoder -> videoconvert -> tee
//      tee -> autovideosink
//      tee -> appsink
// wget https://jell.yfish.us/media/jellyfish-5-mbps-hd-h264.mkv
//==============================================================================
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <gst/gst.h>
//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------
#define USE_NVV4L2DECODER 1 // 1: nvv4l2decoder, 0: avdec_h264
//------------------------------------------------------------------------------
//  Local Data Structures
//------------------------------------------------------------------------------
typedef struct _CustomData
{
    GstElement *pipeline;
    GstElement *video_source, *demuxer;
    GstElement *tee, *video_decoder, *video_convert, *dummy_sink, *app_sink;
    GstElement *video_queue, *app_queue;
    GstElement *h264parse;
    GstBus *bus;
    GstMessage *msg;
    gboolean is_nv_decoder;
} CustomData;
//------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Prototypes
//------------------------------------------------------------------------------
static GstFlowReturn new_sample(GstElement *sink, CustomData *data);
static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data);
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
    CustomData data = {0};
    GstStateChangeReturn ret;

    // init GStreamer
    gst_init(NULL, NULL);

    // create elements
    data.video_source = gst_element_factory_make("filesrc", "source-filesrc");
    data.demuxer = gst_element_factory_make("matroskademux", "demuxer");
    data.h264parse = gst_element_factory_make("h264parse", "h264parse");
#if USE_NVV4L2DECODER == 1
    data.video_decoder = gst_element_factory_make("nvv4l2decoder", "video_decoder");
    if(!data.video_decoder)
    {
        data.video_decoder = gst_element_factory_make("avdec_h264", "video_decoder");
        g_print("avdec_h264 is used\n");
        data.is_nv_decoder = FALSE;
    } else
    {
        g_print("nvv4l2decoder is used\n");
        data.is_nv_decoder = TRUE;
    }
#else
    data.video_decoder = gst_element_factory_make("avdec_h264", "video_decoder");
#endif
    data.tee = gst_element_factory_make("tee", "tee");
    data.video_queue = gst_element_factory_make("queue", "video_queue");
    data.app_queue = gst_element_factory_make("queue", "app_queue");
    data.video_convert = gst_element_factory_make("videoconvert", "videoconvert");
    data.dummy_sink = gst_element_factory_make("autovideosink", "sink"); // autovideosink, fakesink
    data.app_sink = gst_element_factory_make("appsink", "app_sink");

    // create pipeline
    data.pipeline = gst_pipeline_new("pipeline");

    // check above
    if (!data.video_source ||
        !data.demuxer || !data.video_decoder ||
        !data.h264parse ||
        !data.tee || !data.video_queue || !data.app_queue || !data.video_convert || !data.dummy_sink || !data.app_sink || !data.pipeline)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // build the pipeline
    gst_bin_add_many(GST_BIN(data.pipeline), data.video_source, data.demuxer, data.video_decoder,
                     data.h264parse,
                     data.tee,
                     data.video_queue, data.dummy_sink,
                     data.video_convert, data.app_queue, data.app_sink, NULL);

    if (!gst_element_link_many(data.video_source, data.demuxer, NULL) ||
        !gst_element_link_many(data.h264parse, data.video_decoder, data.video_convert, data.tee, NULL) ||
        !gst_element_link_many(data.tee, data.video_queue, data.dummy_sink, NULL) ||
        !gst_element_link_many(data.tee, data.app_queue, data.app_sink, NULL))
    {
        g_printerr("link failed\n");
        gst_object_unref(data.pipeline);
        return -1;
    }


    // set element's properties
    g_object_set(data.video_source, "location", "jellyfish-5-mbps-hd-h264.mkv", NULL);

    // connect to the pad-added signal
    g_signal_connect(data.demuxer, "pad-added", G_CALLBACK (pad_added_handler), &data);

    // configure appsink
    g_object_set(data.app_sink, "emit-signals", TRUE, NULL);
    g_signal_connect(data.app_sink, "new-sample", G_CALLBACK(new_sample), &data);

    // configure nvv4l2decoder
    if(data.is_nv_decoder)
    {
        g_print("set capture-io-mode 2\n");
        g_object_set(data.video_decoder, "capture-io-mode", 2, NULL);
    }

    // start playing
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    // wait until error or EOS
    data.bus = gst_element_get_bus(data.pipeline);
    data.msg = gst_bus_timed_pop_filtered(data.bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    if (data.msg != NULL)
    {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(data.msg))
        {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(data.msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n",
                       GST_OBJECT_NAME(data.msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "None");
            g_clear_error(&err);
            g_free(debug_info);
            break;

        case GST_MESSAGE_EOS:
            g_print("End-of-Stream reached.\n");
            break;

        default:
            g_printerr("Unexpected message received\n");
            break;
        }
    }

    // free resources
    gst_object_unref(data.bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
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
static GstFlowReturn new_sample(GstElement *sink, CustomData *data)
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
            *(guint16*)&map.data[0], *(guint16*)&map.data[1], *(guint16*)&map.data[2], *(guint16*)&map.data[3],
            *(guint16*)&map.data[4], *(guint16*)&map.data[5], *(guint16*)&map.data[6], *(guint16*)&map.data[7]
        );
        gst_buffer_unmap (buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data)
{
    GstPad *sink_pad = gst_element_get_static_pad(data->h264parse, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked(sink_pad))
    {
        g_print("We are already linked. Ignoring.\n");
        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);
    if (!g_str_has_prefix(new_pad_type, "video/x-h264"))
    {
        g_print("It has type '%s' which is not video. Ignoring.\n", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link(new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret))
    {
        g_print("Type is '%s' but link failed.\n", new_pad_type);
    }
    else
    {
        g_print("Link succeeded (type '%s').\n", new_pad_type);
    }

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref(new_pad_caps);

    /* Unreference the sink pad */
    gst_object_unref(sink_pad);
}