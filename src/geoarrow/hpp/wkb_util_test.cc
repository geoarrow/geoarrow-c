#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.hpp"

#include "geoarrow/wkx_testing.hpp"

using geoarrow::wkb_util::WKBGeometry;
using geoarrow::wkb_util::WKBParser;
using XY = geoarrow::array_util::XY<double>;
using XYZ = geoarrow::array_util::XYZ<double>;
using XYM = geoarrow::array_util::XYM<double>;
using XYZM = geoarrow::array_util::XYZM<double>;

TEST(GeoArrowHppTest, ValidWKBArray) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  WKXTester tester;
  tester.ReadWKT("POINT (0 1)", writer.visitor());
  tester.ReadWKT("LINESTRING (0 1, 2 3)", writer.visitor());
  tester.ReadWKT("POLYGON ((4 5, 6 7), (8 9, 10 11))", writer.visitor());
  tester.ReadWKT("MULTIPOINT ((8 9), (10 11))", writer.visitor());
  tester.ReadWKT("MULTILINESTRING ((12 13, 14 15), (16 17, 18 19))", writer.visitor());

  struct ArrowArray array_feat;
  writer.Finish(&array_feat);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array_feat);

  geoarrow::wkb_util::WKBArray<int32_t> array;
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
  EXPECT_EQ(geometry.Sequence(0).size(), 1);
  geometry.VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}));
  geometry.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {0, 1})));

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(1), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_LINESTRING);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 1);
  EXPECT_EQ(geometry.Sequence(0).size(), 2);
  geometry.VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}));
  geometry.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {2, 3})));

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(2), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POLYGON);
  EXPECT_EQ(geometry.NumGeometries(), 0);
  EXPECT_EQ(geometry.NumSequences(), 2);
  EXPECT_EQ(geometry.Sequence(0).size(), 2);
  EXPECT_EQ(geometry.Sequence(1).size(), 2);
  geometry.VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{4, 5}, XY{6, 7}, XY{8, 9}, XY{10, 11}));
  geometry.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({4, 5}, {6, 7}),
                                            std::pair<XY, XY>({8, 9}, {10, 11})));

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(3), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(geometry.NumGeometries(), 2);
  EXPECT_EQ(geometry.NumSequences(), 0);
  EXPECT_EQ(geometry.Geometry(0).NumSequences(), 1);
  EXPECT_EQ(geometry.Geometry(0).geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.Geometry(1).NumSequences(), 1);
  EXPECT_EQ(geometry.Geometry(1).geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  geometry.VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords, ::testing::ElementsAre(XY{8, 9}, XY{10, 11}));
  geometry.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({8, 9}, {8, 9}),
                                            std::pair<XY, XY>({10, 11}, {10, 11})));

  coords.clear();
  edges.clear();
  ASSERT_EQ(parser.Parse(array.value.blob(4), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING);
  EXPECT_EQ(geometry.NumGeometries(), 2);
  EXPECT_EQ(geometry.NumSequences(), 0);
  EXPECT_EQ(geometry.Geometry(0).NumSequences(), 1);
  EXPECT_EQ(geometry.Geometry(0).geometry_type, GEOARROW_GEOMETRY_TYPE_LINESTRING);
  EXPECT_EQ(geometry.Geometry(1).NumSequences(), 1);
  EXPECT_EQ(geometry.Geometry(1).geometry_type, GEOARROW_GEOMETRY_TYPE_LINESTRING);
  geometry.VisitVertices<XY>([&](XY val) { coords.push_back(val); });
  EXPECT_THAT(coords,
              ::testing::ElementsAre(XY{12, 13}, XY{14, 15}, XY{16, 17}, XY{18, 19}));
  geometry.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({12, 13}, {14, 15}),
                                            std::pair<XY, XY>({16, 17}, {18, 19})));

  // Check the whole-array visitors
  coords.clear();
  array.VisitVertices<XY>([&](XY v) { coords.push_back(v); });
  EXPECT_THAT(coords,
              ::testing::ElementsAre(XY{0, 1}, XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7},
                                     XY{8, 9}, XY{10, 11}, XY{8, 9}, XY{10, 11},
                                     XY{12, 13}, XY{14, 15}, XY{16, 17}, XY{18, 19}));

  edges.clear();
  array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(
      edges,
      ::testing::ElementsAre(
          std::pair<XY, XY>({0, 1}, {0, 1}), std::pair<XY, XY>({0, 1}, {2, 3}),
          std::pair<XY, XY>({4, 5}, {6, 7}), std::pair<XY, XY>({8, 9}, {10, 11}),
          std::pair<XY, XY>({8, 9}, {8, 9}), std::pair<XY, XY>({10, 11}, {10, 11}),
          std::pair<XY, XY>({12, 13}, {14, 15}), std::pair<XY, XY>({16, 17}, {18, 19})));
}

TEST(GeoArrowHppTest, ParseSwappedEndian) {
  WKBParser parser;
  WKBGeometry geometry;

  std::basic_string<uint8_t> linestring_be(
      {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x40, 0x3e, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x40, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x34, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

  ASSERT_EQ(parser.Parse(linestring_be.data(), linestring_be.size(), &geometry),
            WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_LINESTRING);
  EXPECT_EQ(geometry.srid, WKBGeometry::kSridUnset);
  EXPECT_EQ(geometry.NumSequences(), 1);
  EXPECT_EQ(geometry.Sequence(0).endianness, 0x00);

  std::vector<XY> vertices;
  geometry.VisitVertices<XY>([&](XY v) { vertices.push_back(v); });
  EXPECT_THAT(vertices, ::testing::ElementsAre(XY{30, 10}, XY{20, 40}));

  std::vector<std::pair<XY, XY>> edges;
  geometry.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({30, 10}, {20, 40})));
}

TEST(GeoArrowHppTest, ParseEWKB) {
  std::basic_string<uint8_t> point_z({0x01, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x40});

  std::basic_string<uint8_t> point_m({0x01, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x40});

  std::basic_string<uint8_t> point_zm(
      {0x01, 0x01, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f});

  // These two have embedded SRID values
  std::basic_string<uint8_t> point_s(
      {0x01, 0x01, 0x00, 0x00, 0x20, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  std::basic_string<uint8_t> point_zms(
      {0x01, 0x01, 0x00, 0x00, 0xe0, 0xe6, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x28, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x40});

  WKBParser parser;
  WKBGeometry geometry;

  uint32_t n_points = 0;
  ASSERT_EQ(parser.Parse(point_z.data(), point_z.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYZ);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.srid, WKBGeometry::kSridUnset);
  geometry.VisitVertices<XYZ>([&](XYZ v) {
    EXPECT_EQ(v, (XYZ{30, 10, 2}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);

  n_points = 0;
  ASSERT_EQ(parser.Parse(point_m.data(), point_m.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.srid, WKBGeometry::kSridUnset);
  geometry.VisitVertices<XYM>([&](XYM v) {
    EXPECT_EQ(v, (XYM{30, 10, 2}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);

  n_points = 0;
  ASSERT_EQ(parser.Parse(point_zm.data(), point_zm.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYZM);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.srid, WKBGeometry::kSridUnset);
  geometry.VisitVertices<XYZM>([&](XYZM v) {
    EXPECT_EQ(v, (XYZM{30, 10, 2, 1}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);
}

TEST(GeoArrowHppTest, ParseEWKBSrid) {
  std::basic_string<uint8_t> point_s(
      {0x01, 0x01, 0x00, 0x00, 0x20, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  std::basic_string<uint8_t> point_zms(
      {0x01, 0x01, 0x00, 0x00, 0xe0, 0xe6, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x28, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x40});

  WKBParser parser;
  WKBGeometry geometry;

  uint32_t n_points = 0;
  ASSERT_EQ(parser.Parse(point_s.data(), point_s.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.srid, 199);
  geometry.VisitVertices<XY>([&](XY v) {
    EXPECT_EQ(v, (XY{30, 10}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);

  n_points = 0;
  ASSERT_EQ(parser.Parse(point_zms.data(), point_zms.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYZM);
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.srid, 4326);
  geometry.VisitVertices<XYZM>([&](XYZM v) {
    EXPECT_EQ(v, (XYZM{30, 10, 12, 14}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);
}

TEST(GeoArrowHppTest, ParseISOZM) {
  std::basic_string<uint8_t> point_z({0x01, 0xe9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x08, 0x40});

  std::basic_string<uint8_t> point_m({0x01, 0xd1, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x08, 0x40});

  std::basic_string<uint8_t> point_zm(
      {0x01, 0xb9, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x40});

  WKBParser parser;
  WKBGeometry geometry;

  uint32_t n_points = 0;
  ASSERT_EQ(parser.Parse(point_z.data(), point_z.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYZ);
  ASSERT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  geometry.VisitVertices<XYZ>([&](XYZ v) {
    EXPECT_EQ(v, (XYZ{1, 2, 3}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);

  n_points = 0;
  ASSERT_EQ(parser.Parse(point_m.data(), point_m.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYM);
  ASSERT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  geometry.VisitVertices<XYM>([&](XYM v) {
    EXPECT_EQ(v, (XYM{1, 2, 3}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);

  n_points = 0;
  ASSERT_EQ(parser.Parse(point_zm.data(), point_zm.size(), &geometry), WKBParser::OK);
  EXPECT_EQ(geometry.dimensions, GEOARROW_DIMENSIONS_XYZM);
  ASSERT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  geometry.VisitVertices<XYZM>([&](XYZM v) {
    EXPECT_EQ(v, (XYZM{1, 2, 3, 4}));
    ++n_points;
  });
  EXPECT_EQ(n_points, 1);
}

TEST(GeoArrowHppTest, InvalidWKBArray) {
  std::basic_string<uint8_t> valid_polygon_with_extra_byte(
      {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x80, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x80, 0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x41,
       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e,
       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x80, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0xff});

  WKBParser parser;
  WKBGeometry geometry;

  // Check all versions of the polygon that aren't the full polygon
  for (uint32_t i = 0; i < (valid_polygon_with_extra_byte.size() - 1); i++) {
    SCOPED_TRACE("valid_polygon[:" + std::to_string(i) + "]");
    EXPECT_EQ(parser.Parse(valid_polygon_with_extra_byte.data(), i, &geometry),
              WKBParser::TOO_FEW_BYTES);
  }

  // Check with the extra byte to see if it is detected
  const uint8_t* end = nullptr;
  EXPECT_EQ(parser.Parse(valid_polygon_with_extra_byte.data(),
                         valid_polygon_with_extra_byte.size(), &geometry, &end),
            WKBParser::TOO_MANY_BYTES);
  // Even with the error, the parsing should have occurred
  EXPECT_EQ(geometry.geometry_type, GEOARROW_GEOMETRY_TYPE_POLYGON);
  EXPECT_EQ(end - valid_polygon_with_extra_byte.data(),
            valid_polygon_with_extra_byte.size() - 1);

  std::basic_string<uint8_t> bad_geometry_type({0x01, 0x01, 0x01, 0x01, 0x01});
  std::basic_string<uint8_t> bad_endian({0xff, 0x01, 0x01, 0x01, 0x01});

  EXPECT_EQ(parser.Parse(bad_geometry_type.data(), bad_geometry_type.size(), &geometry),
            WKBParser::INVALID_GEOMETRY_TYPE);
  EXPECT_EQ(parser.Parse(bad_endian.data(), bad_endian.size(), &geometry),
            WKBParser::INVALID_ENDIAN);

  EXPECT_EQ(parser.ErrorToString(WKBParser::OK), "OK");
  EXPECT_EQ(parser.ErrorToString(WKBParser::TOO_FEW_BYTES), "TOO_FEW_BYTES");
  EXPECT_EQ(parser.ErrorToString(WKBParser::TOO_MANY_BYTES), "TOO_MANY_BYTES");
  EXPECT_EQ(parser.ErrorToString(WKBParser::INVALID_GEOMETRY_TYPE),
            "INVALID_GEOMETRY_TYPE");
  EXPECT_EQ(parser.ErrorToString(WKBParser::INVALID_ENDIAN), "INVALID_ENDIAN");
}
