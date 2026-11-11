
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

struct GeoArrowArrayWriterPrivate {
  struct GeoArrowNativeWriter native_writer;
  struct GeoArrowWKTWriter wkt_writer;
  struct GeoArrowWKBWriter wkb_writer;
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
    case GEOARROW_TYPE_WKT_VIEW:
    case GEOARROW_TYPE_WKB_VIEW:
      return ENOTSUP;
    case GEOARROW_TYPE_WKT:
      result = GeoArrowWKTWriterInit(&private_data->wkt_writer);
      break;
    case GEOARROW_TYPE_WKB:
      result = GeoArrowWKBWriterInit(&private_data->wkb_writer);
      break;
    default:
      result = GeoArrowNativeWriterInit(&private_data->native_writer, type);
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

  return GeoArrowWKTWriterSetPrecision(&private_data->wkt_writer, precision);
}

GeoArrowErrorCode GeoArrowArrayWriterSetFlatMultipoint(struct GeoArrowArrayWriter* writer,
                                                       int flat_multipoint) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  if (private_data->type != GEOARROW_TYPE_WKT &&
      private_data->type != GEOARROW_TYPE_LARGE_WKT) {
    return EINVAL;
  }

  return GeoArrowWKTWriterSetFlatMultipoint(&private_data->wkt_writer, flat_multipoint);
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
      return GeoArrowNativeWriterInitVisitor(&private_data->native_writer, v);
  }
}

GeoArrowErrorCode GeoArrowArrayWriterAppend(struct GeoArrowArrayWriter* writer,
                                            struct GeoArrowGeometryView geom,
                                            struct GeoArrowError* error) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  if (geom.root == NULL || geom.size_nodes <= 0) {
    GeoArrowErrorSet(error, "Expected a geometry view containing at least one node");
    return EINVAL;
  }

  if (geom.size_nodes == 1 &&
      geom.root->geometry_type == GEOARROW_GEOMETRY_TYPE_GEOMETRY) {
    return GeoArrowArrayWriterAppendNull(writer);
  }

  switch (private_data->type) {
    case GEOARROW_TYPE_WKT:
      return GeoArrowWKTWriterAppend(&private_data->wkt_writer, geom);
    case GEOARROW_TYPE_WKB:
      return GeoArrowWKBWriterAppend(&private_data->wkb_writer, geom);
    default:
      return GeoArrowNativeWriterAppend(&private_data->native_writer, geom, error);
  }
}

GeoArrowErrorCode GeoArrowArrayWriterAppendNull(struct GeoArrowArrayWriter* writer) {
  struct GeoArrowArrayWriterPrivate* private_data =
      (struct GeoArrowArrayWriterPrivate*)writer->private_data;

  switch (private_data->type) {
    case GEOARROW_TYPE_WKT:
      return GeoArrowWKTWriterAppendNull(&private_data->wkt_writer);
    case GEOARROW_TYPE_WKB:
      return GeoArrowWKBWriterAppendNull(&private_data->wkb_writer);
    default:
      return GeoArrowNativeWriterAppendNull(&private_data->native_writer);
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
      return GeoArrowNativeWriterFinish(&private_data->native_writer, array, error);
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

  if (private_data->native_writer.private_data != NULL) {
    GeoArrowNativeWriterReset(&private_data->native_writer);
  }

  ArrowFree(private_data);
  writer->private_data = NULL;
}
