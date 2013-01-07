#include <gst/gst.h>

//#define AUDIO

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline;
    GstElement *source;
    GstElement *convert;
    GstElement *audio_sink;
    GstElement *filter;
    GstElement *video_sink;
} CustomData;

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) {
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements */
    data.source = gst_element_factory_make ("uridecodebin", "source");
    data.convert = gst_element_factory_make ("audioconvert", "convert");
    data.audio_sink = gst_element_factory_make ("autoaudiosink", "audiosink");
    data.filter= gst_element_factory_make ("ffmpegcolorspace", "filter");
    data.video_sink = gst_element_factory_make ("autovideosink", "videosink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("test-pipeline");

    if (!data.pipeline || !data.source || !data.convert || !data.audio_sink || !data.filter || !data.video_sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

#ifdef AUDIO
    /* Build the pipeline. Note that we are NOT linking the source at this
     * point. We will do it later. */
    //gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.convert, data.sink, data.filter, data.video_sink, NULL);
    gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.convert, data.audio_sink, NULL);
    if (!gst_element_link (data.convert, data.audio_sink)) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
#else
    gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.filter, data.video_sink, NULL);
    if (!gst_element_link (data.filter, data.video_sink)) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
#endif

    /* Set the URI to play */
    g_object_set (data.source, "uri", "http://docs.gstreamer.com/media/sintel_trailer-480p.webm", NULL);

    /* Connect to the pad-added signal */
    g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);

    /* Start playing */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus (data.pipeline);
    do {
        msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
                GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

        /* Parse message */
        if (msg != NULL) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    terminate = TRUE;
                    break;

                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    terminate = TRUE;
                    break;

                case GST_MESSAGE_STATE_CHANGED:
                    /* We are only interested in state-changed messages from the pipeline */
                    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                        g_print ("Pipeline state changed from %s to %s:\n",
                                gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                    }
                    break;

                default:
                    /* We should not reach here */
                    g_printerr ("Unexpected message received.\n");
                    break;
            }
            gst_message_unref (msg);
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (data.pipeline);
    return 0;
}

/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
#ifdef AUDIO
    GstPad *sink_pad = gst_element_get_static_pad (data->convert, "sink");
#else
    GstPad *video_sink_pad = gst_element_get_static_pad (data->filter, "sink");
#endif
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

#ifdef AUDIO 
    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
        g_print ("audio,  We are already linked. Ignoring.\n");
        goto exit;
    }
#else
    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (video_sink_pad)) {
        g_print ("video,  We are already linked. Ignoring.\n");
        goto exit;
    }
#endif

#if 1
    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    g_print ("  It has type '%s'\n", new_pad_type);

    if (g_str_has_prefix (new_pad_type, "audio/x-raw")) {
#ifdef AUDIO
        ret = gst_pad_link (new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED (ret)) {
            g_print ("Audio,  Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print ("Audio,  Link succeeded (type '%s').\n", new_pad_type);
        }
#endif
    }else if(g_str_has_prefix(new_pad_type, "video/x-raw-yuv")){
#if 1
        ret = gst_pad_link (new_pad, video_sink_pad);
        if (GST_PAD_LINK_FAILED (ret)) {
            g_print ("Video,  Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print ("Video,  Link succeeded (type '%s').\n", new_pad_type);
        }
#endif
        goto exit;
    }
#endif

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);

    /* Unreference the sink pad */
#ifdef AUDIO
    gst_object_unref (sink_pad);
#else
    gst_object_unref (video_sink_pad);
#endif
}
