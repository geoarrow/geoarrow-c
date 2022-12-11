
#include <errno.h>

#include "geoarrow.h"

#include "nanoarrow.h"

static int GeoArrowArrayInitInternal(struct GeoArrowArray* array,
                                     struct GeoArrowError* error) {
  switch (array->schema_view.type) {
    case GEOARROW_TYPE_WKB: {
      struct GeoArrowWKBWriter* writer =
          (struct GeoArrowWKBWriter*)ArrowMalloc(sizeof(struct GeoArrowWKBWriter));
      if (writer == NULL) {
        return ENOMEM;
      }
      NANOARROW_RETURN_NOT_OK(GeoArrowWKBWriterInit(writer));
      return GEOARROW_OK;
    }
    default:
      ArrowErrorSet((struct ArrowError*)error, "Type not supported");
      return ENOTSUP;
  }
}

GeoArrowErrorCode GeoArrowArrayInitFromType(struct GeoArrowArray* array,
                                            enum GeoArrowType type) {
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInitFromType(&array->schema_view, type));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayInitFromSchema(struct GeoArrowArray* array,
                                              struct ArrowSchema* schema,
                                              struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInit(&array->schema_view, schema, error));
  return GEOARROW_OK;
}

void GeoArrowArrayInitVisitor(struct GeoArrowArray* array, struct GeoArrowVisitor* v) {
  switch (array->schema_view.type) {
    case GEOARROW_TYPE_WKB:
      GeoArrowWKBWriterInitVisitor((struct GeoArrowWKBWriter*)array->private_data, v);
      break;
    default:
      memset(v, 0, sizeof(struct GeoArrowVisitor));
      break;
  }
}

GeoArrowErrorCode GeoArrowArrayFinish(struct GeoArrowArray* array,
                                      struct ArrowArray* array_out,
                                      struct GeoArrowError* error) {
  return ENOTSUP;
}

void GeoArrowArrayReset(struct GeoArrowArray* array) {}
