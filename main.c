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
#include <gst/app/gstappsink.h>
//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------
#define USE_STREAM 1        // 1, 2, 3
#define USE_NV_DECODER (0)

//#if IS_JETSON
//#define LAUNCH_CMD  "filesrc location=jellyfish-5-mbps-hd-h264.mkv ! matroskademux ! h264parse ! nvv4l2decoder ! video/x-raw(memory:NVMM) ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink"

#if USE_STREAM == 1

#if USE_NV_DECODER == 1
#define LAUNCH_CMD "souphttpsrc location=http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4 ! qtdemux name=d d.video_0 ! h264parse ! nvv4l2decoder ! video/x-raw(memory:NVMM) ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink"
#else
#define LAUNCH_CMD "souphttpsrc location=http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4 ! qtdemux name=d d.video_0 ! h264parse ! avdec_h264 ! videoconvert ! appsink name=mysink"
#endif
// $ gst-launch-1.0 souphttpsrc location=http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4 ! qtdemux name=d d.video_0 ! h264parse ! avdec_h264 ! videoconvert ! autovideosink


#elif USE_STREAM == 2

#if USE_NV_DECODER == 1
#define LAUNCH_CMD "souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux name=d d.video_0 ! nvv4l2decoder ! video/x-raw(memory:NVMM) ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink"
#else
#define LAUNCH_CMD "souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux name=d d.video_0 ! avdec_vp8 ! videoconvert ! appsink name=mysink"
// $ gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! matroskademux name=d d.video_0 ! avdec_vp8 ! videoconvert ! autovideosink
#endif

#elif USE_STREAM == 3

#if USE_NV_DECODER == 1
#define LAUNCH_CMD "rtspsrc location=rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mp4 ! rtph264depay ! h264parse ! nvv4l2decoder ! video/x-raw(memory:NVMM) ! nvvidconv ! video/x-raw,format=NV12 ! appsink name=mysink"
#else
#define LAUNCH_CMD "rtspsrc location=rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mp4 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink name=mysink"
// $ gst-launch-1.0 rtspsrc location=rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mp4 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
#endif

#endif
//------------------------------------------------------------------------------
//  Local Data Structures
//------------------------------------------------------------------------------
typedef struct _CustomData
{
    guint64 num_samples;
    GMainLoop *main_loop;
    GstPipeline *pipeline;
    GstElement *appsink;
} CustomData;
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
    CustomData data = {0};
    GError *error = NULL;
    GstAppSinkCallbacks callbacks = {NULL, NULL, new_sample};

    // init
    gst_init(NULL, NULL);

    // launch pipeline
    data.pipeline = (GstPipeline *)gst_parse_launch(LAUNCH_CMD, &error);
    gst_element_set_state((GstElement *)data.pipeline, GST_STATE_PLAYING);

    // appsink
    data.appsink = gst_bin_get_by_name(GST_BIN(data.pipeline), "mysink");
    gst_app_sink_set_callbacks(GST_APP_SINK(data.appsink), &callbacks, &data, NULL);

    // check error
    if (data.pipeline == NULL)
    {
        g_print("Failed to parse launch: %s\n", error->message);
        return -1;
    }
    if (error)
        g_error_free(error);

    // main loop
    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);

    // free
    gst_element_set_state((GstElement *)data.pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(data.pipeline));
    g_main_loop_unref(data.main_loop);

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
    CustomData *pdata = (CustomData*)user_data;
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;

    /* Retrieve the buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);

    if (sample)
    {
        buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        g_print("frame count=%6lu, size=%6lu, data=0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X\n",
                pdata->num_samples,
                map.size,
                *(guint16 *)&map.data[0], *(guint16 *)&map.data[1], *(guint16 *)&map.data[2], *(guint16 *)&map.data[3],
                *(guint16 *)&map.data[4], *(guint16 *)&map.data[5], *(guint16 *)&map.data[6], *(guint16 *)&map.data[7]);
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        pdata->num_samples++;
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}
