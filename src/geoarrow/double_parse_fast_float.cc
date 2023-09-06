
#include "fast_float.h"
#include "geoarrow_type.h"

extern "C" GeoArrowErrorCode GeoArrowFromChars(const char* first, const char* last,
                                               double* out) {
  auto answer = fast_float::from_chars(first, last, *out);
  if (answer.ec != std::errc()) {
    return EINVAL;
  } else {
    return GEOARROW_OK;
  }
}
