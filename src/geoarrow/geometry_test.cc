
#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "geoarrow/wkx_testing.hpp"

TEST(GeometryTest, GeometryTestInit) {
  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestInterleaved) {
  WKXTester tester;

  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

  struct GeoArrowGeometryNode* node;
  ASSERT_EQ(GeoArrowGeometryAppendNodeInline(&geom, &node), GEOARROW_OK);

  double coords[] = {10, 11, 12, 13, 14, 15, 16, 17};
  struct GeoArrowBufferView coords_view;
  coords_view.data = reinterpret_cast<const uint8_t*>(coords);
  coords_view.size_bytes = sizeof(coords);

  GeoArrowGeometryNodeSetInterleaved(node, GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                     GEOARROW_DIMENSIONS_XYZM, coords_view);
  ASSERT_EQ(GeoArrowGeometryVisit(&geom, tester.WKTVisitor()), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValue(), "LINESTRING ZM (10 11 12 13, 14 15 16 17)");

  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestSeparated) {
  WKXTester tester;

  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

  struct GeoArrowGeometryNode* node;
  ASSERT_EQ(GeoArrowGeometryAppendNodeInline(&geom, &node), GEOARROW_OK);

  double coords[] = {10, 11, 12, 13, 14, 15, 16, 17};
  struct GeoArrowBufferView coords_view;
  coords_view.data = reinterpret_cast<const uint8_t*>(coords);
  coords_view.size_bytes = sizeof(coords);

  GeoArrowGeometryNodeSetSeparated(node, GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                   GEOARROW_DIMENSIONS_XYZM, coords_view);
  ASSERT_EQ(GeoArrowGeometryVisit(&geom, tester.WKTVisitor()), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValue(), "LINESTRING ZM (10 12 14 16, 11 13 15 17)");

  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestShallowCopy) {
  WKXTester tester;

  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

  struct GeoArrowGeometryNode* node;
  ASSERT_EQ(GeoArrowGeometryAppendNodeInline(&geom, &node), GEOARROW_OK);

  double coords[] = {10, 11, 12, 13, 14, 15, 16, 17};
  struct GeoArrowBufferView coords_view;
  coords_view.data = reinterpret_cast<const uint8_t*>(coords);
  coords_view.size_bytes = sizeof(coords);

  GeoArrowGeometryNodeSetInterleaved(node, GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                     GEOARROW_DIMENSIONS_XYZM, coords_view);

  struct GeoArrowGeometry geom2;
  ASSERT_EQ(GeoArrowGeometryInit(&geom2), GEOARROW_OK);
  ASSERT_EQ(GeoArrowGeometryShallowCopy(GeoArrowGeometryAsView(&geom), &geom2),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowGeometryVisit(&geom2, tester.WKTVisitor()), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValue(), "LINESTRING ZM (10 11 12 13, 14 15 16 17)");

  GeoArrowGeometryReset(&geom2);
  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestDeepCopy) {
  WKXTester tester;

  double coords[] = {10, 11, 12, 13, 14, 15, 16, 17};
  struct GeoArrowBufferView coord1_view;
  coord1_view.data = reinterpret_cast<const uint8_t*>(coords);
  coord1_view.size_bytes = sizeof(coords) / 2;

  struct GeoArrowBufferView coord2_view;
  coord2_view.data = reinterpret_cast<const uint8_t*>(coords + 4);
  coord2_view.size_bytes = sizeof(coords) / 2;

  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

  // Use a multipoint to ensure that multiple nodes are supported
  struct GeoArrowGeometryNode* node;
  ASSERT_EQ(GeoArrowGeometryAppendNodeInline(&geom, &node), GEOARROW_OK);
  node->geometry_type = GEOARROW_GEOMETRY_TYPE_MULTIPOINT;
  node->dimensions = GEOARROW_DIMENSIONS_XYZM;
  node->size = 2;

  ASSERT_EQ(GeoArrowGeometryAppendNodeInline(&geom, &node), GEOARROW_OK);
  GeoArrowGeometryNodeSetInterleaved(node, GEOARROW_GEOMETRY_TYPE_POINT,
                                     GEOARROW_DIMENSIONS_XYZM, coord1_view);

  ASSERT_EQ(GeoArrowGeometryAppendNodeInline(&geom, &node), GEOARROW_OK);
  GeoArrowGeometryNodeSetInterleaved(node, GEOARROW_GEOMETRY_TYPE_POINT,
                                     GEOARROW_DIMENSIONS_XYZM, coord2_view);

  struct GeoArrowGeometry geom2;
  ASSERT_EQ(GeoArrowGeometryInit(&geom2), GEOARROW_OK);
  ASSERT_EQ(GeoArrowGeometryShallowCopy(GeoArrowGeometryAsView(&geom), &geom2),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowGeometryVisit(&geom2, tester.WKTVisitor()), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValue(), "MULTIPOINT ZM ((10 11 12 13), (14 15 16 17))");

  GeoArrowGeometryReset(&geom2);
  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestRoundtripWKT) {
  // These are also tested in the WKTFilesTest; however, we inline a fer here since
  // they are specifically testing the geometry constructor
  std::vector<std::string> wkts = {
      "POINT (30 10)", "POINT ZM (30 10 40 300)", "LINESTRING (30 10, 10 30, 40 40)",
      "LINESTRING ZM (30 10 40 300, 10 30 40 300, 40 40 80 1600)",
      "POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))",
      "MULTIPOINT ((10 40), (40 30), (20 20), (30 10))",
      "MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))",
      // Funny definition to avoid 'suspicous string literal' warning
      std::string("GEOMETRYCOLLECTION (GEOMETRYCOLLECTION (POINT (30 10), LINESTRING (30 "
                  "10, 10 30, ") +
          "40 40), POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10)), MULTIPOINT ((30 10)), "
          "MULTILINESTRING ((30 10, 10 30, 40 40)), MULTIPOLYGON (((30 10, 40 40, 20 40, "
          "10 "
          "20, 30 10)))))"};

  for (const auto& wkt : wkts) {
    SCOPED_TRACE(wkt);
    WKXTester tester;

    struct GeoArrowGeometry geom;
    ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

    struct GeoArrowVisitor v;
    struct GeoArrowError error;
    v.error = &error;
    GeoArrowGeometryInitVisitor(&geom, &v);

    tester.ReadWKT(wkt, &v);
    ASSERT_EQ(GeoArrowGeometryVisit(&geom, tester.WKTVisitor()), GEOARROW_OK);
    EXPECT_EQ(tester.WKTValue(), wkt);

    GeoArrowGeometryReset(&geom);
  }
}
