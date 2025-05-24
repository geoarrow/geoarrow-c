
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "geoarrow/geoarrow.h"

const char* GeoArrowVersion(void) { return GEOARROW_VERSION; }

int GeoArrowVersionInt(void) { return GEOARROW_VERSION_INT; }

GeoArrowErrorCode GeoArrowErrorSet(struct GeoArrowError* error, const char* fmt, ...) {
  if (error == NULL) {
    return GEOARROW_OK;
  }

  memset(error->message, 0, sizeof(error->message));

  va_list args;
  va_start(args, fmt);
  int chars_needed = vsnprintf(error->message, sizeof(error->message), fmt, args);
  va_end(args);

  if (chars_needed < 0) {
    return EINVAL;
  } else if (((size_t)chars_needed) >= sizeof(error->message)) {
    return ERANGE;
  } else {
    return GEOARROW_OK;
  }
}
