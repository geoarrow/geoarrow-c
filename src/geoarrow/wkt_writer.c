
#include <stdio.h>
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

// Using ryu for double -> char* is ~5x faster and is not locale dependent
// could also use to_chars() if C++17 is available
// https://github.com/paleolimbot/geoarrow/tree/58ccd0a9606f3f6e51f200e143d8c7672782e30a/src/ryu
#ifndef geoarrow_d2s_fixed_n
static inline int geoarrow_compat_d2s_fixed_n(double f, uint32_t precision,
                                              char* result) {
  return snprintf(result, 128, "%.*g", precision, f);
}

#define geoarrow_d2s_fixed_n geoarrow_compat_d2s_fixed_n
#endif

struct WKTWriterPrivate {
  enum ArrowType storage_type;
  struct ArrowBitmap validity;
  struct ArrowBuffer offsets;
  struct ArrowBuffer values;
  enum GeoArrowGeometryType geometry_type[32];
  int64_t i[32];
  int32_t level;
  int64_t length;
  int64_t null_count;
  int significant_digits;
};

static inline int WKTWriterCheckLevel(struct WKTWriterPrivate* private) {
  printf("Checking level: %d\n", private->level);
  if (private->level >= 0 && private->level <= 31) {
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static inline int WKTWriterWrite(struct WKTWriterPrivate* private, const char* value) {
  return ArrowBufferAppend(&private->values, value, strlen(value));
}

static inline void WKTWriterWriteDoubleUnsafe(struct WKTWriterPrivate* private,
                                              double value) {
  private->values.size_bytes +=
      geoarrow_d2s_fixed_n(value, private->significant_digits,
                           ((char*)private->values.data) + private->values.size_bytes);
}

static int reserve_feat_wkt(struct GeoArrowVisitor* v, int64_t n) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;

  if (private->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&private->validity, n));
  }

  NANOARROW_RETURN_NOT_OK(ArrowBufferReserve(&private->offsets, n * sizeof(int32_t)));
  return ArrowBufferReserve(&private->values, n * strlen("POINT ()"));
}

static int feat_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->level = -1;
  private->length++;
  return ArrowBufferAppendInt32(&private->offsets, private->values.size_bytes);
}

static int null_feat_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  if (private->validity.buffer.data == NULL && private->length > 0) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&private->validity, private->length));
    ArrowBitmapAppendUnsafe(&private->validity, 1, private->length - 1);
  }

  private->null_count++;
  return ArrowBitmapAppend(&private->validity, 0, 1);
}

static int geom_start_wkt(struct GeoArrowVisitor* v,
                          enum GeoArrowGeometryType geometry_type,
                          enum GeoArrowDimensions dimensions) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->level++;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->level > 0 && private->i[private->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, ", "));
  } else if (private->level > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, "("));
  }

  if (private->level == 0 || private->geometry_type[private->level - 1] ==
                                 GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION) {
    const char* geometry_type_name = GeoArrowGeometryTypeString(geometry_type);
    if (geometry_type_name == NULL) {
      ArrowErrorSet((struct ArrowError*)v->error,
                    "WKTWriter::geom_start(): Unexpected `geometry_type`");
      return EINVAL;
    }

    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, geometry_type_name));

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XY:
        break;
      case GEOARROW_DIMENSIONS_XYZ:
        NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, " Z"));
        break;
      case GEOARROW_DIMENSIONS_XYM:
        NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, " M"));
        break;
      case GEOARROW_DIMENSIONS_XYZM:
        NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, " ZM"));
        break;
      default:
        ArrowErrorSet((struct ArrowError*)v->error,
                      "WKTWriter::geom_start(): Unexpected `dimensions`");
        return EINVAL;
    }

    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, " "));
  }

  if (private->level > 0) {
    private->i[private->level - 1]++;
  }

  private->geometry_type[private->level] = geometry_type;
  private->i[private->level] = 0;
  return GEOARROW_OK;
}

static int ring_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->level++;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->level > 0 && private->i[private->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, ", "));
  } else {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, "("));
  }

  if (private->level > 0) {
    private->i[private->level - 1]++;
  }

  private->geometry_type[private->level] = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  private->i[private->level] = 0;
  return GEOARROW_OK;
}

static int coords_wkt(struct GeoArrowVisitor* v, const double** values, int64_t n_coords,
                      int32_t n_dims) {
  if (n_coords == 0) {
    return GEOARROW_OK;
  }

  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  int64_t max_chars_needed = (n_coords * 2) +  // space + comma after coordinate
                             (n_coords * (n_dims - 1)) +  // spaces between ordinates
                             ((private->significant_digits + 1) * n_coords *
                              n_dims);  // significant digits + decimal
  NANOARROW_RETURN_NOT_OK(ArrowBufferReserve(&private->values, max_chars_needed));

  // Write the first coordinate, possibly with a leading comma if there was
  // a previous call to coords
  if (private->i[private->level] != 0) {
    ArrowBufferAppendUnsafe(&private->values, ", ", 2);
  } else {
    ArrowBufferAppendUnsafe(&private->values, "(", 1);
  }

  WKTWriterWriteDoubleUnsafe(private, values[0][0]);
  for (int32_t j = 1; j < n_dims; j++) {
    ArrowBufferAppendUnsafe(&private->values, " ", 1);
    WKTWriterWriteDoubleUnsafe(private, values[j][0]);
  }

  // Write the remaining coordinates (which all have leading commas)
  for (int64_t i = 1; i < n_coords; i++) {
    ArrowBufferAppendUnsafe(&private->values, ", ", 2);
    WKTWriterWriteDoubleUnsafe(private, values[0][i]);
    for (int32_t j = 1; j < n_dims; j++) {
      ArrowBufferAppendUnsafe(&private->values, " ", 1);
      WKTWriterWriteDoubleUnsafe(private, values[j][i]);
    }
  }

  private->i[private->level] += n_coords;
  return GEOARROW_OK;
}

static int ring_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->i[private->level] == 0) {
    private->level--;
    return WKTWriterWrite(private, "EMPTY");
  } else {
    private->level--;
    return WKTWriterWrite(private, ")");
  }
}

static int geom_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->i[private->level] == 0) {
    private->level--;
    return WKTWriterWrite(private, "EMPTY");
  } else {
    private->level--;
    return WKTWriterWrite(private, ")");
  }
}

GeoArrowErrorCode GeoArrowWKTWriterInit(struct GeoArrowWKTWriter* writer) {
  struct WKTWriterPrivate* private =
      (struct WKTWriterPrivate*)ArrowMalloc(sizeof(struct WKTWriterPrivate));
  if (private == NULL) {
    return ENOMEM;
  }

  private->storage_type = NANOARROW_TYPE_STRING;
  private->length = 0;
  private->level = 0;
  private->null_count = 0;
  ArrowBitmapInit(&private->validity);
  ArrowBufferInit(&private->offsets);
  ArrowBufferInit(&private->values);
  private->significant_digits = 16;
  writer->private_data = private;

  return GEOARROW_OK;
}

void GeoArrowWKTWriterInitVisitor(struct GeoArrowWKTWriter* writer,
                                  struct GeoArrowVisitor* v) {
  GeoArrowVisitorInitVoid(v);
  v->private_data = writer->private_data;
  v->reserve_feat = &reserve_feat_wkt;
  v->feat_start = &feat_start_wkt;
  v->null_feat = &null_feat_wkt;
  v->geom_start = &geom_start_wkt;
  v->ring_start = &ring_start_wkt;
  v->coords = &coords_wkt;
  v->ring_end = &ring_end_wkt;
  v->geom_end = &geom_end_wkt;
}

GeoArrowErrorCode GeoArrowWKTWriterFinish(struct GeoArrowWKTWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)writer->private_data;
  array->release = NULL;

  NANOARROW_RETURN_NOT_OK(
      ArrowBufferAppendInt32(&private->offsets, private->values.size_bytes));
  NANOARROW_RETURN_NOT_OK(ArrowArrayInit(array, private->storage_type));
  ArrowArraySetValidityBitmap(array, &private->validity);
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 1, &private->offsets));
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 2, &private->values));
  array->length = private->length;
  array->null_count = private->null_count;
  return ArrowArrayFinishBuilding(array, (struct ArrowError*)error);
}

void GeoArrowWKTWriterReset(struct GeoArrowWKTWriter* writer) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)writer->private_data;
  ArrowBitmapReset(&private->validity);
  ArrowBufferReset(&private->offsets);
  ArrowBufferReset(&private->values);
  ArrowFree(private);
  writer->private_data = NULL;
}
