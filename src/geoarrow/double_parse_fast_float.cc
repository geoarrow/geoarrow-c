
#include "geoarrow/geoarrow_type.h"

#if defined(GEOARROW_USE_FAST_FLOAT) && GEOARROW_USE_FAST_FLOAT

#include "fast_float.h"

extern "C" GeoArrowErrorCode GeoArrowFromChars(const char* first, const char* last,
                                               double* out) {
#ifdef GEOARROW_NAMESPACE
  auto answer = GEOARROW_NAMESPACE::fast_float::from_chars(first, last, *out);
#else
  auto answer = fast_float::from_chars(first, last, *out);
#endif

  if (answer.ec != std::errc()) {
    return EINVAL;
  } else {
    return GEOARROW_OK;
  }
}

// Internal prefix parser used by the WKT reader. Keep GeoArrowFromChars()'s
// public signature and token-bounded callers unchanged.
extern "C" GeoArrowErrorCode GeoArrowFromCharsPartial(const char* first, const char* last,
                                                      double* out, const char** end) {
#ifdef GEOARROW_NAMESPACE
  auto answer = GEOARROW_NAMESPACE::fast_float::from_chars(first, last, *out);
#else
  auto answer = fast_float::from_chars(first, last, *out);
#endif
  if (answer.ec != std::errc()) {
    return EINVAL;
  }

  *end = answer.ptr;
  return GEOARROW_OK;
}

#endif
