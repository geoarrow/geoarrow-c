
#include <errno.h>

#include "geoarrow.h"

#include "nanoarrow.h"

GeoArrowErrorCode GeoArrowArrayInitFromSchema(struct GeoArrowArray* array,
                                              struct ArrowSchema* schema,
                                              struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInit(&array->schema_view, schema, error));
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayInitFromSchema(&array->array, schema, (struct ArrowError*)error));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayInitFromType(struct GeoArrowArray* array,
                                            enum GeoArrowType type) {
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInitFromType(&array->schema_view, type));
  struct ArrowSchema schema;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInit(&schema, type));
  int result = ArrowArrayInitFromSchema(&array->array, &schema, NULL);
  schema.release(&schema);
  return result;
}

void GeoArrowArrayReset(struct GeoArrowArray* array) {
  if (array->array.release != NULL) {
    array->array.release(&array->array);
  }
  array->schema_view.type = GEOARROW_TYPE_UNINITIALIZED;
}

struct GeoArrowFindBufferResult {
  struct ArrowArray* array;
  int level;
  int i;
};

static int GeoArrowArrayFindBuffer(struct ArrowArray* array,
                                   struct GeoArrowFindBufferResult* res, int i, int level,
                                   int skip_first) {
  int total_buffers = (array->n_buffers - skip_first);
  if (i < total_buffers) {
    res->array = array;
    res->i = i + skip_first;
    res->level = level;
    return total_buffers;
  }

  i -= total_buffers;

  for (int64_t child_id = 0; child_id < array->n_children; child_id++) {
    int child_buffers =
        GeoArrowArrayFindBuffer(array->children[child_id], res, i, level + 1, 1);
    total_buffers += child_buffers;
    if (i < child_buffers) {
      return total_buffers;
    }
    i -= child_buffers;
  }

  return total_buffers;
}

static void GeoArrowSetArrayLengthFromBufferLength(struct GeoArrowSchemaView* schema_view,
                                                   struct GeoArrowFindBufferResult* res,
                                                   int64_t size_bytes) {
  // By luck, buffer index 1 for every array is the one we use to infer the length;
  // however, this is a slightly different formula for each type/depth
  if (res->i != 1) {
    return;
  }

  switch (schema_view->type) {
    case GEOARROW_TYPE_WKB:
      res->array->length = (size_bytes / sizeof(int32_t)) - 1;
      return;
    case GEOARROW_TYPE_LARGE_WKB:
      res->array->length = (size_bytes / sizeof(int64_t)) - 1;
      return;
    default:
      break;
  }

  int coord_level;
  switch (schema_view->geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      coord_level = 0;
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      coord_level = 1;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      coord_level = 2;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      coord_level = 3;
    default:
      return;
  }

  if (res->level < coord_level) {
    // This is an offset buffer
    res->array->length = (size_bytes / sizeof(int32_t)) - 1;
  } else {
    // This is a data buffer
    res->array->length = size_bytes / sizeof(double);
  }
}

GeoArrowErrorCode GeoArrowArraySetBufferCopy(struct GeoArrowArray* array, int64_t i,
                                             struct GeoArrowBufferView value) {
  struct GeoArrowFindBufferResult res;
  res.array = 0;
  res.level = -1;
  res.i = -1;
  GeoArrowArrayFindBuffer(&array->array, &res, i, 0, 0);

  if (res.array == NULL) {
    return ERANGE;
  }

  // Set the buffer content
  struct ArrowBuffer* buffer = ArrowArrayBuffer(res.array, res.i);
  NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, value.data, value.n_bytes));

  // Possibly set the array length from the buffer length
  GeoArrowSetArrayLengthFromBufferLength(&array->schema_view, &res, value.n_bytes);

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayFinish(struct GeoArrowArray* array,
                                      struct ArrowArray* array_out,
                                      struct GeoArrowError* error) {
  // At this point all the array lengths should be set except for the
  // fixed-size list or struct parent to the coordinate array(s).
  int scale;
  switch (array->schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_SEPARATE:
      scale = 1;
      break;
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      switch (array->schema_view.dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          scale = 2;
          break;
        case GEOARROW_DIMENSIONS_XYZ:
        case GEOARROW_DIMENSIONS_XYM:
          scale = 3;
          break;
        case GEOARROW_DIMENSIONS_XYZM:
          scale = 4;
          break;
        default:
          return EINVAL;
      }
      break;
    default:
      // e.g., WKB
      break;
  }

  switch (array->schema_view.geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      array->array.length = array->array.children[0]->length;
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      array->array.children[0]->length = array->array.children[0]->children[0]->length;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      array->array.children[0]->children[0]->length =
          array->array.children[0]->children[0]->children[0]->length;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      array->array.children[0]->children[0]->children[0]->length =
          array->array.children[0]->children[0]->children[0]->children[0]->length;
      break;
    default:
      // e.g., WKB
      break;
  }

  NANOARROW_RETURN_NOT_OK(
      ArrowArrayFinishBuilding(&array->array, (struct ArrowError*)error));
  memcpy(array_out, &array->array, sizeof(struct ArrowArray));
  array->array.release = NULL;
  return GEOARROW_OK;
}
