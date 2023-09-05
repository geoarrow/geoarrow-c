
#include "fast_float.h"
#include "geoarrow_type.h"

extern "C" GeoArrowErrorCode GeoArrowFromChars(struct GeoArrowStringView src,
                                               double* out) {
  auto answer = fast_float::from_chars(src.data, src.data + src.size_bytes, *out);
  if (answer.ec != std::errc()) {
    return EINVAL;
  } else {
    return GEOARROW_OK;
  }
}
