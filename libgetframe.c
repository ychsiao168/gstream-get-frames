/**
 * @file libgetframe.c
 * @author ychsiao168
 * @brief
 * @version 0.1
 * @date 2021-12-30
 *
 * @copyright Copyright (c) 2021 FourD Technology Co. Ltd. All rights reserved.
 *
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <stdio.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "libgetframe.h"
//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------
#define HAS_NV_DECODER (1)  // 1: on jetson board, 0: general platform
#define SHOW_VIDEO (0)      // 1: show, 0: not show
//------------------------------------------------------------------------------
//  Local Data Structures
//------------------------------------------------------------------------------
typedef struct _CustomData
{
    GMainLoop *main_loop;
    GstPipeline *pipeline;
    GstElement *appsink;
    fp_frame_cb frame_callback;
} CustomData;
//------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------
static CustomData sg_data = {0};
//------------------------------------------------------------------------------
//  Local Prototypes
//------------------------------------------------------------------------------
static void appsink_eos(GstAppSink *appsink, gpointer user_data);
static GstFlowReturn new_sample(GstAppSink *appsink, gpointer user_data);
/*##############################################################################
##  Public Function Implementation                                            ##
##############################################################################*/
/**
 * @brief initialize this library
 *
 * @param url [IN] url to decode frames, can be https://, rtsp://, file:///
 * @param cb [IN] callback function to get each frame
 * @return int 0 for success
 */
int fourd_init_frame(const char* url, fp_frame_cb cb)
{
    // vars
    GError *error = NULL;
    char launch_cmd[512] = {0};
    GstAppSinkCallbacks callbacks = {appsink_eos, NULL, new_sample};

    // init gstreamer
    gst_init(NULL, NULL);

    // save
    sg_data.frame_callback = cb;

    // launch pipeline
#if HAS_NV_DECODER == 1 && SHOW_VIDEO == 0
    snprintf(&launch_cmd[0], sizeof(launch_cmd), "uridecodebin uri=%s ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink", url); // nvidia, show == 0
#elif HAS_NV_DECODER == 1 && SHOW_VIDEO == 1
    snprintf(&launch_cmd[0], sizeof(launch_cmd), "uridecodebin uri=%s ! tee name=t ! queue ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink t. ! queue ! videoconvert ! autovideosink", url); // nvidia, show == 1
#elif HAS_NV_DECODER == 0 && SHOW_VIDEO == 0
    snprintf(&launch_cmd[0], sizeof(launch_cmd), "uridecodebin uri=%s ! videoconvert ! appsink name=mysink", url); // for non-nvidia, show == 0
#elif HAS_NV_DECODER == 0 && SHOW_VIDEO == 1
    snprintf(&launch_cmd[0], sizeof(launch_cmd), "uridecodebin uri=%s ! tee name=t ! queue ! videoconvert ! appsink name=mysink t. ! queue ! videoconvert ! autovideosink", url); // for non-nvidia, show == 1
#endif
    sg_data.pipeline = (GstPipeline *)gst_parse_launch(launch_cmd, &error);
    gst_element_set_state((GstElement *)sg_data.pipeline, GST_STATE_PLAYING);

    // appsink
    sg_data.appsink = gst_bin_get_by_name(GST_BIN(sg_data.pipeline), "mysink");
    gst_app_sink_set_callbacks(GST_APP_SINK(sg_data.appsink), &callbacks, &sg_data, NULL);

    // check error
    if (sg_data.pipeline == NULL)
    {
        g_print("Failed to parse launch: %s\n", error->message);
        return -1;
    }
    if (error)
        g_error_free(error);

    // create main_loop
    sg_data.main_loop = g_main_loop_new(NULL, FALSE);

    return 0;
}

/**
 * @brief start getting frames. note it's blocking.
 *
 */
void fourd_start_frame_loop(void)
{
    g_main_loop_run(sg_data.main_loop);
}

/**
 * @brief de-initialize this library
 *
 */
void fourd_deinit_frame(void)
{
    gst_element_set_state((GstElement *)sg_data.pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(sg_data.pipeline));
    g_main_loop_unref(sg_data.main_loop);
}

/*##############################################################################
##  Static Function Implementation                                            ##
##############################################################################*/
/**
 * @brief internal callback function for appsink. frame_callback is called here.
 *
 * @param sink appsink instance
 * @param user_data pointer to user's custom data
 * @return GstFlowReturn GST_FLOW_OK if got sample or GST_FLOW_ERROR
 */
static GstFlowReturn new_sample(GstAppSink *appsink, gpointer user_data)
{
    CustomData *pdata = (CustomData*)user_data;
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;

    // Retrieve the buffer
    g_signal_emit_by_name(appsink, "pull-sample", &sample);

    if (sample)
    {
        buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        if(pdata->frame_callback)
        {
            pdata->frame_callback(map.data, map.size);
        }
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

/**
 * @brief internal callback function for appsink. it's called when End of Stream.
 *
 * @param sink appsink instance
 * @param user_data pointer to user's custom data
 */
static void appsink_eos(GstAppSink *appsink, gpointer user_data)
{
    CustomData *pdata = (CustomData*)user_data;
    g_main_loop_quit(pdata->main_loop);
}