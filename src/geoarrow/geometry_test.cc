
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
