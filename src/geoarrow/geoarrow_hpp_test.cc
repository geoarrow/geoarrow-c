
#include <gtest/gtest.h>

#include <geoarrow.hpp>

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeMakeType) {
  auto type = geoarrow::GeometryDataType::Make(GEOARROW_TYPE_MULTIPOINT);
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

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeModify) {
  auto type = geoarrow::GeometryDataType::Make(GEOARROW_TYPE_MULTIPOINT);
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

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeModifyBox) {
  EXPECT_EQ(geoarrow::Box().XYZ().id(), GEOARROW_TYPE_BOX_Z);
  EXPECT_EQ(geoarrow::Box().XYM().id(), GEOARROW_TYPE_BOX_M);
  EXPECT_EQ(geoarrow::Box().XYZM().id(), GEOARROW_TYPE_BOX_ZM);
  EXPECT_EQ(geoarrow::Box().XYZ().XY().id(), GEOARROW_TYPE_BOX);

  EXPECT_FALSE(geoarrow::Box().WithCoordType(GEOARROW_COORD_TYPE_INTERLEAVED).valid());
}

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeModifyXYZM) {
  EXPECT_EQ(geoarrow::Point().XYZ().id(), GEOARROW_TYPE_POINT_Z);
  EXPECT_EQ(geoarrow::Point().XYM().id(), GEOARROW_TYPE_POINT_M);
  EXPECT_EQ(geoarrow::Point().XYZM().id(), GEOARROW_TYPE_POINT_ZM);
  EXPECT_EQ(geoarrow::Point().XYZ().XY().id(), GEOARROW_TYPE_POINT);
}

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeModifyMultipoint) {
  EXPECT_EQ(geoarrow::Point().Multi().id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(geoarrow::Point().Multi().Multi().id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(geoarrow::Point().Simple().id(), GEOARROW_TYPE_POINT);
  EXPECT_EQ(geoarrow::Point().Multi().Simple().id(), GEOARROW_TYPE_POINT);
}

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeModifyMultilinestring) {
  EXPECT_EQ(geoarrow::Linestring().Multi().id(), GEOARROW_TYPE_MULTILINESTRING);
  EXPECT_EQ(geoarrow::Linestring().Multi().Multi().id(), GEOARROW_TYPE_MULTILINESTRING);
  EXPECT_EQ(geoarrow::Linestring().Simple().id(), GEOARROW_TYPE_LINESTRING);
  EXPECT_EQ(geoarrow::Linestring().Multi().Simple().id(), GEOARROW_TYPE_LINESTRING);
}

TEST(GeoArrowHppTest, GeoArrowHppTestGeometryDataTypeModifyMultipolygon) {
  EXPECT_EQ(geoarrow::Polygon().Multi().id(), GEOARROW_TYPE_MULTIPOLYGON);
  EXPECT_EQ(geoarrow::Polygon().Multi().Multi().id(), GEOARROW_TYPE_MULTIPOLYGON);
  EXPECT_EQ(geoarrow::Polygon().Simple().id(), GEOARROW_TYPE_POLYGON);
  EXPECT_EQ(geoarrow::Polygon().Multi().Simple().id(), GEOARROW_TYPE_POLYGON);
}

TEST(GeoArrowHppTest, GeoArrowHppTestTypeConstructors) {
  EXPECT_EQ(geoarrow::Wkb().id(), GEOARROW_TYPE_WKB);
  EXPECT_EQ(geoarrow::Wkt().id(), GEOARROW_TYPE_WKT);
  EXPECT_EQ(geoarrow::Point().id(), GEOARROW_TYPE_POINT);
  EXPECT_EQ(geoarrow::Linestring().id(), GEOARROW_TYPE_LINESTRING);
  EXPECT_EQ(geoarrow::Polygon().id(), GEOARROW_TYPE_POLYGON);
}

TEST(GeoArrowHppTest, GeoArrowHppTestModifyInvalid) {
  auto invalid = geoarrow::GeometryDataType::Invalid();
  EXPECT_FALSE(invalid.WithGeometryType(GEOARROW_GEOMETRY_TYPE_GEOMETRY).valid());
  EXPECT_FALSE(invalid.WithDimensions(GEOARROW_DIMENSIONS_XY).valid());
  EXPECT_FALSE(invalid.WithEdgeType(GEOARROW_EDGE_TYPE_SPHERICAL).valid());
  EXPECT_FALSE(invalid.WithCrs("abcd1234").valid());
}

TEST(GeoArrowHppTest, GeoArrowHppTestVectorArray) {
  geoarrow::VectorArray array(geoarrow::GeometryDataType::Invalid("some message"));
  EXPECT_FALSE(array.valid());
  EXPECT_EQ(array.error(), "some message");
  EXPECT_EQ(array->release, nullptr);

  geoarrow::VectorArray array2(geoarrow::Point());
  EXPECT_FALSE(array2.valid());
  EXPECT_EQ(array2.error(), "VectorArray is released");
}

TEST(GeoArrowHppTest, GeoArrowHppTestArrayFromVectors) {
  auto array =
      geoarrow::ArrayFromVectors(geoarrow::Point(), {{1, 2, 3, 4}, {5, 6, 7, 8}});

  EXPECT_TRUE(array.valid());
  EXPECT_EQ(array.type().id(), GEOARROW_TYPE_POINT);
  ASSERT_EQ(array.view()->length[0], 4);
  ASSERT_EQ(array.view()->coords.n_coords, 4);
  ASSERT_EQ(array.view()->coords.n_values, 2);
  EXPECT_EQ(array.view()->coords.values[0][0], 1);
  EXPECT_EQ(array.view()->coords.values[1][0], 5);

  // Check that this can be moved to a new home
  geoarrow::VectorArray array2 = std::move(array);
  EXPECT_EQ(array->release, nullptr);
  EXPECT_FALSE(array.valid());
  EXPECT_TRUE(array2.valid());
  EXPECT_EQ(array2.type().id(), GEOARROW_TYPE_POINT);
  ASSERT_EQ(array2.view()->length[0], 4);
  ASSERT_EQ(array2.view()->coords.n_coords, 4);
  ASSERT_EQ(array2.view()->coords.n_values, 2);
  EXPECT_EQ(array2.view()->coords.values[0][0], 1);
  EXPECT_EQ(array2.view()->coords.values[1][0], 5);
}
