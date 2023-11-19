#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include "geoarrow.h"
#include "nanoarrow.h"

static inline int build_inner_offsets(SEXP item, struct GeoArrowBuilder* builder,
                                      int level, int32_t* current_offsets) {
  if (level >= builder->view.n_offsets) {
    Rf_error("Unexpected level of nesting whilst buliding offset buffers from sfc");
  }

  switch (TYPEOF(item)) {
    // Level of nesting
    case VECSXP: {
      int32_t n = Rf_length(item);
      current_offsets[level] += n;
      NANOARROW_RETURN_NOT_OK(
          GeoArrowBuilderOffsetAppend(builder, level, current_offsets + level, 1));
      for (int32_t i = 0; i < n; i++) {
        build_inner_offsets(item, builder, level + 1, current_offsets);
      }
      break;
    }

    // Matrix containing a coordinate sequence
    case REALSXP: {
      int32_t n = Rf_nrows(item);
      current_offsets[level] += n;
      NANOARROW_RETURN_NOT_OK(
          GeoArrowBuilderOffsetAppend(builder, level, current_offsets + level, 1));
      break;
    }

    default:
      Rf_error("Unexpected element whilst building offset buffers from sfc");
  }

  return GEOARROW_OK;
}

static int build_offsets(SEXP sfc, struct GeoArrowBuilder* builder) {
  R_xlen_t n = Rf_xlength(sfc);

  // Keep track of current last value
  int32_t current_offsets[] = {0, 0, 0};

  // Append initial 0 to the offset buffers and reserve memory for their minimum
  // size.
  for (int i = 1; i < builder->view.n_offsets; i++) {
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(builder, i, current_offsets + i, 1));
    NANOARROW_RETURN_NOT_OK(GeoArrowBuilderOffsetReserve(builder, i, n + 1));
  }

  for (R_xlen_t i = 0; i < n; i++) {
    SEXP item = VECTOR_ELT(sfc, i);
    NANOARROW_RETURN_NOT_OK(build_inner_offsets(item, builder, 0, current_offsets));
  }

  builder->view.length = n;
  return GEOARROW_OK;
}

static void finalize_builder_xptr(SEXP builder_xptr) {
  struct GeoArrowBuilder* builder =
      (struct GeoArrowBuilder*)R_ExternalPtrAddr(builder_xptr);
  if (builder != NULL && builder->private_data != NULL) {
    GeoArrowBuilderReset(builder);
  }

  if (builder != NULL) {
    free(builder);
  }
}

SEXP geoarrow_c_as_nanoarrow_array_sfc(SEXP sfc, SEXP schema_xptr, SEXP array_xptr) {
  struct ArrowSchema* schema = (struct ArrowSchema*)R_ExternalPtrAddr(schema_xptr);
  struct ArrowArray* array = (struct ArrowArray*)R_ExternalPtrAddr(schema_xptr);

  // Use external pointer finalizer to ensure builder is cleaned up
  struct GeoArrowBuilder* builder =
      (struct GeoArrowBuilder*)malloc(sizeof(struct GeoArrowBuilder));
  if (builder == NULL) {
    Rf_error("Failed to allocate for GeoArrowBuilder");
  }
  builder->private_data = NULL;
  SEXP builder_xptr = PROTECT(R_MakeExternalPtr(builder, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(builder_xptr, &finalize_builder_xptr);

  struct GeoArrowError error;
  error.message[0] = '\0';

  // Initialize the builder
  int result = GeoArrowBuilderInitFromSchema(builder, schema, &error);
  if (result != GEOARROW_OK) {
    Rf_error("GeoArrowBuilderInitFromSchema() failed: %s", error.message);
  }

  // Build the offset buffers from the various layers of nesting
  result = build_offsets(sfc, builder);
  if (result != GEOARROW_OK) {
    Rf_error("build_offsets() failed to allocate memory for offset buffers");
  }

  // Set coordinate buffers

  // Build result
  //   result = GeoArrowBuilderFinish(builder, array, &error);
  //   if (result != GEOARROW_OK) {
  //     Rf_error("GeoArrowBuilderFinish() failed: %s", error.message);
  //   }

  UNPROTECT(1);
  return R_NilValue;
}
