
#include "geoarrow/geoarrow_type.h"

#if defined(GEOARROW_USE_FAST_FLOAT) && GEOARROW_USE_FAST_FLOAT

#include "fast_float.h"

extern "C" GeoArrowErrorCode GeoArrowFromChars(const char* first, const char* last,
                                               double* out, const char** end) {
#ifdef GEOARROW_NAMESPACE
  namespace ff = GEOARROW_NAMESPACE::fast_float;
#else
  namespace ff = fast_float;
#endif
  auto answer =
      ff::from_chars(first, last, *out,
                     ff::chars_format::general | ff::chars_format::allow_leading_plus);
  if (answer.ec != std::errc()) {
    return EINVAL;
  }

  *end = answer.ptr;
  return GEOARROW_OK;
}

#endif
