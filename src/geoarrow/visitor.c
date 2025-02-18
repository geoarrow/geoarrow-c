
#include <stddef.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

static int feat_start_void(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int null_feat_void(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int geom_start_void(struct GeoArrowVisitor* v,
                           enum GeoArrowGeometryType geometry_type,
                           enum GeoArrowDimensions dimensions) {
  NANOARROW_UNUSED(v);
  NANOARROW_UNUSED(geometry_type);
  NANOARROW_UNUSED(dimensions);
  return GEOARROW_OK;
}

static int ring_start_void(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int coords_void(struct GeoArrowVisitor* v,
                       const struct GeoArrowCoordView* coords) {
  NANOARROW_UNUSED(v);
  NANOARROW_UNUSED(coords);
  return GEOARROW_OK;
}

static int ring_end_void(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int geom_end_void(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int feat_end_void(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

void GeoArrowVisitorInitVoid(struct GeoArrowVisitor* v) {
  v->feat_start = &feat_start_void;
  v->null_feat = &null_feat_void;
  v->geom_start = &geom_start_void;
  v->ring_start = &ring_start_void;
  v->coords = &coords_void;
  v->ring_end = &ring_end_void;
  v->geom_end = &geom_end_void;
  v->feat_end = &feat_end_void;
  v->error = NULL;
  v->private_data = NULL;
}
