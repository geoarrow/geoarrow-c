#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.hpp"

#include "geoarrow/wkx_testing.hpp"

using geoarrow::geometry::Geometry;
using geoarrow::geometry::WKBParser;
using XY = geoarrow::array_util::XY<double>;
using XYZ = geoarrow::array_util::XYZ<double>;
using XYM = geoarrow::array_util::XYM<double>;
using XYZM = geoarrow::array_util::XYZM<double>;

TEST(GeoArrowHppTest, ValidWKBArray) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  WKXTester tester;
  tester.ReadWKT("POINT (0 1)", writer.visitor());

  struct ArrowArray array_feat;
  writer.Finish(&array_feat);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array_feat);

  geoarrow::wkb_util::WKBArray<int32_t> array;
  array.Init(reader.View().array_view());

  Geometry geometry;
  WKBParser parser;
  std::vector<XY> coords;
  std::vector<std::pair<XY, XY>> edges;

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(0), &geometry), WKBParser::OK);

  auto& root = *geometry.data();
  EXPECT_EQ(root.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  root.VisitVertices([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}));
}
