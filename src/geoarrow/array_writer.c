
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow.h"

struct GeoArrowArrayWriterPrivate {
  struct GeoArrowWKTWriter wkt_writer;
  struct GeoArrowWKBWriter wkb_writer;
  struct GeoArrowBuilder builder;
  enum GeoArrowType type;
};

GeoArrowErrorCode GeoArrowArrayWriterInitFromType(struct GeoArrowArrayWriter* writer,
                                                  enum GeoArrowType type) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowArrayWriterPrivate));

  if (private_data == NULL) {
    return ENOMEM;
  }

  memset(private_data, 0, sizeof(struct GeoArrowArrayWriterPrivate));

  int result;
  switch (type) {
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_LARGE_WKB:
      return ENOTSUP;
    case GEOARROW_TYPE_WKT:
      result = GeoArrowWKTWriterInit(&private_data->wkt_writer);
      break;
    case GEOARROW_TYPE_WKB:
      result = GeoArrowWKBWriterInit(&private_data->wkb_writer);
      break;
    default:
      result = GeoArrowBuilderInitFromType(&private_data->builder, type);
      break;
  }

  if (result != GEOARROW_OK) {
    ArrowFree(private_data);
    return result;
  }

  private_data->type = type;
  writer->private_data = private_data;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayWriterInitFromSchema(struct GeoArrowArrayWriter* writer,
                                                    const struct ArrowSchema* schema) {
  struct GeoArrowSchemaView schema_view;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInit(&schema_view, schema, NULL));
  return GeoArrowArrayWriterInitFromType(writer, schema_view.type);
}

GeoArrowErrorCode GeoArrowArrayWriterSetPrecision(struct GeoArrowArrayWriter* writer,
                                                  int precision) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  if (private_data->type != GEOARROW_TYPE_WKT &&
      private_data->type != GEOARROW_TYPE_LARGE_WKT) {
    return EINVAL;
  }

  private_data->wkt_writer.precision = precision;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayWriterSetFlatMultipoint(struct GeoArrowArrayWriter* writer,
                                                       int flat_multipoint) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  if (private_data->type != GEOARROW_TYPE_WKT &&
      private_data->type != GEOARROW_TYPE_LARGE_WKT) {
    return EINVAL;
  }

  private_data->wkt_writer.use_flat_multipoint = flat_multipoint;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayWriterInitVisitor(struct GeoArrowArrayWriter* writer,
                                                 struct GeoArrowVisitor* v) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  switch (private_data->type) {
    case GEOARROW_TYPE_WKT:
      GeoArrowWKTWriterInitVisitor(&private_data->wkt_writer, v);
      return GEOARROW_OK;
    case GEOARROW_TYPE_WKB:
      GeoArrowWKBWriterInitVisitor(&private_data->wkb_writer, v);
      return GEOARROW_OK;
    default:
      return GeoArrowBuilderInitVisitor(&private_data->builder, v);
  }
}

GeoArrowErrorCode GeoArrowArrayWriterBuilder(struct GeoArrowArrayWriter* writer,
                                             struct GeoArrowBuilder** out) {
  NANOARROW_DCHECK(writer != NULL);
  NANOARROW_DCHECK(out != NULL);
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  // One could update the wkt/wkb writers to use the builder so that this could
  // return the builder in any case.
  switch (private_data->type) {
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_WKB:
      return ENOTSUP;
    default:
      *out = &private_data->builder;
      return GEOARROW_OK;
  }
}

GeoArrowErrorCode GeoArrowArrayWriterFinish(struct GeoArrowArrayWriter* writer,
                                            struct ArrowArray* array,
                                            struct GeoArrowError* error) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  switch (private_data->type) {
    case GEOARROW_TYPE_WKT:
      return GeoArrowWKTWriterFinish(&private_data->wkt_writer, array, error);
    case GEOARROW_TYPE_WKB:
      return GeoArrowWKBWriterFinish(&private_data->wkb_writer, array, error);
    default:
      return GeoArrowBuilderFinish(&private_data->builder, array, error);
  }
}

void GeoArrowArrayWriterReset(struct GeoArrowArrayWriter* writer) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  if (private_data->wkt_writer.private_data != NULL) {
    GeoArrowWKTWriterReset(&private_data->wkt_writer);
  }

  if (private_data->wkb_writer.private_data != NULL) {
    GeoArrowWKBWriterReset(&private_data->wkb_writer);
  }

  if (private_data->builder.private_data != NULL) {
    GeoArrowBuilderReset(&private_data->builder);
  }

  ArrowFree(private_data);
  writer->private_data = NULL;
}
