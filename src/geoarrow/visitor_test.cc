#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

TEST(VisitorTest, VisitorTestVoid) {
  struct GeoArrowVisitor v;
  struct GeoArrowCoordView coords;
  coords.n_coords = 0;
  coords.n_values = 2;
  coords.coords_stride = 1;

  GeoArrowVisitorInitVoid(&v);
  EXPECT_EQ(v.private_data, nullptr);
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, &coords), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.error, nullptr);
}
