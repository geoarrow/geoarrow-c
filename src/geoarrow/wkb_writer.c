
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

struct WKBWriterPrivate {
  enum ArrowType storage_type;
  struct ArrowBitmap validity;
  struct ArrowBuffer offsets;
  struct ArrowBuffer values;
  enum GeoArrowGeometryType geometry_type[32];
  int64_t size_pos[32];
  uint32_t size[32];
  int32_t level;
  int64_t length;
  int64_t null_count;
};

#define GEOARROW_NATIVE_ENDIAN 0x01

static inline int WKBWriterCheckLevel(struct WKBWriterPrivate* private) {
  if (private->level >= 0 && private->level <= 31) {
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static int feat_start_wkb(struct GeoArrowVisitor* v) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;
  private->level = 0;
  private->length++;
  return ArrowBufferAppendInt32(&private->offsets, private->values.size_bytes);
}

static int null_feat_wkb(struct GeoArrowVisitor* v) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;
  if (private->validity.buffer.data == NULL && private->length > 0) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&private->validity, private->length));
    ArrowBitmapAppendUnsafe(&private->validity, 1, private->length - 1);
  }

  private->null_count++;
  return ArrowBitmapAppend(&private->validity, 0, 1);
}

static int geom_start_wkb(struct GeoArrowVisitor* v,
                          enum GeoArrowGeometryType geometry_type,
                          enum GeoArrowDimensions dimensions) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;
  private->size[private->level]++;
  private->level++;
  private->geometry_type[private->level] = geometry_type;

  NANOARROW_RETURN_NOT_OK(
      ArrowBufferAppendUInt8(&private->values, GEOARROW_NATIVE_ENDIAN));
  NANOARROW_RETURN_NOT_OK(ArrowBufferAppendUInt32(
      &private->values, geometry_type + ((dimensions - 1) * 1000)));
  if (geometry_type != GEOARROW_GEOMETRY_TYPE_POINT) {
    private->size_pos[private->level] = private->values.size_bytes;
    NANOARROW_RETURN_NOT_OK(ArrowBufferAppendUInt32(&private->values, 0));
  }

  return GEOARROW_OK;
}

static int ring_start_wkb(struct GeoArrowVisitor* v) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;
  private->size[private->level]++;
  private->level++;
  private->geometry_type[private->level] = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  private->size_pos[private->level] = private->values.size_bytes;
  return ArrowBufferAppendUInt32(&private->values, 0);
}

static int coords_wkb(struct GeoArrowVisitor* v, const double** values, int64_t n_coords,
                      int32_t n_dims) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;
  private->size[private->level] += n_coords;
  NANOARROW_RETURN_NOT_OK(
      ArrowBufferReserve(&private->values, n_dims * n_coords * sizeof(double)));
  for (int64_t i = 0; i < n_coords; i++) {
    for (int32_t j = 0; j < n_dims; j++) {
      ArrowBufferAppendUnsafe(&private->values, values[j] + i, sizeof(double));
    }
  }

  return GEOARROW_OK;
}

static int ring_end_wkb(struct GeoArrowVisitor* v) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;
  memcpy(private->values.data + private->size_pos[private->level],
         private->size + private->level, sizeof(uint32_t));
  private->level--;
  return GEOARROW_OK;
}

static int geom_end_wkb(struct GeoArrowVisitor* v) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)v->private_data;

  if (private->geometry_type[private->level] != GEOARROW_GEOMETRY_TYPE_POINT) {
    memcpy(private->values.data + private->size_pos[private->level],
           private->size + private->level, sizeof(uint32_t));
  }

  private->level--;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowWKBWriterInit(struct GeoArrowWKBWriter* writer) {
  struct WKBWriterPrivate* private =
      (struct WKBWriterPrivate*)ArrowMalloc(sizeof(struct WKBWriterPrivate));
  if (private == NULL) {
    return ENOMEM;
  }

  private->storage_type = NANOARROW_TYPE_BINARY;
  private->length = 0;
  private->level = 0;
  private->null_count = 0;
  ArrowBitmapInit(&private->validity);
  ArrowBufferInit(&private->offsets);
  ArrowBufferInit(&private->values);
  writer->private_data = private;

  return GEOARROW_OK;
}

void GeoArrowWKBWriterInitVisitor(struct GeoArrowWKBWriter* writer,
                                  struct GeoArrowVisitor* v) {
  GeoArrowVisitorInitVoid(v);

  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)writer->private_data;

  v->private_data = writer->private_data;
  v->feat_start = &feat_start_wkb;
  v->null_feat = &null_feat_wkb;
  v->geom_start = &geom_start_wkb;
  v->ring_start = &ring_start_wkb;
  v->coords = &coords_wkb;
  v->ring_end = &ring_end_wkb;
  v->geom_end = &geom_end_wkb;
}

GeoArrowErrorCode GeoArrowWKBWriterFinish(struct GeoArrowWKBWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)writer->private_data;
  array->release = NULL;

  NANOARROW_RETURN_NOT_OK(
      ArrowBufferAppendInt32(&private->offsets, private->values.size_bytes));
  NANOARROW_RETURN_NOT_OK(ArrowArrayInit(array, private->storage_type));
  ArrowArraySetValidityBitmap(array, &private->validity);
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 1, &private->offsets));
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 2, &private->values));
  array->length = private->length;
  array->null_count = private->null_count;
  private->length = 0;
  private->null_count = 0;
  return ArrowArrayFinishBuilding(array, (struct ArrowError*)error);
}

void GeoArrowWKBWriterReset(struct GeoArrowWKBWriter* writer) {
  struct WKBWriterPrivate* private = (struct WKBWriterPrivate*)writer->private_data;
  ArrowBitmapReset(&private->validity);
  ArrowBufferReset(&private->offsets);
  ArrowBufferReset(&private->values);
  ArrowFree(private);
  writer->private_data = NULL;
}
