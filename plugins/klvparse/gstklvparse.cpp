/**
 * SECTION:element-klvparse
 *
 * FIXME:Describe klvparse here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! klvparse ! fakesink silent=TRUE
 * ]|
 * 
 * gst-launch-1.0 filesrc location="/home/pavel/Desktop/PCAPS/stanag4609-239.10.12.2.pcap" ! pcapparse dst-ip=239.10.12.2 ! tsdemux ! meta/x-klv  ! identity eos-after=2 ! klvparse ! fakesink dump=true
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <sys/time.h>
#include "gstklvparse.h"
#include <inttypes.h>
GST_DEBUG_CATEGORY_STATIC (gst_klvparse_debug);
#define GST_CAT_DEFAULT gst_klvparse_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_FPS,
  PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("meta/x-klv")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-json, encoding=(string)UTF-8")
    );

#define gst_klvparse_parent_class parent_class
G_DEFINE_TYPE (Gstklvparse, gst_klvparse, GST_TYPE_ELEMENT);

static void gst_klvparse_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_klvparse_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_klvparse_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_klvparse_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static void gst_klvparse_finalize(GObject * object);
static void gst_klvparse_dispose(GObject * object);

/* GObject vmethod implementations */

/* initialize the klvparse's class */
static void
gst_klvparse_class_init (GstklvparseClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_klvparse_set_property;
  gobject_class->get_property = gst_klvparse_get_property;

  gobject_class->finalize = gst_klvparse_finalize;
  gobject_class->dispose = gst_klvparse_dispose;

  // g_object_class_install_property (gobject_class, PROP_SILENT,
  //     g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
  //         FALSE, G_PARAM_READWRITE));

  // g_object_class_install_property (gobject_class, PROP_FPS,
  //     g_param_spec_float ("fps", "FPS", "Set Output FPS",
	//       -1,
	//       5000,
	//       -1, G_PARAM_READWRITE));
  gst_element_class_set_details_simple(gstelement_class,
    "klvparse",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "shvarpa <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_klvparse_init (Gstklvparse * filter)
{
  filter->parser = new KLVParser();
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_klvparse_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_klvparse_chain));
  // GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  // GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  // filter->silent = FALSE;
  // filter->sent_sei = FALSE;
  // filter->fps_num = 0;
  // filter->fps_den = 1;

}

static void
gst_klvparse_finalize(GObject * object) {
    Gstklvparse *filter = GST_klvparse (object);
    if(filter->parser != NULL) {
      delete filter->parser;
      filter->parser = NULL;
    }
}

// called multiple times
static void gst_klvparse_dispose(GObject * object) {}

static void
gst_klvparse_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstklvparse *filter = GST_klvparse (object);

  switch (prop_id) {
    case PROP_SILENT:
      // filter->silent = g_value_get_boolean (value);
      break;
    case PROP_FPS:
      // filter->fps_num = g_value_get_float(value);
      // filter->fps_den = 1;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_klvparse_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstklvparse *filter = GST_klvparse (object);

  switch (prop_id) {
    case PROP_SILENT:
      // g_value_set_boolean (value, filter->silent);
      break;
    case PROP_FPS:
      // g_value_set_float (value, filter->fps_num);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_klvparse_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstklvparse *filter;
  gboolean ret;

  filter = GST_klvparse (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}



/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_klvparse_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    Gstklvparse *filter = GST_klvparse (parent);

    GstMapInfo info;
    if(!gst_buffer_map(buf, &info, GST_MAP_READ)) {
      return GST_FLOW_ERROR;
    }

    GstFlowReturn ret = GST_FLOW_OK;

    if(filter->parser->parse(info.data, info.size)) {
      json j = filter->parser->to_json();
      // std::cout << j.dump(2) << std::endl;
      std::string j_str = j.dump();
      GstBuffer *j_buf = gst_buffer_new_and_alloc(j_str.size());
      GstMapInfo j_info;
      if(gst_buffer_map(j_buf, &j_info, GST_MAP_READWRITE)) {
        memcpy(j_info.data, j_str.c_str(), j_str.size());
        gst_buffer_unmap(j_buf, &j_info);
        // GstBuffer *j_buf = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, (gpointer) j_str.c_str(), j_str.size(), 0, j_str.size(), NULL, NULL);
        ret = gst_pad_push(filter->srcpad, j_buf);
      }

    }

    gst_buffer_unmap(buf, &info);
    gst_buffer_unref(buf);
    
    return ret;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
klvparse_init (GstPlugin * klvparse)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template klvparse' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_klvparse_debug, "klvparse",
      0, "Template klvparse");

  return gst_element_register (klvparse, "klvparse", GST_RANK_NONE,
      GST_TYPE_klvparse);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstklvparse"
#endif

/* gstreamer looks for this structure to register klvparses
 *
 * exchange the string 'Template klvparse' with your klvparse description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    klvparse,
    "Template klvparse",
    klvparse_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
