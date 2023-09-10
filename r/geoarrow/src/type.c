#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include "geoarrow.h"

SEXP geoarrow_c_make_type(SEXP geometry_type_sexp, SEXP dimensions_sexp,
                          SEXP coord_type_sexp) {
  enum GeoArrowType out =
      GeoArrowMakeType((enum GeoArrowGeometryType)INTEGER(geometry_type_sexp)[0],
                       (enum GeoArrowDimensions)INTEGER(dimensions_sexp)[0],
                       (enum GeoArrowCoordType)INTEGER(coord_type_sexp)[0]);

  return Rf_ScalarInteger(out);
}

SEXP geoarrow_c_schema_init_extension(SEXP schema_xptr, SEXP type_sexp) {
  struct ArrowSchema* schema = (struct ArrowSchema*)R_ExternalPtrAddr(schema_xptr);
  enum GeoArrowType type_id = (enum GeoArrowType)INTEGER(type_sexp)[0];

  int result = GeoArrowSchemaInitExtension(schema, type_id);
  if (result != GEOARROW_OK) {
    Rf_error("[GeoArrowSchemaInitExtension][%d] type_id not valid", result);
  }

  return R_NilValue;
}
