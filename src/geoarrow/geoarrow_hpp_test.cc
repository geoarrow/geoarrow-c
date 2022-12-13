
#include <gtest/gtest.h>

#include "geoarrow.hpp"

TEST(GeoArrowHppTest, GeoArrowHppTestVectorTypeMakeType) {
  auto type = geoarrow::VectorType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_TRUE(type.valid());
  EXPECT_EQ(type.extension_name(), "geoarrow.multipoint");
  EXPECT_EQ(type.extension_metadata(), "{}");
  EXPECT_EQ(type.id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(type.crs(), "");
}

TEST(GeoArrowHppTest, GeoArrowHppTestVectorTypeModify) {
  auto type = geoarrow::VectorType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_TRUE(type.valid());

  auto new_type = type.WithGeometryType(GEOARROW_GEOMETRY_TYPE_POINT);
  ASSERT_TRUE(new_type.valid());
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.crs(), "");

  new_type = type.WithDimensions(GEOARROW_DIMENSIONS_XYM);
  ASSERT_TRUE(new_type.valid());
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.crs(), "");

  new_type = type.WithEdgeType(GEOARROW_EDGE_TYPE_SPHERICAL);
  ASSERT_TRUE(new_type.valid());
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.crs(), "");

  new_type = type.WithCrs("some crs value");
  ASSERT_TRUE(new_type.valid());
  EXPECT_EQ(new_type.crs(), "some crs value");
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
}
