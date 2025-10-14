
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

static void GeoArrowScalarUdfVoidFactoryInit(struct GeoArrowScalarUdfFactory* self,
                                             struct GeoArrowScalarUdf* out);

static void GeoArrowScalarUdfVoidFactoryRelease(struct GeoArrowScalarUdfFactory* self) {
  GEOARROW_UNUSED(self);
}

GeoArrowErrorCode GeoArrowScalarUdfFactoryInit(struct GeoArrowScalarUdfFactory* out,
                                               const char* name, const char* options,
                                               struct GeoArrowError* error) {
  GEOARROW_UNUSED(out);
  GEOARROW_UNUSED(name);
  GEOARROW_UNUSED(options);

  if (strcmp(name, "void") == 0) {
    out->new_scalar_udf_impl = &GeoArrowScalarUdfVoidFactoryInit;
  } else {
    GeoArrowErrorSet(
        error, "GeoArrow C scalar implementation with name '%s' does not exist", name);
    return ENOTSUP;
  }

  out->private_data = NULL;
  out->release = &GeoArrowScalarUdfVoidFactoryRelease;
  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowScalarUdfVoidInit(struct GeoArrowScalarUdf* self,
                                                   const struct ArrowSchema** arg_types,
                                                   struct ArrowArray** scalar_args,
                                                   int64_t n_args,
                                                   struct ArrowSchema* out) {
  GEOARROW_UNUSED(self);
  GEOARROW_UNUSED(arg_types);
  GEOARROW_UNUSED(scalar_args);
  GEOARROW_UNUSED(n_args);

  GEOARROW_RETURN_NOT_OK(ArrowSchemaInitFromType(out, NANOARROW_TYPE_NA));
  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowScalarUdfVoidExecute(struct GeoArrowScalarUdf* self,
                                                      struct ArrowArray** args,
                                                      int64_t n_args, int64_t n_rows,
                                                      struct ArrowArray* out) {
  GEOARROW_UNUSED(self);
  GEOARROW_UNUSED(args);
  GEOARROW_UNUSED(n_args);

  GEOARROW_RETURN_NOT_OK(ArrowArrayInitFromType(out, NANOARROW_TYPE_NA));
  out->length = n_rows;
  out->null_count = n_rows;
  GEOARROW_RETURN_NOT_OK(ArrowArrayFinishBuildingDefault(out, NULL));

  return GEOARROW_OK;
}

static const char* GeoArrowScalarUdfVoidGetLastError(struct GeoArrowScalarUdf* self) {
  GEOARROW_UNUSED(self);
  return NULL;
}

static void GeoArrowScalarUdfVoidRelease(struct GeoArrowScalarUdf* self) {
  GEOARROW_UNUSED(self);
}

static void GeoArrowScalarUdfVoidFactoryInit(struct GeoArrowScalarUdfFactory* self,
                                             struct GeoArrowScalarUdf* out) {
  GEOARROW_UNUSED(self);
  out->init = &GeoArrowScalarUdfVoidInit;
  out->execute = &GeoArrowScalarUdfVoidExecute;
  out->get_last_error = &GeoArrowScalarUdfVoidGetLastError;
  out->release = &GeoArrowScalarUdfVoidRelease;
}
