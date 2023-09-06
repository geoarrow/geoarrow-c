
#include <stdio.h>

#include "geoarrow_type.h"

#if defined(GEOARROW_USE_RYU) && GEOARROW_USE_RYU

#include "ryu/ryu2.h"

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result) {
  return d2fixed_buffered_n(f, precision, result);
}

#else

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result) {
  return snprintf(result, 128, "%.*g", precision, f);
}

#endif
