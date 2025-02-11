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

TEST(GeoArrowHppTest, WKBGeometryTypes) {
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

  geoarrow::array_util::BinaryArray<int32_t> array;
  array.Init(reader.View().array_view());

  WKBGeometry geometry;
  WKBParser parser;
  std::vector<XY> coords;

  coords.clear();
  ASSERT_EQ(parser.Parse(array.value.at(0), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 1);
  EXPECT_EQ(geometry.Sequence(0).size, 1);
  geometry.Sequence(0).Visit<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}));

  coords.clear();
  ASSERT_EQ(parser.Parse(array.value.at(1), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_LINESTRING);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 1);
  EXPECT_EQ(geometry.Sequence(0).size, 2);
  geometry.Sequence(0).Visit<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}));

  coords.clear();
  ASSERT_EQ(parser.Parse(array.value.at(2), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POLYGON);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 2);
  EXPECT_EQ(geometry.Sequence(0).size, 2);
  EXPECT_EQ(geometry.Sequence(1).size, 2);
  geometry.Sequence(0).Visit<XY>([&](XY val) { coords.push_back(val); });
  geometry.Sequence(1).Visit<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{4, 5}, XY{6, 7}, XY{8, 9}, XY{10, 11}));

  coords.clear();
  ASSERT_EQ(parser.Parse(array.value.at(3), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(geometry.NumGeometries(), 2);
  EXPECT_EQ(geometry.NumSequences(), 0);
  EXPECT_EQ(geometry.Geometry(0).NumSequences(), 1);
  EXPECT_EQ(geometry.Geometry(1).NumSequences(), 1);
  geometry.Geometry(0).Sequence(0).Visit<XY>([&](XY val) { coords.push_back(val); });
  geometry.Geometry(1).Sequence(0).Visit<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{8, 9}, XY{10, 11}));
}
