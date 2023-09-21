#define R_NO_REMAP
#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>

SEXP geoarrow_c_handle_stream(SEXP data, SEXP handler_xptr);
SEXP geoarrow_c_make_type(SEXP geometry_type_sexp, SEXP dimensions_sexp,
                          SEXP coord_type_sexp);
SEXP geoarrow_c_schema_init_extension(SEXP schema_xptr, SEXP type_sexp);
SEXP geoarrow_c_writer_new(SEXP schema_xptr, SEXP array_out_xptr);
SEXP geoarrow_c_schema_parse(SEXP schema_xptr, SEXP extension_name_sexp);
SEXP geoarrow_c_kernel(SEXP kernel_name_sexp, SEXP arg_types_sexp, SEXP options_sexp,
                       SEXP schema_out_xptr);

static const R_CallMethodDef CallEntries[] = {
    {"geoarrow_c_handle_stream", (DL_FUNC)&geoarrow_c_handle_stream, 2},
    {"geoarrow_c_make_type", (DL_FUNC)&geoarrow_c_make_type, 3},
    {"geoarrow_c_schema_init_extension", (DL_FUNC)&geoarrow_c_schema_init_extension, 2},
    {"geoarrow_c_writer_new", (DL_FUNC)&geoarrow_c_writer_new, 2},
    {"geoarrow_c_schema_parse", (DL_FUNC)&geoarrow_c_schema_parse, 2},
    {"geoarrow_c_kernel", (DL_FUNC)&geoarrow_c_kernel, 4},
    {NULL, NULL, 0}};

void R_init_geoarrow(DllInfo* dll) {
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}
