//==============================================================================
/*
    File Name:      main.c
    Created:        2021/12/14   
    Author:         
    Description:
   
*/
//==============================================================================
//
//
//  Copyright (C) 201X XXX Technology Co. Ltd. All rights reserved.
//
//
//==============================================================================
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <gst/gst.h>
#include <stdio.h>
//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Local Data Structures
//------------------------------------------------------------------------------
typedef struct _CustomData
{
    GstElement *pipeline;
    GstElement *video_source, *ai_process, *dummy_sink;

    guint64 num_samples; /* Number of samples generated so far (for timestamp generation) */

    guint sourceid; /* To control the GSource */

    GMainLoop *main_loop; /* GLib's Main Loop */
    GstBus *bus;          // todo
    GstMessage *msg;      // todo
} CustomData;
//------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Local Prototypes
//------------------------------------------------------------------------------
static GstFlowReturn new_sample(GstElement *sink, CustomData *data);
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
    CustomData data;
    GstStateChangeReturn ret;

    // init GStreamer
    gst_init(NULL, NULL);

    // create elements
    data.video_source = gst_element_factory_make("videotestsrc", "source");
    //data.ai_process = gst_element_factory_make("videotestsrc", "source");
    data.dummy_sink = gst_element_factory_make("autovideosink", "sink");

    // create pipeline
    data.pipeline = gst_pipeline_new("pipeline");

    // check above
    if (!data.video_source || !data.dummy_sink || !data.pipeline)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // build the pipeline
    gst_bin_add_many(GST_BIN(data.pipeline), data.video_source, data.dummy_sink, NULL);
    if (gst_element_link(data.video_source, data.dummy_sink) != TRUE)
    {
        g_printerr("video_source and dummy_sink could not be linked\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    // set element's properties
    g_object_set(data.video_source, "pattern", 0, NULL);

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

    /* Retrieve the buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample)
    {
        /* The only thing we do in this example is print a * to indicate a received buffer */
        g_print("*");
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}
