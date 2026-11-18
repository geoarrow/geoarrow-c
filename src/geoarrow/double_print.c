
#include "geoarrow/geoarrow_type.h"

#include <stdio.h>

#if defined(GEOARROW_USE_RYU) && GEOARROW_USE_RYU

#include "ryu/ryu.h"

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result) {
  if (precision == UINT32_MAX) {
    // Use fixed notation where 20 decimal places are sufficient for all 17
    // significant digits, and scientific notation outside that range. Ryu's
    // conversion emits the shortest representation that round-trips.
    if ((f != 0 && f > -1.0e-4 && f < 1.0e-4) || f > 1.0e17 || f < -1.0e17) {
      return GeoArrowd2sexp_buffered_n(f, 17, result);
    } else {
      return GeoArrowd2sfixed_buffered_n(f, 20, result);
    }
  }

  // Use exponential to serialize very large numbers in scientific notation
  // and ignore user precision for these cases.
  if (f > 1.0e17 || f < -1.0e17) {
    return GeoArrowd2sexp_buffered_n(f, 17, result);
  } else {
    // Note: d2sfixed_buffered_n() may write up to 310 characters into result
    // for the case where f is the minimum possible double value.
    return GeoArrowd2sfixed_buffered_n(f, precision, result);
  }
}

#else

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result) {
  if (precision == UINT32_MAX) {
    // Seventeen significant decimal digits are sufficient to round-trip an
    // IEEE-754 double. %g also avoids wasting precision on leading zeroes.
    return snprintf(result, 40, "%.17g", f);
  }

  // For very large numbers, use scientific notation ignoring user precision
  if (f > 1.0e17 || f < -1.0e17) {
    return snprintf(result, 40, "%0.*e", 16, f);
  }

  int64_t n_chars = snprintf(result, 40, "%0.*f", precision, f);
  if (n_chars > 39) {
    n_chars = 39;
  }

  // Strip trailing zeroes + decimal
  for (int64_t i = n_chars - 1; i >= 0; i--) {
    if (result[i] == '0') {
      n_chars--;
    } else if (result[i] == '.') {
      n_chars--;
      break;
    } else {
      break;
    }
  }

  return n_chars;
}

#endif
