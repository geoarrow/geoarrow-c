
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
  int result = GeoArrowArrayInitFromSchema(array, &schema, NULL);
  schema.release(&schema);
  return result;
}

GeoArrowErrorCode GeoArrowArrayFinish(struct GeoArrowArray* array,
                                      struct ArrowArray* array_out,
                                      struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayFinishBuilding(&array->array, (struct ArrowError*)error));
  memcpy(array_out, &array->array, sizeof(struct ArrowArray));
  return GEOARROW_OK;
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

  return GEOARROW_OK;
}
