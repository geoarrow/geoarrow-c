#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "array_reader.hpp"
#include "array_util.hpp"
#include "array_writer.hpp"
#include "binary_util.hpp"

#include "wkx_testing.hpp"

using geoarrow::binary_util::WKBGeometry;
using geoarrow::binary_util::WKBParser;
using XY = geoarrow::array_util::XY<double>;

TEST(GeoArrowHppTest, ValidWKBArray) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  WKXTester tester;
  tester.ReadWKT("POINT (0 1)", writer.visitor());
  tester.ReadWKT("LINESTRING (0 1, 2 3)", writer.visitor());
  tester.ReadWKT("POLYGON ((4 5, 6 7), (8 9, 10 11))", writer.visitor());
  tester.ReadWKT("MULTIPOINT ((8 9), (10 11))", writer.visitor());

  struct ArrowArray array_feat;
  writer.Finish(&array_feat);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array_feat);

  geoarrow::binary_util::WKBArray<int32_t> array;
  array.Init(reader.View().array_view());

  WKBGeometry geometry;
  WKBParser parser;
  std::vector<XY> coords;
  std::vector<std::pair<XY, XY>> edges;

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(0), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 1);
  EXPECT_EQ(geometry.Sequence(0).size, 1);
  geometry.Sequence(0).VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}));
  geometry.Sequence(0).VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_TRUE(edges.empty());

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(1), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_LINESTRING);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 1);
  EXPECT_EQ(geometry.Sequence(0).size, 2);
  geometry.Sequence(0).VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}));
  geometry.Sequence(0).VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {2, 3})));

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(2), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POLYGON);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 2);
  EXPECT_EQ(geometry.Sequence(0).size, 2);
  EXPECT_EQ(geometry.Sequence(1).size, 2);
  geometry.Sequence(0).VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  geometry.Sequence(1).VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{4, 5}, XY{6, 7}, XY{8, 9}, XY{10, 11}));
  geometry.Sequence(0).VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  geometry.Sequence(1).VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({4, 5}, {6, 7}),
                                            std::pair<XY, XY>({8, 9}, {10, 11})));

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(3), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(geometry.NumGeometries(), 2);
  EXPECT_EQ(geometry.NumSequences(), 0);
  EXPECT_EQ(geometry.Geometry(0).NumSequences(), 1);
  EXPECT_EQ(geometry.Geometry(1).NumSequences(), 1);
  geometry.Geometry(0).Sequence(0).VisitVertices<XY>(
      [&](XY val) { coords.push_back(val); });
  geometry.Geometry(1).Sequence(0).VisitVertices<XY>(
      [&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{8, 9}, XY{10, 11}));
  geometry.Geometry(0).Sequence(0).VisitEdges<XY>([&](XY v0, XY v1) {
    edges.push_back({v0, v1});
  });
  geometry.Geometry(1).Sequence(0).VisitEdges<XY>([&](XY v0, XY v1) {
    edges.push_back({v0, v1});
  });

  EXPECT_TRUE(edges.empty());

  // Check the visitors
  coords.clear();
  array.VisitVertices<XY>([&](XY v) { coords.push_back(v); });
  EXPECT_THAT(coords,
              ::testing::ElementsAre(XY{0, 1}, XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7},
                                     XY{8, 9}, XY{10, 11}, XY{8, 9}, XY{10, 11}));

  edges.clear();
  array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {0, 1}),
                                            std::pair<XY, XY>({0, 1}, {2, 3}),
                                            std::pair<XY, XY>({4, 5}, {6, 7}),
                                            std::pair<XY, XY>({8, 9}, {10, 11}),
                                            std::pair<XY, XY>({8, 9}, {8, 9}),
                                            std::pair<XY, XY>({10, 11}, {10, 11})));
}
