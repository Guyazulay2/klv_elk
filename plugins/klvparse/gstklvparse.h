#ifndef __GST_klvparse_H__
#define __GST_klvparse_H__

#include <gst/gst.h>
#include <KLVParser.h>

using namespace KLV;

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_klvparse \
  (gst_klvparse_get_type())
#define GST_klvparse(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_klvparse,Gstklvparse))
#define GST_klvparse_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_klvparse,GstklvparseClass))
#define GST_IS_klvparse(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_klvparse))
#define GST_IS_klvparse_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_klvparse))

typedef struct _Gstklvparse      Gstklvparse;
typedef struct _GstklvparseClass GstklvparseClass;

struct _Gstklvparse
{
  GstElement element;
  KLVParser *parser;
  GstPad *sinkpad, *srcpad;
};

struct _GstklvparseClass 
{
  GstElementClass parent_class;
};

GType gst_klvparse_get_type (void);

G_END_DECLS

#endif /* __GST_klvparse_H__ */
