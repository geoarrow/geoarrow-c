
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.hpp"

TEST(GeoArrowHppTest, GeometryDataTypeMakeType) {
  auto type = geoarrow::GeometryDataType::Make(GEOARROW_TYPE_MULTIPOINT);
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

TEST(GeoArrowHppTest, GeometryDataTypeMakeTypeFromSchema) {
  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);
  auto type = geoarrow::GeometryDataType::Make(&schema);
  ASSERT_EQ(type.id(), GEOARROW_TYPE_POINT);
  ArrowSchemaRelease(&schema);

  type.WithCrsLonLat().InitSchema(&schema);
  type = geoarrow::GeometryDataType::Make(&schema);
  ASSERT_EQ(type.crs_type(), GEOARROW_CRS_TYPE_PROJJSON);
  ASSERT_GT(type.crs().size(), 0);
  ArrowSchemaRelease(&schema);
}

TEST(GeoArrowHppTest, GeometryDataTypeMakeTypeErrors) {
  EXPECT_THROW(geoarrow::GeometryDataType::Make(nullptr), geoarrow::ErrnoException);
  EXPECT_THROW(geoarrow::GeometryDataType::Make(GEOARROW_TYPE_UNINITIALIZED),
               geoarrow::Exception);
  EXPECT_THROW(geoarrow::GeometryDataType::Make(GEOARROW_TYPE_POINT, "foofyfoofyfoofy"),
               geoarrow::ErrnoException);
}

TEST(GeoArrowHppTest, GeometryDataTypeModify) {
  auto type = geoarrow::GeometryDataType::Make(GEOARROW_TYPE_MULTIPOINT);

  auto new_type = type.WithGeometryType(GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.crs(), "");

  new_type = type.WithDimensions(GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.crs(), "");

  new_type = type.WithEdgeType(GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.crs(), "");

  new_type = type.WithCrs("some crs value");
  EXPECT_EQ(new_type.crs(), "some crs value");
  EXPECT_EQ(new_type.crs_type(), GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(new_type.geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
}

TEST(GeoArrowHppTest, GeometryDataTypeModifyBox) {
  EXPECT_EQ(geoarrow::Box().XYZ().id(), GEOARROW_TYPE_BOX_Z);
  EXPECT_EQ(geoarrow::Box().XYM().id(), GEOARROW_TYPE_BOX_M);
  EXPECT_EQ(geoarrow::Box().XYZM().id(), GEOARROW_TYPE_BOX_ZM);
  EXPECT_EQ(geoarrow::Box().XYZ().XY().id(), GEOARROW_TYPE_BOX);
}

TEST(GeoArrowHppTest, GeometryDataTypeModifyXYZM) {
  EXPECT_EQ(geoarrow::Point().num_dimensions(), 2);
  EXPECT_EQ(geoarrow::Point().XYZ().id(), GEOARROW_TYPE_POINT_Z);
  EXPECT_EQ(geoarrow::Point().XYZ().num_dimensions(), 3);
  EXPECT_EQ(geoarrow::Point().XYM().id(), GEOARROW_TYPE_POINT_M);
  EXPECT_EQ(geoarrow::Point().XYM().num_dimensions(), 3);
  EXPECT_EQ(geoarrow::Point().XYZM().id(), GEOARROW_TYPE_POINT_ZM);
  EXPECT_EQ(geoarrow::Point().XYZM().num_dimensions(), 4);
  EXPECT_EQ(geoarrow::Point().XYZ().XY().id(), GEOARROW_TYPE_POINT);
}

TEST(GeoArrowHppTest, GeometryDataTypeModifyMultipoint) {
  EXPECT_EQ(geoarrow::Point().Multi().id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(geoarrow::Point().Multi().Multi().id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(geoarrow::Point().Simple().id(), GEOARROW_TYPE_POINT);
  EXPECT_EQ(geoarrow::Point().Multi().Simple().id(), GEOARROW_TYPE_POINT);
}

TEST(GeoArrowHppTest, GeometryDataTypeModifyMultilinestring) {
  EXPECT_EQ(geoarrow::Linestring().Multi().id(), GEOARROW_TYPE_MULTILINESTRING);
  EXPECT_EQ(geoarrow::Linestring().Multi().Multi().id(), GEOARROW_TYPE_MULTILINESTRING);
  EXPECT_EQ(geoarrow::Linestring().Simple().id(), GEOARROW_TYPE_LINESTRING);
  EXPECT_EQ(geoarrow::Linestring().Multi().Simple().id(), GEOARROW_TYPE_LINESTRING);
}

TEST(GeoArrowHppTest, GeometryDataTypeModifyMultipolygon) {
  EXPECT_EQ(geoarrow::Polygon().Multi().id(), GEOARROW_TYPE_MULTIPOLYGON);
  EXPECT_EQ(geoarrow::Polygon().Multi().Multi().id(), GEOARROW_TYPE_MULTIPOLYGON);
  EXPECT_EQ(geoarrow::Polygon().Simple().id(), GEOARROW_TYPE_POLYGON);
  EXPECT_EQ(geoarrow::Polygon().Multi().Simple().id(), GEOARROW_TYPE_POLYGON);
}

TEST(GeoArrowHppTest, GeometryDataTypeModifyErrors) {
  EXPECT_THROW(geoarrow::Box().WithCoordType(GEOARROW_COORD_TYPE_INTERLEAVED),
               geoarrow::Exception);
  EXPECT_THROW(geoarrow::Box().Simple(), geoarrow::Exception);
  EXPECT_THROW(geoarrow::Box().Multi(), geoarrow::Exception);
  EXPECT_THROW(geoarrow::GeometryDataType().extension_name(), geoarrow::Exception);
}

TEST(GeoArrowHppTest, SerializedAccessors) {
  EXPECT_EQ(geoarrow::Wkt().geometry_type(), GEOARROW_GEOMETRY_TYPE_GEOMETRY);
  EXPECT_EQ(geoarrow::Wkt().coord_type(), GEOARROW_COORD_TYPE_UNKNOWN);
  EXPECT_EQ(geoarrow::Wkt().dimensions(), GEOARROW_DIMENSIONS_UNKNOWN);
  EXPECT_EQ(geoarrow::Wkt().num_dimensions(), -1);
}

TEST(GeoArrowHppTest, TypeConstructors) {
  EXPECT_EQ(geoarrow::Wkb().id(), GEOARROW_TYPE_WKB);
  EXPECT_EQ(geoarrow::Wkt().id(), GEOARROW_TYPE_WKT);
  EXPECT_EQ(geoarrow::Point().id(), GEOARROW_TYPE_POINT);
  EXPECT_EQ(geoarrow::Linestring().id(), GEOARROW_TYPE_LINESTRING);
  EXPECT_EQ(geoarrow::Polygon().id(), GEOARROW_TYPE_POLYGON);
}

TEST(GeoArrowHppTest, SerializedToString) {
  EXPECT_EQ(geoarrow::Wkt().ToString(), "geoarrow.wkt");
  EXPECT_EQ(geoarrow::Wkb().ToString(), "geoarrow.wkb");
  EXPECT_EQ(geoarrow::GeometryDataType::Make(GEOARROW_TYPE_LARGE_WKT).ToString(),
            "large geoarrow.wkt");
  EXPECT_EQ(geoarrow::GeometryDataType::Make(GEOARROW_TYPE_LARGE_WKB).ToString(),
            "large geoarrow.wkb");
}

TEST(GeoArrowHppTest, NativeToString) {
  EXPECT_EQ(geoarrow::Point().ToString(), "geoarrow.point");
  EXPECT_EQ(geoarrow::Linestring().ToString(), "geoarrow.linestring");
  EXPECT_EQ(geoarrow::Polygon().ToString(), "geoarrow.polygon");
  EXPECT_EQ(geoarrow::Point().Multi().ToString(), "geoarrow.multipoint");
  EXPECT_EQ(geoarrow::Linestring().Multi().ToString(), "geoarrow.multilinestring");
  EXPECT_EQ(geoarrow::Polygon().Multi().ToString(), "geoarrow.multipolygon");
}

TEST(GeoArrowHppTest, InterleavedToString) {
  EXPECT_EQ(geoarrow::Point().WithCoordType(GEOARROW_COORD_TYPE_INTERLEAVED).ToString(),
            "interleaved geoarrow.point");
}

TEST(GeoArrowHppTest, NonPlanarToString) {
  EXPECT_EQ(geoarrow::Linestring().WithEdgeType(GEOARROW_EDGE_TYPE_SPHERICAL).ToString(),
            "spherical geoarrow.linestring");
}

TEST(GeoArrowHppTest, DimensionsToString) {
  EXPECT_EQ(geoarrow::Point().XY().ToString(), "geoarrow.point");
  EXPECT_EQ(geoarrow::Point().XYZ().ToString(), "geoarrow.point_z");
  EXPECT_EQ(geoarrow::Point().XYM().ToString(), "geoarrow.point_m");
  EXPECT_EQ(geoarrow::Point().XYZM().ToString(), "geoarrow.point_zm");
}

TEST(GeoArrowHppTest, CRSToString) {
  EXPECT_EQ(geoarrow::Wkt().WithCrs("foofy").ToString(), "geoarrow.wkt<foofy>");
  EXPECT_EQ(geoarrow::Wkt().WithCrsLonLat().ToString(),
            R"(geoarrow.wkt<projjson:{"type":"GeographicCRS","n...>)");
}
