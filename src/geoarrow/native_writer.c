
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow.h"

// Bytes for four quiet (little-endian) NANs
// static uint8_t kEmptyPointCoords[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
//                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
//                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
//                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};

struct GeoArrowNativeWriterPrivate {
  struct GeoArrowBuilder builder;

  // Fields to keep track of state when using the visitor pattern
  int visitor_initialized;
  int feat_is_null;
  int nesting_multipoint;
  double empty_coord_values[4];
  struct GeoArrowCoordView empty_coord;
  enum GeoArrowDimensions last_dimensions;
  int64_t size[32];
  int32_t level;
  int64_t null_count;
};

GeoArrowErrorCode GeoArrowNativeWriterInit(struct GeoArrowNativeWriter* writer,

                                           enum GeoArrowType type) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowNativeWriterPrivate));
  if (private_data == NULL) {
    return ENOMEM;
  }

  memset(private_data, 0, sizeof(struct GeoArrowNativeWriterPrivate));

  GeoArrowErrorCode code = GeoArrowBuilderInitFromType(&private_data->builder, type);
  if (code != GEOARROW_OK) {
    ArrowFree(private_data);
    return code;
  }

  writer->private_data = private_data;
  return GEOARROW_OK;
}

void GeoArrowNativeWriterReset(struct GeoArrowNativeWriter* writer) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  GeoArrowBuilderReset(&private_data->builder);
  ArrowFree(private_data);
}
