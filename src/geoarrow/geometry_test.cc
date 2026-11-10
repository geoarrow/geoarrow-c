
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
  ASSERT_EQ(GeoArrowGeometryDeepCopy(GeoArrowGeometryAsView(&geom), &geom2), GEOARROW_OK);
  ASSERT_EQ(GeoArrowGeometryVisit(&geom2, tester.WKTVisitor()), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValue(), "MULTIPOINT ZM ((10 11 12 13), (14 15 16 17))");

  GeoArrowGeometryReset(&geom2);
  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestSetBox) {
  WKXTester tester;
  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

  ASSERT_EQ(GeoArrowGeometrySetBox(&geom, 1, 2, 3, 4), GEOARROW_OK);
  EXPECT_EQ(tester.AsWKT(geom), "POLYGON ((1 2, 3 2, 3 4, 1 4, 1 2))");

  ASSERT_EQ(GeoArrowGeometrySetBox(&geom, 170, -10, -170, 20), GEOARROW_OK);
  EXPECT_EQ(tester.AsWKT(geom),
            "MULTIPOLYGON (((170 -10, inf -10, inf 20, 170 20, 170 -10)), "
            "((-inf -10, -170 -10, -170 20, -inf 20, -inf -10)))");

  ASSERT_EQ(GeoArrowGeometrySetBox(&geom, 1, 4, 3, 2), GEOARROW_OK);
  EXPECT_EQ(tester.AsWKT(geom), "POLYGON EMPTY");

  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryTestRoundtripWKT) {
  // These are also tested in the WKXFilesTest; however, we inline a few here since
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

TEST(GeometryTest, GeometryFromVisitorErrors) {
  struct GeoArrowGeometry geom;
  struct GeoArrowVisitor v;
  struct GeoArrowError error;
  GeoArrowGeometryInit(&geom);
  GeoArrowGeometryInitVisitor(&geom, &v);
  v.error = &error;

  struct GeoArrowCoordView coords;
  coords.n_coords = 0;
  coords.n_values = 2;
  coords.coords_stride = 1;

  // Invalid because level < 0
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), EINVAL);
  EXPECT_EQ(v.ring_end(&v), EINVAL);
  EXPECT_EQ(v.coords(&v, &coords), EINVAL);
  EXPECT_EQ(v.geom_end(&v), EINVAL);
  EXPECT_EQ(v.feat_end(&v), EINVAL);

  // Invalid because of too much nesting
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  for (int i = 0; i < 31; i++) {
    EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
              GEOARROW_OK);
  }
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            EINVAL);

  GeoArrowGeometryReset(&geom);
}

TEST(GeometryTest, GeometryViewValidate) {
  WKXTester tester;
  GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  for (const char* wkt :
       {"POINT (1 2)", "POINT Z (1 2 3)", "POINT M (1 2 3)", "POINT ZM (1 2 3 4)",
        "POINT EMPTY", "LINESTRING EMPTY", "LINESTRING (1 2)", "POLYGON (EMPTY, (1 2))",
        "MULTIPOINT (EMPTY, (1 2))", "MULTILINESTRING (EMPTY, (1 2))",
        "MULTIPOLYGON (EMPTY, ((1 2)))", "GEOMETRYCOLLECTION EMPTY",
        "GEOMETRYCOLLECTION (POINT EMPTY, GEOMETRYCOLLECTION (LINESTRING (1 2)))"}) {
    SCOPED_TRACE(wkt);
    auto wkb = tester.AsWKB(wkt);
    GeoArrowGeometryView view;
    ASSERT_EQ(
        GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                              &view, nullptr),
        GEOARROW_OK);
    EXPECT_EQ(GeoArrowGeometryViewValidate(view, nullptr), GEOARROW_OK);
  }
  GeoArrowWKBReaderReset(&reader);
}

TEST(GeometryTest, GeometryViewValidateInvalidStructure) {
  WKXTester tester;
  const auto& geom = tester.AsGeometry("MULTIPOLYGON (((1 2, 3 4)))");
  std::vector<GeoArrowGeometryNode> nodes(geom.root, geom.root + geom.size_nodes);
  auto reject = [&](GeoArrowGeometryView view) {
    GeoArrowError error = {};
    EXPECT_EQ(GeoArrowGeometryViewValidate(view, &error), EINVAL);
    EXPECT_NE(error.message[0], '\0');
  };
  reject({nullptr, 1});
  reject({nodes.data(), -1});
  reject({nodes.data(), 0});
  reject({nodes.data(), 2});
  nodes.push_back(nodes.back());
  reject({nodes.data(), 4});
  nodes.pop_back();
  nodes[0].size = 0;
  reject({nodes.data(), 3});
  nodes[0].size = 1;
  nodes[1].level = 2;
  reject({nodes.data(), 3});
  nodes[1].level = 1;
  nodes[1].geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
  reject({nodes.data(), 3});
  nodes[1].geometry_type = GEOARROW_GEOMETRY_TYPE_POLYGON;
  nodes[2].dimensions = 0;
  reject({nodes.data(), 3});
  nodes[2].dimensions = GEOARROW_DIMENSIONS_XYZM + 1;
  reject({nodes.data(), 3});
  nodes[2].dimensions = GEOARROW_DIMENSIONS_XY;
  nodes[0].dimensions = 0;
  reject({nodes.data(), 3});
  nodes[0].dimensions = GEOARROW_DIMENSIONS_XY;
  nodes[0].geometry_type = GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION + 1;
  reject({nodes.data(), 3});

  nodes[0] = nodes[2];
  nodes[0].geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  nodes[0].size = 2;
  reject({nodes.data(), 1});
  nodes[0].size = 0;
  EXPECT_EQ(GeoArrowGeometryViewValidate({nodes.data(), 1}, nullptr), GEOARROW_OK);
  nodes[0].dimensions = GEOARROW_DIMENSIONS_XYZM + 1;
  reject({nodes.data(), 1});
}

TEST(GeometryTest, GeometryViewValidateSubtreeAndNestingLimit) {
  std::vector<GeoArrowGeometryNode> nodes(33);
  for (int i = 0; i < 33; i++) {
    nodes[i].geometry_type = GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION;
    nodes[i].dimensions = GEOARROW_DIMENSIONS_XY;
    nodes[i].level = static_cast<uint8_t>(i);
    nodes[i].size = 1;
  }
  nodes[31].geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  nodes[31].size = 0;
  EXPECT_EQ(GeoArrowGeometryViewValidate({nodes.data(), 32}, nullptr), GEOARROW_OK);

  // A complete tree with one additional level must fail before indexing the stack.
  nodes[31].geometry_type = GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION;
  nodes[31].size = 1;
  nodes[32].geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  nodes[32].size = 0;
  EXPECT_EQ(GeoArrowGeometryViewValidate({nodes.data(), 33}, nullptr), EINVAL);
  // The limit is relative to the view root, including when it is a subtree.
  EXPECT_EQ(GeoArrowGeometryViewValidate({nodes.data() + 1, 32}, nullptr), GEOARROW_OK);

  // Absolute node levels still cannot overflow their uint8_t representation.
  nodes[0].level = UINT8_MAX;
  nodes[1].geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  nodes[1].size = 0;
  nodes[1].level = 0;
  EXPECT_EQ(GeoArrowGeometryViewValidate({nodes.data(), 2}, nullptr), EINVAL);
}

TEST(GeometryTest, GeometryViewValidateDoesNotLimitNativeOffsets) {
  GeoArrowGeometryNode node = {};
  node.geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
  node.dimensions = GEOARROW_DIMENSIONS_XY;
  node.size = UINT32_MAX;
  // Only metadata is inspected: no coordinate buffers are read or allocated.
  EXPECT_EQ(GeoArrowGeometryViewValidate({&node, 1}, nullptr), GEOARROW_OK);
}

TEST(GeometryTest, WriteSequenceLargeSize) {
  GeoArrowGeometryNode node = {};
  node.geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
  node.dimensions = GEOARROW_DIMENSIONS_XYZM;
  node.size = 0x40000000U;
  // Sizing a sequence must not overflow before allocating the copy buffer.
  EXPECT_EQ(GeoArrowGeometryNodeWriteSequence(&node, nullptr, 0),
            int64_t{0x40000000} * 4 * 8);
}
