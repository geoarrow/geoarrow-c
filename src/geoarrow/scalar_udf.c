
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

GeoArrowErrorCode GeoArrowScalarUdfFactoryInit(struct GeoArrowScalarUdfFactory* out,
                                               const char* name, const char* options) {
  GEOARROW_UNUSED(out);
  GEOARROW_UNUSED(name);
  GEOARROW_UNUSED(options);
  return ENOTSUP;
}
