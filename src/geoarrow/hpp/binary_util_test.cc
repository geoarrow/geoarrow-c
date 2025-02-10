#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "array_reader.hpp"
#include "array_util.hpp"
#include "array_writer.hpp"
#include "binary_util.hpp"

#include "wkx_testing.hpp"

using geoarrow::binary_util::wkb::WKBGeometry;
using geoarrow::binary_util::wkb::WKBParser;

TEST(GeoArrowHppTest, ReadWKBBasic) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  WKXTester tester;
  tester.ReadWKT("LINESTRING (0 1, 2 3)", writer.visitor());
  tester.ReadWKT("LINESTRING (4 5, 6 7)", writer.visitor());
  tester.ReadWKT("LINESTRING (8 9, 10 11, 12 13)", writer.visitor());

  struct ArrowArray array_feat;
  writer.Finish(&array_feat);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array_feat);

  geoarrow::array_util::BinaryArray<int32_t> array;
  array.Init(reader.View().array_view());

  WKBGeometry geometry;
  WKBParser parser;
  ASSERT_EQ(parser.Parse(array.value.at(0), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type_, GEOARROW_GEOMETRY_TYPE_LINESTRING);
}
