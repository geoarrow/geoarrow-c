
#include "geoarrow_type.h"

#if defined(GEOARROW_USE_RYU) && GEOARROW_USE_RYU

#include "ryu/ryu.h"

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result) {
  return GeoArrowd2sfixed_buffered_n(f, precision, result);
}

#else

#include <stdio.h>

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result) {
  snprintf(result, 128, "%.*g", precision, f);
}

#endif
