/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*
*     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <glib-unix.h>
#include <gst/gst.h>
#include <camera/CameraMetadata.h>
#include <camera/VendorTagDescriptor.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <log/log.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include <pthread.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TeledyneInfo"

// static pthread_t                        triggerThread;
// #define trigger_SIG_EXIT SIGUSR1



#define E2V_NODE_PATH      "/sys/devices/platform/soc/ac50000.qcom,cci/ac50000.qcom,cci:qcom,cam-sensor3/e2v_node"


#define ANGORA_LOG_WARNING(fmt, arg...) ALOGW("[%s][%d] " fmt , __FUNCTION__, __LINE__, ##arg)
#define ANGORA_LOG_ERROR(fmt, arg...) ALOGE("[%s][%d] " fmt , __FUNCTION__, __LINE__, ##arg)
#define ANGORA_LOG_INFO(fmt, arg...) ALOGI("[%s][%d] " fmt , __FUNCTION__, __LINE__, ##arg)
// #define ANGORA_LOG_DEBUG(fmt, arg...) ALOGD_IF((uvcsink_log_level > 0), "[%s][%d] " fmt , __FUNCTION__, __LINE__, ##arg)


// #define TELEDYNE_PIPELINE_STAR_OPTION   "s"
// #define TELEDYNE_PIPELINE_STOP_OPTION   "p"
#define TELEDYNE_WRITEDATA_OPTION       "w"
#define TELEDYNE_READDATA_OPTION        "r"
#define TELEDYNE_DUMP_RAW_OPTION        "d"


#define HASH_LINE  "##################################################"
#define EQUAL_LINE "=================================================="

#define APPEND_MENU_HEADER(string) \
  g_string_append_printf (string, "\n\n%.*s MENU %.*s\n\n", \
      37, HASH_LINE, 37, HASH_LINE);

#define APPEND_CONTROLS_SECTION(string) \
  g_string_append_printf (string, " %.*s Teledyne Parameter Controls %.*s\n", \
      30, EQUAL_LINE, 30, EQUAL_LINE);



typedef struct _TeledyneOps TeledyneOps;

/// ML Auto Framing related command line options.
struct _TeledyneOps
{
  gboolean  start;
  gboolean  stop;
  gint      regAddr;
  gint      regData;
  gint     rawNum;
  gboolean dumpEnable;
};

static TeledyneOps Teledyneops = {
  TRUE, FALSE, 0x13, 0x1, 0, FALSE
};


//#define GST_CAMERA_PIPELINE "qtiqmmfsrc name=camera camera=0 ! "\
//"video/x-bayer,format=\(string\)mono,bpp=\(string\)10,width=1920,height=1080,framerate=60/1 "\
//"! queue ! appsink name=sink emit-signals=true sync=false async=true"

#define GST_CAMERA_PIPELINE "qtiqmmfsrc name=camera camera=0 ! "\
"video/x-bayer,format=\(string\)mono,bpp=\(string\)10,width=1920,height=1080,framerate=60/1 "\
"! queue ! multifilesink name=sink location=\"/data/misc/camera/frame%d.raw\" sync=true async=false max-files=3"


#define TERMINATE_MESSAGE      "APP_TERMINATE_MSG"
#define PIPELINE_STATE_MESSAGE "APP_PIPELINE_STATE_MSG"
#define PIPELINE_EOS_MESSAGE   "APP_PIPELINE_EOS_MSG"
#define STDIN_MESSAGE          "APP_STDIN_MSG"

#define GST_APP_CONTEXT_CAST(obj)           ((GstAppContext*)(obj))

typedef struct _GstAppContext GstAppContext;

struct _GstAppContext
{
  // Main application event loop.
  GMainLoop   *mloop;

  // GStreamer pipeline instance.
  GstElement  *pipeline;

  // Asynchronous queue thread communication.
  GAsyncQueue *messages;
};

/// Command line option variables.
static gboolean eos_on_shutdown = TRUE;

static GstAppContext *
gst_app_context_new ()
{
  GstAppContext *ctx = g_new0 (GstAppContext, 1);

  ctx->messages = g_async_queue_new_full ((GDestroyNotify) gst_structure_free);
  ctx->pipeline = NULL;
  ctx->mloop = NULL;

  return ctx;
}

static void
gst_app_context_free (GstAppContext * ctx)
{
  if (ctx->mloop != NULL)
    g_main_loop_unref (ctx->mloop);

  if (ctx->pipeline != NULL)
    gst_object_unref (ctx->pipeline);

  g_async_queue_unref (ctx->messages);
  g_free (ctx);

  return;
}

static void
gst_sample_release (GstSample * sample)
{
    gst_sample_unref (sample);
#if GST_VERSION_MAJOR >= 1 && GST_VERSION_MINOR > 14
    gst_sample_set_buffer (sample, NULL);
#endif
}

static gboolean
wait_stdin_message (GAsyncQueue * messages, gchar** input)
{
  GstStructure *message = NULL;

  // Cleanup input variable from previous uses.
  g_free (*input);
  *input = NULL;

  // Wait for either a STDIN or TERMINATE message.
  while ((message = (GstStructure *)g_async_queue_pop(messages)) != NULL) {
    if (gst_structure_has_name (message, TERMINATE_MESSAGE)) {
      gst_structure_free (message);
      return FALSE;
    }

    if (gst_structure_has_name (message, STDIN_MESSAGE)) {
      *input = g_strdup (gst_structure_get_string (message, "input"));
      break;
    }

    gst_structure_free (message);
  }
  gst_structure_free (message);

  return TRUE;
}


static gboolean
extract_integer_value (const gchar * input, gint64 min, gint64 max, gint64 * value)
{
  // Convert string to integer value.
  gint64 newvalue = g_ascii_strtoll (input, NULL, 0);

  if (newvalue < min && newvalue > max) {
    g_printerr ("\nValue is outside range!\n");
    return FALSE;
  }

  *value = newvalue;
  return TRUE;
}

static gboolean
extract_double_value (const gchar * input, gdouble min, gdouble max, gdouble * value)
{
  // Convert string to double value.
  gdouble newvalue = g_ascii_strtod (input, NULL);

  if (newvalue < min && newvalue > max) {
    g_printerr ("\nValue is outside range!\n");
    return FALSE;
  }

  *value = newvalue;
  return TRUE;
}


static guint
get_vendor_tag_by_name (const gchar * section, const gchar * name)
{
  ::android::sp<::android::VendorTagDescriptor> vtags;
  ::android::status_t status = 0;
  guint tag_id = 0;

  vtags = ::android::VendorTagDescriptor::getGlobalVendorTagDescriptor();
  if (vtags.get() == NULL) {
    GST_WARNING ("Failed to retrieve Global Vendor Tag Descriptor!");
    return 0;
  }

  status = vtags->lookupTag(::android::String8(name),
      ::android::String8(section), &tag_id);
  if (status != 0) {
    GST_WARNING ("Unable to locate tag for '%s', section '%s'!", name, section);
    return 0;
  }

  return tag_id;
}

static gboolean
handle_interrupt_signal (gpointer userdata)
{
  GstAppContext *appctx = GST_APP_CONTEXT_CAST (userdata);
  GstState state = GST_STATE_VOID_PENDING;
  static gboolean waiting_eos = FALSE;

  // Signal menu thread to quit.
  g_async_queue_push (appctx->messages,
      gst_structure_new_empty (TERMINATE_MESSAGE));

  // Get the current state of the pipeline.
  gst_element_get_state (appctx->pipeline, &state, NULL, 0);

  if (eos_on_shutdown && !waiting_eos && (state == GST_STATE_PLAYING)) {
    g_print ("\nEOS enabled -- Sending EOS on the pipeline\n");

    gst_element_post_message (GST_ELEMENT (appctx->pipeline),
        gst_message_new_custom (GST_MESSAGE_EOS, GST_OBJECT (appctx->pipeline),
            gst_structure_new_empty ("GST_PIPELINE_INTERRUPT")));

    g_print ("\nWaiting for EOS ...\n");
    waiting_eos = TRUE;
  } else if (eos_on_shutdown && waiting_eos) {
    g_print ("\nInterrupt while waiting for EOS - quit main loop...\n");

    gst_element_set_state (appctx->pipeline, GST_STATE_NULL);
    g_main_loop_quit (appctx->mloop);

    waiting_eos = FALSE;
  } else {
    g_print ("\n\nReceived an interrupt signal, stopping pipeline ...\n");
    gst_element_set_state (appctx->pipeline, GST_STATE_NULL);
    g_main_loop_quit (appctx->mloop);
  }
  ANGORA_LOG_INFO ("end ...");

  return TRUE;
}

static gboolean
handle_bus_message (GstBus * bus, GstMessage * message, gpointer userdata)
{
  GstAppContext *appctx = GST_APP_CONTEXT_CAST (userdata);
  static GstState target_state = GST_STATE_VOID_PENDING;
  static gboolean in_progress = FALSE, buffering = FALSE;

  if(message == NULL)
     ANGORA_LOG_INFO("[GST_BUS] %s bus messageName is NULL\n", __FUNCTION__);
  else
     ANGORA_LOG_INFO("[GST_BUS] %s bus messageName is %s\n", __FUNCTION__, GST_MESSAGE_SRC_NAME(message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *debug = NULL;

      g_print ("\n\n");
      gst_message_parse_error (message, &error, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), error, debug);

      g_free (debug);
      g_error_free (error);

      g_print ("\nSetting pipeline to NULL ...\n");
      gst_element_set_state (appctx->pipeline, GST_STATE_NULL);

      g_async_queue_push (appctx->messages,
          gst_structure_new_empty (TERMINATE_MESSAGE));
      g_main_loop_quit (appctx->mloop);
    }
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *error = NULL;
      gchar *debug = NULL;

      g_print ("\n\n");
      gst_message_parse_warning (message, &error, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), error, debug);

      g_free (debug);
      g_error_free (error);
    }
      break;
    case GST_MESSAGE_EOS:
      g_print ("\nReceived End-of-Stream from '%s' ...\n",
          GST_MESSAGE_SRC_NAME (message));

      g_async_queue_push (appctx->messages,
          gst_structure_new_empty (PIPELINE_EOS_MESSAGE));

      // Stop pipeline and quit main loop in case user interrupt has been sent.
      gst_element_set_state (appctx->pipeline, GST_STATE_NULL);
      g_main_loop_quit (appctx->mloop);
      break;
    case GST_MESSAGE_REQUEST_STATE:
    {
      gchar *name = gst_object_get_path_string (GST_MESSAGE_SRC (message));
      GstState state;

      gst_message_parse_request_state (message, &state);
      g_print ("\nSetting pipeline state to %s as requested by %s...\n",
          gst_element_state_get_name (state), name);

      gst_element_set_state (appctx->pipeline, state);
      target_state = state;

      g_free (name);
    }
      break;
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldstate, newstate, pending;
	  ANGORA_LOG_INFO("state changed enter ...");

      // Handle state changes only for the pipeline.
      if (GST_MESSAGE_SRC (message) != GST_OBJECT_CAST (appctx->pipeline))
        break;

      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
      g_print ("\nPipeline state changed from %s to %s, pending: %s\n",
          gst_element_state_get_name (oldstate),
          gst_element_state_get_name (newstate),
          gst_element_state_get_name (pending));

      g_async_queue_push (appctx->messages, gst_structure_new (
          PIPELINE_STATE_MESSAGE, "new", G_TYPE_UINT, newstate,
          "pending", G_TYPE_UINT, pending, NULL));
     }
	   ANGORA_LOG_INFO("state changed end ...");
      break;
    default:
		ANGORA_LOG_INFO("defaut end ...");
      break;
  }

  return TRUE;
}

static GstFlowReturn
new_sample (GstElement * element, gpointer userdata)
{
  GstSample *sample = NULL;
  GstBuffer *buffer = NULL;
  guint64 timestamp = 0;
  GstMapInfo info;
  char filename[128];
  int ret;

  // New sample is available, retrieve the buffer from the sink.
  g_signal_emit_by_name (element, "pull-sample", &sample);

  if (sample == NULL) {
    g_printerr ("ERROR: Pulled sample is NULL!\n");
    return GST_FLOW_ERROR;
  }

  if ((buffer = gst_sample_get_buffer (sample)) == NULL) {
    g_printerr ("ERROR: Pulled buffer is NULL!\n");
    gst_sample_release (sample);
    return GST_FLOW_ERROR;
  }

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ)) {
    g_printerr ("ERROR: Failed to map the pulled buffer!\n");
    gst_sample_release (sample);
    return GST_FLOW_ERROR;
  }

  // Extract the original camera timestamp from GstBuffer OFFSET_END field
  timestamp = GST_BUFFER_OFFSET_END (buffer);
  //ANGORA_LOG_INFO ("Camera timestamp: %" G_GUINT64_FORMAT "\n", timestamp);

// add dump file for single trigger mode, the continus mode will affect perfomance
if(Teledyneops.dumpEnable) {
  ANGORA_LOG_INFO ("raw data dump frameNum=%d ...\n",Teledyneops.rawNum);
  sprintf(filename, "/data/misc/camera/output%d.raw", Teledyneops.rawNum++);
  FILE *output_raw = fopen(filename, "wb");

  if(!output_raw) {
	ANGORA_LOG_INFO("could not open file\n");
	}

  if(output_raw)
	  ret = fwrite(info.data, 1, info.size, output_raw);

  if(ret < 1)
     ANGORA_LOG_INFO("could not write file\n");

  if(output_raw)
     fclose(output_raw);
}
  gst_buffer_unmap (buffer, &info);
  gst_sample_release (sample);

  return GST_FLOW_OK;
}

static void
result_metadata (GstElement * element, gpointer metadata, gpointer userdata)
{
  ::android::CameraMetadata *meta = (::android::CameraMetadata*) metadata;
  guint tag_id = 0;

  if (meta == nullptr)
    return;

//  g_print ("\nResult metadata ... entries - %ld\n", meta->entryCount());
//  ANGORA_LOG_INFO ("enter ...");

  // Exposure time
  if (meta->exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
    gint64 exptime = meta->find(ANDROID_SENSOR_EXPOSURE_TIME).data.i64[0];
   //g_print ("Result Sensor Exposure Time - %" G_GINT64_FORMAT "\n", exptime);
  }

  // Sensor Timestamp
  if (meta->exists(ANDROID_SENSOR_TIMESTAMP)) {
    gint64 timestamp = meta->find(ANDROID_SENSOR_TIMESTAMP).data.i64[0];
    //ANGORA_LOG_INFO ("Result timestamp - %" G_GINT64_FORMAT "\n", timestamp);
  }

  // AE mode for manual control
  if (meta->exists(ANDROID_CONTROL_AE_MODE)) {
    gint mode = meta->find(ANDROID_CONTROL_AE_MODE).data.u8[0];
  //  g_print ("Result Auto Exposure Mode - %d\n", mode);
  }

  // AE Target
  if (meta->exists(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)) {
    gint compensation =
      meta->find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION).data.i32[0];
  //  g_print ("Result Exposure Compensation - %d\n", compensation);
  }

  // AE Lock
  if (meta->exists(ANDROID_CONTROL_AE_LOCK)) {
    gint lock = meta->find(ANDROID_CONTROL_AE_LOCK).data.u8[0];
  //  g_print ("Result Exposure Lock - %s\n", (lock > 0) ? "ON" : "OFF");
  }

  // sensor analog + digital gain
  if (meta->exists(ANDROID_SENSOR_SENSITIVITY)) {
    gint32 sensitivity = meta->find(ANDROID_SENSOR_SENSITIVITY).data.i32[0];
  //  g_print ("Result Sensor Sensitivity - %d\n", sensitivity);
  }

  // Sensor analog gain
  if (meta->exists(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY)) {
    gint32 max = meta->find(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY).data.i32[0];
  //  g_print ("Result Sensor Max Sensitivity - %d\n", max);
  }

  // EV mode
  if (meta->exists(ANDROID_CONTROL_AE_COMPENSATION_RANGE)) {
    gint32 min = meta->find(ANDROID_CONTROL_AE_COMPENSATION_RANGE).data.i32[0];
    gint32 max = meta->find(ANDROID_CONTROL_AE_COMPENSATION_RANGE).data.i32[1];
  //  g_print ("Result AE Compensation Range - %d - %d\n", min, max);
  }

    // EV steps
  if (meta->exists(ANDROID_CONTROL_AE_COMPENSATION_STEP)) {
    gint numerator =
      meta->find(ANDROID_CONTROL_AE_COMPENSATION_STEP).data.r[0].numerator;
    gint denominator =
      meta->find(ANDROID_CONTROL_AE_COMPENSATION_STEP).data.r[0].denominator;
 //   g_print ("Result AE Compensation Step - %d/%d\n", numerator, denominator);
  }

  // Sensor Read Result
  gboolean result = 0;
  tag_id = get_vendor_tag_by_name (
      "org.codeaurora.qcamera3.sensorreadoutput", "SensorReadResult");
  if (meta->exists(tag_id)) {
    result = meta->find(tag_id).data.u8[0];
   //g_print ("Sensor Read SensorReadResult Result: %d\n", result);
  }

  if (result) {
    // Sensor Read Output
    tag_id = get_vendor_tag_by_name (
        "org.codeaurora.qcamera3.sensorreadoutput", "SensorReadOutput");
    if (meta->exists(tag_id)) {
      guint value =
          (meta->find(tag_id).data.u8[0]) | (meta->find(tag_id).data.u8[1] << 8);
     // g_print ("Sensor Read Output: %d\n", value);
    }
  }
}

static void
urgent_metadata (GstElement * element, gpointer metadata, gpointer userdata)
{
  ::android::CameraMetadata *meta = (::android::CameraMetadata*) metadata;

  if (meta == nullptr)
    return;

  g_print ("\nUrgent metadata ... entries - %ld\n", meta->entryCount());

  // AWB Mode
  if (meta->exists(ANDROID_CONTROL_AWB_MODE)) {
    gint8 mode = meta->find(ANDROID_CONTROL_AWB_MODE).data.u8[0];
    g_print ("Urgent AWB Mode - %d\n", mode);
  }

  // AWB State
  if (meta->exists(ANDROID_CONTROL_AWB_STATE)) {
    gint8 state = meta->find(ANDROID_CONTROL_AWB_STATE).data.u8[0];
    g_print ("Urgent AWB state - %d\n", state);
  }

  // AF Mode
  if (meta->exists(ANDROID_CONTROL_AF_MODE)) {
    gint8 mode = meta->find(ANDROID_CONTROL_AF_MODE).data.u8[0];
    g_print ("Urgent AF mode - %d\n", mode);
  }

  // AF State
  if (meta->exists(ANDROID_CONTROL_AF_STATE)) {
    gint8 state = meta->find(ANDROID_CONTROL_AF_STATE).data.u8[0];
    g_print ("Urgent AF state - %d\n", state);
  }

  // AE Mode
  if (meta->exists(ANDROID_CONTROL_AE_MODE)) {
    gint8 mode = meta->find(ANDROID_CONTROL_AE_MODE).data.u8[0];
    g_print ("Urgent AE mode - %d\n", mode);
  }

  // AE State
  if (meta->exists(ANDROID_CONTROL_AE_STATE)) {
    gint8 state = meta->find(ANDROID_CONTROL_AE_STATE).data.u8[0];
    g_print ("Urgent AE state - %d\n", state);
  }
}

static gboolean
wait_pipeline_eos_message (GAsyncQueue * messages)
{
  GstStructure *message = NULL;

  // Wait for either a PIPELINE_EOS or TERMINATE message.
  while ((message = (GstStructure*) g_async_queue_pop (messages)) != NULL) {
    if (gst_structure_has_name (message, TERMINATE_MESSAGE)) {
      gst_structure_free (message);
      return FALSE;
    }

    if (gst_structure_has_name (message, PIPELINE_EOS_MESSAGE))
      break;

    gst_structure_free (message);
  }

  gst_structure_free (message);
  return TRUE;
}

static gboolean
wait_pipeline_state_message (GAsyncQueue * messages, GstState state)
{
  GstStructure *message = NULL;

  // Pipeline does not notify us when changing to NULL state, skip wait.
  if (state == GST_STATE_NULL)
    return TRUE;

  // Wait for either a PIPELINE_STATE or TERMINATE message.
  while ((message = (GstStructure*) g_async_queue_pop (messages)) != NULL) {
    if (gst_structure_has_name (message, TERMINATE_MESSAGE)) {
      gst_structure_free (message);
      return FALSE;
    }

    if (gst_structure_has_name (message, PIPELINE_STATE_MESSAGE)) {
      GstState newstate = GST_STATE_VOID_PENDING;
      gst_structure_get_uint (message, "new", (guint*) &newstate);

      if (newstate == state)
        break;
    }

    gst_structure_free (message);
  }

  gst_structure_free (message);
  return TRUE;
}

static gboolean
update_pipeline_state (GstElement * pipeline, GAsyncQueue * messages,
    GstState state)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
  GstState current, pending;

  // First check current and pending states of the pipeline.
  ret = gst_element_get_state (pipeline, &current, &pending, 0);

  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Failed to retrieve pipeline state!\n");
    return TRUE;
  }

  if (state == current) {
    g_print ("Already in %s state\n", gst_element_state_get_name (state));
    return TRUE;
  } else if (state == pending) {
    g_print ("Pending %s state\n", gst_element_state_get_name (state));
    return TRUE;
  }

  // Check whether to send an EOS event on the pipeline.
  if (eos_on_shutdown &&
      (current == GST_STATE_PLAYING) && (state == GST_STATE_NULL)) {
    g_print ("EOS enabled -- Sending EOS on the pipeline\n");

    if (!gst_element_send_event (pipeline, gst_event_new_eos ())) {
      g_printerr ("Failed to send EOS event!");
      return TRUE;
    }

    if (!wait_pipeline_eos_message (messages))
      return FALSE;
  }

  g_print ("Setting pipeline to %s\n", gst_element_state_get_name (state));
  ret = gst_element_set_state (pipeline, state);

  switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
      g_printerr ("ERROR: Failed to transition to %s state!\n",
          gst_element_state_get_name (state));
      return TRUE;
    case GST_STATE_CHANGE_NO_PREROLL:
      g_print ("Pipeline is live and does not need PREROLL.\n");
      break;
    case GST_STATE_CHANGE_ASYNC:
      g_print ("Pipeline is PREROLLING ...\n");

      ret = gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

      if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Pipeline failed to PREROLL!\n");
        return TRUE;
      }
      break;
    case GST_STATE_CHANGE_SUCCESS:
      g_print ("Pipeline state change was successful\n");
      break;
  }

  if (!wait_pipeline_state_message (messages, state))
    return FALSE;

  return TRUE;
}

static gpointer
work_task (gpointer userdata)
{
  GstAppContext *appctx = GST_APP_CONTEXT_CAST (userdata);
  GstElement *camsrc = NULL;
  ::android::CameraMetadata *smeta = nullptr, *meta = nullptr;
  gboolean success = FALSE;
  ANGORA_LOG_INFO ("enter ...");

  // Transition to READY state in order to initilize the camera.
  if (!update_pipeline_state (appctx->pipeline, appctx->messages, GST_STATE_READY)) {
    g_main_loop_quit (appctx->mloop);
    return NULL;
  }

  // Get a reference to the camera plugin.
  camsrc = gst_bin_get_by_name (GST_BIN (appctx->pipeline), "camera");

  // Get static metadata, containing the camera capabilities.
  g_object_get (G_OBJECT (camsrc), "static-metadata", &smeta, NULL);

  if (smeta == nullptr) {
    g_printerr ("ERROR: Failed to fetch static camera metadata!\n");
    gst_object_unref (camsrc);
    return NULL;
  }

  g_print ("\nGot static-metadata entries - %ld\n", smeta->entryCount());

  // Delete the static metadata, no longer needed.
  delete smeta;

  // Transition to PAUSED state in order to prepare the camera streams.
  if (!update_pipeline_state (appctx->pipeline, appctx->messages, GST_STATE_PAUSED)) {
    gst_object_unref (camsrc);
    g_main_loop_quit (appctx->mloop);
    return NULL;
  }

  // Get video metadata, which will be used for video streams.
  g_object_get (G_OBJECT (camsrc), "video-metadata", &meta, NULL);

  // Change the video streams AWB mode.
  guchar mode = ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT;
  meta->update(ANDROID_CONTROL_AWB_MODE, &mode, 1);

  // Sensor Read Input
  guchar flag = 1;
  guint tag_id = get_vendor_tag_by_name (
      "org.codeaurora.qcamera3.sensorreadinput", "SensorReadFlag");
  meta->update(tag_id, &flag, 1);

  g_object_set (G_OBJECT (camsrc), "video-metadata", meta, NULL);

  // Decrease the reference count to the camera element, no longer needed.
  gst_object_unref (camsrc);

  // Transition to PLAYING state.
  if (!update_pipeline_state (appctx->pipeline, appctx->messages, GST_STATE_PLAYING)) {
    g_main_loop_quit (appctx->mloop);
    return NULL;
  }

  // Run the pipeline for 15 more seconds.
  sleep(15);

  // Stop the pipeline.
 update_pipeline_state (appctx->pipeline, appctx->messages, GST_STATE_NULL);

 g_main_loop_quit (appctx->mloop);
  return NULL;
}


static gboolean
handle_stdin_source (GIOChannel * source, GIOCondition condition,
    gpointer userdata)
{
  GstAppContext *appctx = GST_APP_CONTEXT_CAST (userdata);
  GIOStatus status = G_IO_STATUS_NORMAL;
  gchar *input = NULL;

  do {
    GError *error = NULL;
    status = g_io_channel_read_line (source, &input, NULL, NULL, &error);

    if ((G_IO_STATUS_ERROR == status) && (error != NULL)) {
      g_printerr ("Failed to parse command line options: %s!\n",
           GST_STR_NULL (error->message));
      g_clear_error (&error);
      return FALSE;
    } else if ((G_IO_STATUS_ERROR == status) && (NULL == error)) {
      g_printerr ("Unknown error!\n");
      return FALSE;
    }
  } while (status == G_IO_STATUS_AGAIN);

  // Clear trailing whitespace and newline.
  input = g_strchomp (input);

  // Push stdin string into the inputs queue.
  g_async_queue_push (appctx->messages, gst_structure_new (STDIN_MESSAGE,
      "input", G_TYPE_STRING, input, NULL));
  g_free (input);

  return TRUE;
}

static gboolean
start_pipeline (GstElement * pipeline)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  g_print ("Setting pipeline to PLAYING state ...\n");
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);

  switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
      g_printerr ("ERROR: Failed to transition to PLAYING state!\n");
      return FALSE;
    case GST_STATE_CHANGE_NO_PREROLL:
      g_print ("Pipeline is live and does not need PREROLL.\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      g_print ("Pipeline state change was successful\n");
      break;
  }

  return TRUE;
}

static gboolean
stop_pipeline (GstElement * pipeline)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  g_print ("Setting pipeline to NULL ...\n");
  ret = gst_element_set_state (pipeline, GST_STATE_NULL);

  switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
      g_printerr ("ERROR: Failed to transition to NULL state!\n");
      return FALSE;
    case GST_STATE_CHANGE_NO_PREROLL:
      g_print ("Pipeline is live and does not need PREROLL.\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      g_print ("Pipeline state change was successful\n");
      break;
  }

  return TRUE;
}

static int hexStrToInt(char* hex_str)
{
    return strtoul(hex_str, NULL, 16);
}

static void writeReg(int fd, int reg_addr, int reg_data)
{
    int            ret;
    char           buf[30] = {0};
	ANGORA_LOG_INFO("enter ...");

    ANGORA_LOG_INFO("reg_addr: 0x%x, reg_data: 0x%x\n", reg_addr, reg_data);

    sprintf(buf, "0x%02hx 0x%04hx", reg_addr, reg_data);
    ANGORA_LOG_INFO("buf val: %s\n", buf);

    ret = write(fd, buf, strlen(buf)+1);
    ANGORA_LOG_INFO("return write: %d\n", ret);
}

static void readReg(int fd, int reg_addr)
{
    char buf[50] = {0};
    char read_buf[50] = {0};
    int ret;

    sprintf(buf, "0x%02hx 0x%04hx", reg_addr, 0x9999);

    ret = write(fd, buf, strlen(buf)+1);
    ANGORA_LOG_INFO("return write: %d\n", ret);

    ret = lseek(fd, 0, SEEK_SET);
    ANGORA_LOG_INFO("return lseek: %d\n", ret);
    //write() change the fd pos, need use lseek to make pos point to the start of the file

    ret = read(fd, read_buf, sizeof(read_buf));
    //ANGORA_LOG_INFO("return read: %d, buf: %s\n", ret, read_buf);
	g_print("\nEnter readReg return read: %d, buf: %s\n", ret, read_buf);
	//return read_buf;
}

static gboolean
Teledyne_ops_menu (GAsyncQueue * messages)
{
  GString *options = g_string_new (NULL);
  gchar *input = NULL;
  gint fd;
  gint reg_addr, reg_data;

  APPEND_MENU_HEADER (options);
  ANGORA_LOG_INFO ("enter ...");

  APPEND_CONTROLS_SECTION (options);

  /*
  g_string_append_printf (options, "   (%s) %-35s: %s\n",
      TELEDYNE_PIPELINE_STAR_OPTION, "star pipeline",
      "start camera ");
  g_string_append_printf (options, "   (%s) %-35s: %s\n",
      TELEDYNE_PIPELINE_STOP_OPTION, "stop pipeline",
      "stop camera ");
  */
  g_string_append_printf (options, "   (%s) %-35s: %s\n",
      TELEDYNE_WRITEDATA_OPTION, "Set write data",
      "Set write data value ");

    g_string_append_printf (options, "   (%s) %-35s: %s\n",
      TELEDYNE_READDATA_OPTION, "Set read data",
      "Set read data value ");
  /*
    g_string_append_printf (options, "   (%s) %-35s: %s\n",
      TELEDYNE_DUMP_RAW_OPTION, "Set Dump Raw Data",
      "Set dump raw data enable/disable ");
*/
  g_print ("%s", options->str);
  g_string_free (options, TRUE);

  g_print ("\n\nChoose an option: ");

  // If FALSE is returned termination signal has been issued.
  if (!wait_stdin_message (messages, &input))
    return FALSE;

 if (g_str_equal (input, TELEDYNE_WRITEDATA_OPTION)) {

    g_print ("\nCurrent register address value: 0x%x \n", Teledyneops.regAddr);
    g_print ("\nEnter new value (or press Enter to keep current one): ");

	 fd = open(E2V_NODE_PATH, O_RDWR);
	 if (fd < 0)
	  {
		  ANGORA_LOG_ERROR("open error");
	  }

    if (!wait_stdin_message (messages, &input))
      return FALSE;

    if (!g_str_equal (input, ""))	
	    Teledyneops.regAddr = hexStrToInt(input);
      //extract_double_value (input, 1, 16, &value);

	g_free (input);
	input = NULL;

	 g_print ("\nCurrent register data value: 0x%x \n", Teledyneops.regData);
	 g_print ("\nEnter new value (or press Enter to keep current one): ");

	 if (!wait_stdin_message (messages, &input))
      return FALSE;

    if (!g_str_equal (input, ""))		
      Teledyneops.regData = hexStrToInt(input);
	  //extract_integer_value (input, 1, 4096, &digital_value);

    writeReg(fd, Teledyneops.regAddr, Teledyneops.regData);

	close(fd);

  }else if (g_str_equal (input, TELEDYNE_READDATA_OPTION)) {
    g_print ("\nCurrent register address value: 0x%x \n", Teledyneops.regAddr);
    g_print ("\nEnter new value (or press Enter to keep current one): ");

	 fd = open(E2V_NODE_PATH, O_RDWR);
	 if (fd < 0)
	  {
		  ANGORA_LOG_ERROR("open error");
	  }

    if (!wait_stdin_message (messages, &input))
      return FALSE;

    if (!g_str_equal (input, ""))
	    Teledyneops.regAddr = hexStrToInt(input);
      //extract_double_value (input, 1, 16, &value);

     readReg(fd, Teledyneops.regAddr);
	//g_print ("\nCurrent read register data value: %s\n", readReg(fd, Teledyneops.regAddr));

	close(fd);

  }
  g_free (input);
  input = NULL;
  ANGORA_LOG_INFO ("end ...");

  return TRUE;
}


static gpointer
main_menu(gpointer userdata)
{
  GstAppContext *appctx = GST_APP_CONTEXT_CAST (userdata);
  gboolean active = TRUE;

  while (active) {
    active = Teledyne_ops_menu (appctx->messages);
  }

  return NULL;
}

gint
main (gint argc, gchar *argv[])
{
  GstAppContext *appctx = gst_app_context_new ();
  GstElement *element = NULL;
  GThread *mthread_menu = NULL, *mthread = NULL;
  guint bus_watch_id = 0, intrpt_watch_id = 0;
  GIOChannel *iostdin = NULL;
  guint stdin_watch_id = 0;

  g_set_prgname ("gst-camera-teledyne-example");

  // Initialize GST library.
  gst_init (&argc, &argv);

  {
    GError *error = NULL;

    appctx->pipeline = gst_parse_launch (GST_CAMERA_PIPELINE, &error);

    // Check for errors on pipe creation.
    if ((NULL == appctx->pipeline) && (error != NULL)) {
      g_printerr ("Failed to create pipeline, error: %s!\n",
          GST_STR_NULL (error->message));
      g_clear_error (&error);

      gst_app_context_free (appctx);
      return -1;
    } else if ((NULL == appctx->pipeline) && (NULL == error)) {
      g_printerr ("Failed to create pipeline, unknown error!\n");

      gst_app_context_free (appctx);
      return -1;
    } else if ((appctx->pipeline != NULL) && (error != NULL)) {
      g_printerr ("Erroneous pipeline, error: %s!\n",
          GST_STR_NULL (error->message));

      g_clear_error (&error);
      gst_app_context_free (appctx);
      return -1;
    }
  }

  // Connect a callback to the new-sample signal.
  // element = gst_bin_get_by_name (GST_BIN (appctx->pipeline), "sink");
  // g_signal_connect (element, "new-sample", G_CALLBACK (new_sample), NULL);
  // gst_object_unref (element);

  // Get a reference to the camera plugin.
  element = gst_bin_get_by_name (GST_BIN (appctx->pipeline), "camera");

  // Connect a callbacks to the qtiqmmfsrc metadata signals.
  g_signal_connect (element, "result-metadata",
      G_CALLBACK (result_metadata), NULL);
  g_signal_connect (element, "urgent-metadata",
      G_CALLBACK (urgent_metadata), NULL);

  // Decrease the reference count to the camera element.
  gst_object_unref (element);

  // Initialize main loop.
  if ((appctx->mloop = g_main_loop_new (NULL, FALSE)) == NULL) {
    g_printerr ("ERROR: Failed to create Main loop!\n");
    gst_app_context_free (appctx);
    return -1;
  }
    GstBus *bus = NULL;

    // Retrieve reference to the pipeline's bus.
    if ((bus = gst_pipeline_get_bus (GST_PIPELINE (appctx->pipeline))) == NULL) {
      g_printerr ("ERROR: Failed to retrieve pipeline bus!\n");
      gst_app_context_free (appctx);
      return -1;
    }

    // Watch for messages on the pipeline's bus.
    bus_watch_id = gst_bus_add_watch (bus, handle_bus_message, appctx);
    gst_object_unref (bus);
 // }

  // Register function for handling interrupt signals with the main loop.
  intrpt_watch_id = g_unix_signal_add (SIGINT, handle_interrupt_signal, appctx);



     // Create IO channel from the stdin stream.
    if ((iostdin = g_io_channel_unix_new (fileno (stdin))) == NULL) {
      g_printerr ("ERROR: Failed to initialize Main loop!\n");
      gst_app_context_free (appctx);
      return -1;
    }

    // Register handing function with the main loop for stdin channel data.
    GIOCondition flag = (GIOCondition)(G_IO_IN | G_IO_PRI);
    stdin_watch_id = g_io_add_watch (
        iostdin, flag, handle_stdin_source, appctx);
    g_io_channel_unref (iostdin);

	// Initiate the main menu thread.
	if ((mthread_menu = g_thread_new ("MainMenu", main_menu, appctx)) == NULL) {
		 g_printerr ("ERROR: Failed to create event loop thread!\n");
		 g_print ("\n init main menu thread failed appctx: %p\n", appctx);
		 gst_app_context_free (appctx);
		 g_print ("ERROR: Failed to create event loop thread!appctx - %p\n",appctx);
		 return -1;
	   }

   #if 0
	 // Initiate the main thread in which we will work with the camera element.
  if ((mthread = g_thread_new ("WorkTask", work_task, appctx)) == NULL) {
    g_printerr ("ERROR: Failed to create event loop thread!\n");
    gst_app_context_free (appctx);
    return -1;
  }
  #endif

 ANGORA_LOG_INFO ("enter ...");

  // Set pipeline playing.
  gst_element_set_state (appctx->pipeline, GST_STATE_PLAYING);

  // Run main loop.
  g_main_loop_run (appctx->mloop);

  // Signal menu thread to quit.
  g_async_queue_push (appctx->messages,
      gst_structure_new_empty (TERMINATE_MESSAGE));

  // Waits until main menu thread finishes.
  g_thread_join (mthread_menu);

  // g_thread_join (mthread);

  // Waits trigger mode thread finishes.
  ANGORA_LOG_INFO ("end ...");

  g_source_remove (bus_watch_id);
  g_source_remove (stdin_watch_id);
  g_source_remove (intrpt_watch_id);

  gst_app_context_free (appctx);

  gst_deinit ();

  return 0;
}
