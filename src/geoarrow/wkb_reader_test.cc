
#include "geoarrow/wkx_testing.hpp"

#include <gtest/gtest.h>

#include <cmath>

#define EXPECT_WKB_ROUNDTRIP(tester_, value_) EXPECT_EQ(tester_.AsWKB(value_), value_)

TEST(WBTReaderTest, WKBReaderTestBasic) {
  struct GeoArrowWKBReader reader;
  GeoArrowWKBReaderInit(&reader);
  GeoArrowWKBReaderReset(&reader);
}

TEST(WKBReaderTest, WKBReaderTestPoint) {
  WKXTester tester;

  std::vector<uint8_t> point({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  EXPECT_WKB_ROUNDTRIP(tester, point);
  EXPECT_EQ(tester.AsWKT(point), "POINT (30 10)");

  struct GeoArrowWKBReader reader;
  GeoArrowWKBReaderInit(&reader);

  struct GeoArrowGeometryView geometry;
  ASSERT_EQ(
      GeoArrowWKBReaderRead(&reader, {point.data(), static_cast<int64_t>(point.size())},
                            &geometry, nullptr),
      GEOARROW_OK);
  EXPECT_EQ(geometry.root->geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(geometry.root->size, 1);

  GeoArrowWKBReaderReset(&reader);
}

TEST(WKBReaderTest, WKBReaderTestPointEmpty) {
  WKXTester tester;

  // POINT EMPTY is encoded in WKB using an all-NaN coordinate.
  std::vector<uint8_t> point_empty({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f});

  EXPECT_WKB_ROUNDTRIP(tester, point_empty);
  EXPECT_EQ(tester.AsWKT(point_empty), "POINT EMPTY");

  const auto& geometry = tester.AsGeometry(point_empty);
  EXPECT_EQ(tester.AsWKT(geometry), "POINT EMPTY");
}

TEST(WKBReaderTest, WKBReaderTestPointZM) {
  WKXTester tester;

  std::vector<uint8_t> point_z({0x01, 0xe9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x08, 0x40});

  std::vector<uint8_t> point_m({0x01, 0xd1, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x08, 0x40});

  std::vector<uint8_t> point_zm(
      {0x01, 0xb9, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x40});

  EXPECT_EQ(tester.AsWKT(point_z), "POINT Z (1 2 3)");
  EXPECT_EQ(tester.AsWKT(point_m), "POINT M (1 2 3)");
  EXPECT_EQ(tester.AsWKT(point_zm), "POINT ZM (1 2 3 4)");
}

TEST(WKBReaderTest, WKBReaderTestPointInvalid) {
  WKXTester tester;

  std::vector<uint8_t> short_point1({});

  std::vector<uint8_t> short_point2({0x01, 0x01, 0x00, 0x00});

  std::vector<uint8_t> short_point3({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x24});

  std::vector<uint8_t> bad_point({0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  EXPECT_THROW(tester.AsWKB(short_point1), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(),
            "Expected endian byte but found end of buffer at byte 0");
  EXPECT_THROW(tester.AsWKB(short_point2), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(),
            "Expected uint32 but found end of buffer at byte 1");
  EXPECT_THROW(tester.AsWKB(short_point3), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(),
            "Expected coordinate sequence of 1 coords (16 bytes) but found 15 bytes "
            "remaining at byte 5");

  EXPECT_THROW(tester.AsWKB(bad_point), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(),
            "Expected valid geometry type code but found 257 at byte 1");
}

TEST(WKBReaderTest, WKBReaderTestPointBigEndian) {
  WKXTester tester;

  std::vector<uint8_t> point({0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x3e,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
                              0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

  EXPECT_EQ(tester.AsWKT(point), "POINT (30 10)");
}

TEST(WKBReaderTest, WKBReaderTestPointEWKB) {
  WKXTester tester;

  std::vector<uint8_t> point_z({0x01, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x40});

  std::vector<uint8_t> point_m({0x01, 0x01, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x40});

  std::vector<uint8_t> point_zm(
      {0x01, 0x01, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f});

  // These two have embedded SRID values (which the this parser understands but
  // skips since there's no place to put that information)
  std::vector<uint8_t> point_s({0x01, 0x01, 0x00, 0x00, 0x20, 0xc7, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  std::vector<uint8_t> point_zms(
      {0x01, 0x01, 0x00, 0x00, 0xe0, 0xe6, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x28, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x40});

  EXPECT_EQ(tester.AsWKT(point_z), "POINT Z (30 10 2)");
  EXPECT_EQ(tester.AsWKT(point_m), "POINT M (30 10 2)");
  EXPECT_EQ(tester.AsWKT(point_zm), "POINT ZM (30 10 2 1)");
  EXPECT_EQ(tester.AsWKT(point_s), "POINT (30 10)");
  EXPECT_EQ(tester.AsWKT(point_zms), "POINT ZM (30 10 12 14)");
}

TEST(WKBReaderTest, WKBReaderTestLinestring) {
  WKXTester tester;

  EXPECT_WKB_ROUNDTRIP(
      tester, std::vector<uint8_t>({0x01, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x28, 0x40, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x45, 0x40}));
}

TEST(WKBReaderTest, WKBReaderTestPolygon) {
  WKXTester tester;

  std::vector<uint8_t> polygon(
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
       0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40});

  EXPECT_EQ(
      tester.AsWKT(polygon),
      "POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))");

  EXPECT_WKB_ROUNDTRIP(tester, polygon);
}

TEST(WKBReaderTest, WKBReaderTestCollection) {
  WKXTester tester;

  EXPECT_WKB_ROUNDTRIP(
      tester, std::vector<uint8_t>(
                  {0x01, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x3e, 0x40, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40,
                   0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e,
                   0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40}));
}

TEST(WKBReaderTest, WKBReaderTestNestedCollection) {
  WKXTester tester;

  std::vector<uint8_t> nested_collection(
      {0x01, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x24, 0x40, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34,
       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x44, 0x40, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x46, 0x40,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01, 0x07, 0x00, 0x00,
       0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x01, 0x02,
       0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01,
       0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
       0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  EXPECT_EQ(tester.AsWKT(nested_collection),
            "GEOMETRYCOLLECTION (POINT (40 10), LINESTRING (10 10, 20 20, 10 40), "
            "POLYGON ((40 40, 20 45, 45 30, 40 40)), GEOMETRYCOLLECTION (POINT (40 10), "
            "LINESTRING (10 10, 20 20, 10 40), POLYGON ((40 40, 20 45, 45 30, 40 40))), "
            "GEOMETRYCOLLECTION EMPTY, POINT (30 10))");

  EXPECT_WKB_ROUNDTRIP(tester, nested_collection);
}

TEST(WKBReaderTest, WKTReaderTestManyCoordinates) {
  // The reader uses an internal coordinate buffer of 3072 ordinates; however,
  // none of the above tests have enough coordinates to run into a situation
  // where it must be flushed.

  // Make a big linestring
  std::stringstream ss;
  ss << "LINESTRING (0 1";
  for (int i = 1; i < 1537; i++) {
    ss << ", " << i << " " << (i + 1);
  }
  ss << ")";

  WKXTester tester;
  std::vector<uint8_t> big_linestring_wkb = tester.AsWKB(ss.str());
  EXPECT_WKB_ROUNDTRIP(tester, big_linestring_wkb);
}

TEST(WKBReaderTest, WKBReaderTestTruncatedMultipoint) {
  WKXTester tester;
  auto wkb = tester.AsWKB("MULTIPOINT ((1 2), (3 4))");
  wkb.resize(wkb.size() - sizeof(double));

  struct GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  struct GeoArrowGeometryView geometry;
  struct GeoArrowError error = {};
  EXPECT_EQ(GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                                  &geometry, &error),
            EINVAL);
  EXPECT_STREQ(error.message,
               "Expected coordinate sequence of 1 coords (16 bytes) but found 8 bytes "
               "remaining at byte 35");
  GeoArrowWKBReaderReset(&reader);
}

TEST(WKBReaderTest, WKBReaderTestTruncatedBuffers) {
  WKXTester tester;
  const std::vector<std::string> geometries = {
      "POINT (1 2)",
      "POINT Z (1 2 3)",
      "POINT M (1 2 3)",
      "POINT ZM (1 2 3 4)",
      "LINESTRING (1 2, 3 4)",
      "POLYGON ((0 0, 4 0, 0 4, 0 0), (1 1, 2 1, 1 2, 1 1))",
      "MULTIPOINT ((1 2), (3 4))",
      "MULTILINESTRING ((1 2, 3 4), (5 6, 7 8))",
      "MULTIPOLYGON (((0 0, 4 0, 0 4, 0 0)), ((1 1, 2 1, 1 2, 1 1)))",
      "GEOMETRYCOLLECTION (POINT (1 2), GEOMETRYCOLLECTION (POINT (3 4)))",
      "GEOMETRYCOLLECTION (LINESTRING EMPTY, POINT (1 2), POLYGON EMPTY)",
      "GEOMETRYCOLLECTION EMPTY"};

  struct GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  struct GeoArrowGeometryView geometry;
  for (const auto& wkt : geometries) {
    SCOPED_TRACE(wkt);
    auto wkb = tester.AsWKB(wkt);
    for (size_t size = 0; size < wkb.size(); size++) {
      SCOPED_TRACE(size);
      // Give sanitizers an allocation ending exactly at the truncated buffer's end.
      // Keep a non-null pointer for the empty prefix.
      std::vector<uint8_t> truncated(size == 0 ? 1 : size);
      memcpy(truncated.data(), wkb.data(), size);
      EXPECT_EQ(
          GeoArrowWKBReaderRead(&reader, {truncated.data(), static_cast<int64_t>(size)},
                                &geometry, nullptr),
          EINVAL);
    }

    // A failed read must not prevent reuse of the reader for a complete geometry.
    EXPECT_EQ(
        GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                              &geometry, nullptr),
        GEOARROW_OK);
  }
  GeoArrowWKBReaderReset(&reader);
}

TEST(WKBReaderTest, WKBReaderTestLargeCoordinateCounts) {
  struct GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  struct GeoArrowGeometryView geometry;
  for (uint8_t endian : {uint8_t(0), uint8_t(1)}) {
    SCOPED_TRACE(endian);
    for (uint32_t type : {2, 1002, 2002, 3002}) {
      SCOPED_TRACE(type);
      for (uint32_t count : {0x80000000U, 0xaaaaaaabU, 0x40000000U, UINT32_MAX}) {
        SCOPED_TRACE(count);
        // These counts can wrap the product of count and dimension to zero or a
        // small value if multiplication is performed in 32 bits.
        std::vector<uint8_t> wkb(9 + 4 * sizeof(double), 0);
        wkb[0] = endian;
        for (int i = 0; i < 4; i++) {
          int shift = (endian == 1 ? i : 3 - i) * 8;
          wkb[1 + i] = static_cast<uint8_t>(type >> shift);
          wkb[5 + i] = static_cast<uint8_t>(count >> shift);
        }
        EXPECT_EQ(
            GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                                  &geometry, nullptr),
            EINVAL);
      }
    }
  }
  GeoArrowWKBReaderReset(&reader);
}

namespace {

void AppendWKBUInt32(std::vector<uint8_t>* out, uint32_t value, uint8_t endian) {
  for (int i = 0; i < 4; i++) {
    out->push_back(static_cast<uint8_t>(value >> ((endian == 1 ? i : 3 - i) * 8)));
  }
}

void AppendWKBHeader(std::vector<uint8_t>* out, uint32_t type, uint8_t endian) {
  out->push_back(endian);
  AppendWKBUInt32(out, type, endian);
}

void AppendWKBCoords(std::vector<uint8_t>* out, const std::vector<double>& coords,
                     uint8_t endian) {
  for (double coord : coords) {
    uint64_t bits;
    memcpy(&bits, &coord, sizeof(bits));
    for (int i = 0; i < 8; i++) {
      out->push_back(static_cast<uint8_t>(bits >> ((endian == 1 ? i : 7 - i) * 8)));
    }
  }
}

double ReadNodeCoord(const GeoArrowGeometryNode& node, uint32_t coord, int dimension) {
  double value;
  memcpy(&value, node.coords[dimension] + coord * node.coord_stride[dimension],
         sizeof(value));
  return value;
}

}  // namespace

TEST(WKBReaderTest, NativeEndianCoordinatesAndReaderReuse) {
  GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  GeoArrowGeometryView geom;
  const GeoArrowGeometryNode* previous_root = nullptr;
  // Alternate byte order and grow/shrink the copied coordinate buffer on one reader.
  for (uint32_t count : {1, 200, 2}) {
    for (uint32_t type : {2, 1002, 2002, 3002}) {
      int dims = type == 2 ? 2 : (type == 3002 ? 4 : 3);
      for (uint8_t endian : {uint8_t(0), uint8_t(1), uint8_t(0), uint8_t(1)}) {
        SCOPED_TRACE(type);
        SCOPED_TRACE(endian);
        std::vector<uint8_t> wkb;
        AppendWKBHeader(&wkb, type, endian);
        AppendWKBUInt32(&wkb, count, endian);
        for (uint32_t i = 0; i < count; i++) {
          std::vector<double> coord;
          for (int j = 0; j < dims; j++) coord.push_back(i * 10.0 + j + 0.25);
          AppendWKBCoords(&wkb, coord, endian);
        }
        const auto original = wkb;
        ASSERT_EQ(
            GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                                  &geom, nullptr),
            GEOARROW_OK);
        ASSERT_EQ(geom.size_nodes, 1);
        // Both zero-copy and converted reads reuse the same nodes.
        if (previous_root != nullptr) {
          EXPECT_EQ(geom.root, previous_root);
        }
        previous_root = geom.root;
        EXPECT_EQ(geom.root->flags, 0);
        EXPECT_EQ(geom.root->size, count);
        if (endian == GEOARROW_NATIVE_ENDIAN) {
          EXPECT_EQ(geom.root->coords[0], wkb.data() + 9);
        } else {
          EXPECT_NE(geom.root->coords[0], wkb.data() + 9);
        }
        for (uint32_t i = 0; i < count; i++) {
          for (int j = 0; j < dims; j++) {
            EXPECT_DOUBLE_EQ(ReadNodeCoord(*geom.root, i, j), i * 10.0 + j + 0.25);
          }
        }
        for (int j = dims; j < 4; j++) {
          EXPECT_TRUE(std::isnan(ReadNodeCoord(*geom.root, 0, j)));
        }
        EXPECT_EQ(wkb, original);

        // Failure after reading a swapped sequence must not poison the next read.
        EXPECT_EQ(GeoArrowWKBReaderRead(
                      &reader, {wkb.data(), static_cast<int64_t>(wkb.size() - 1)}, &geom,
                      nullptr),
                  EINVAL);
      }
    }
  }
  GeoArrowWKBReaderReset(&reader);
}

TEST(WKBReaderTest, MixedEndianNestedCollection) {
  GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  GeoArrowGeometry saved;
  ASSERT_EQ(GeoArrowGeometryInit(&saved), GEOARROW_OK);
  for (uint8_t endian : {uint8_t(0), uint8_t(1)}) {
    uint8_t other = !endian;
    std::vector<uint8_t> wkb;
    AppendWKBHeader(&wkb, 7, endian);
    AppendWKBUInt32(&wkb, 2, endian);
    AppendWKBHeader(&wkb, 1, other);
    AppendWKBCoords(&wkb, {1, 2}, other);
    AppendWKBHeader(&wkb, 7, endian);
    AppendWKBUInt32(&wkb, 3, endian);
    AppendWKBHeader(&wkb, 3, other);
    AppendWKBUInt32(&wkb, 2, other);
    AppendWKBUInt32(&wkb, 4, other);
    AppendWKBCoords(&wkb, {0, 0, 4, 0, 0, 4, 0, 0}, other);
    AppendWKBUInt32(&wkb, 0, other);  // Empty ring after a nonempty sequence.
    AppendWKBHeader(&wkb, 1, endian);
    AppendWKBCoords(&wkb, {3, 4}, endian);
    AppendWKBHeader(&wkb, 2, other);
    AppendWKBUInt32(&wkb, 0, other);  // Empty sequence at the end of copied storage.
    const auto original = wkb;
    GeoArrowGeometryView geom;
    ASSERT_EQ(
        GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                              &geom, nullptr),
        GEOARROW_OK);
    ASSERT_EQ(geom.size_nodes, 8);
    for (int64_t i = 0; i < geom.size_nodes; i++) EXPECT_EQ(geom.root[i].flags, 0);
    EXPECT_DOUBLE_EQ(ReadNodeCoord(geom.root[1], 0, 0), 1);
    EXPECT_DOUBLE_EQ(ReadNodeCoord(geom.root[4], 1, 0), 4);
    EXPECT_DOUBLE_EQ(ReadNodeCoord(geom.root[6], 0, 1), 4);
    for (int index : {5, 7}) {
      for (int j = 0; j < 4; j++) {
        EXPECT_TRUE(std::isnan(ReadNodeCoord(geom.root[index], 0, j)));
      }
    }
    EXPECT_EQ(wkb, original);
    ASSERT_EQ(GeoArrowGeometryDeepCopy(geom, &saved), GEOARROW_OK);
    WKXTester tester;
    EXPECT_EQ(tester.AsWKT(saved),
              "GEOMETRYCOLLECTION (POINT (1 2), GEOMETRYCOLLECTION (POLYGON ((0 0, 4 0, "
              "0 4, 0 0), EMPTY), POINT (3 4), LINESTRING EMPTY))");
    EXPECT_EQ(tester.AsWKB(saved), tester.AsWKB(wkb));
    EXPECT_EQ(tester.AsWKTUsingVisitor(saved), tester.AsWKT(saved));
    // Truncate after a successfully parsed point, including a swapped sequence.
    EXPECT_EQ(GeoArrowWKBReaderRead(&reader, {wkb.data(), 31}, &geom, nullptr), EINVAL);
  }
  GeoArrowWKBReaderReset(&reader);
  EXPECT_DOUBLE_EQ(ReadNodeCoord(saved.root[1], 0, 0), 1);
  GeoArrowGeometryReset(&saved);
}

TEST(WKBReaderTest, OppositeEndianEmptyGeometries) {
  GeoArrowWKBReader reader;
  ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
  uint8_t endian = !GEOARROW_NATIVE_ENDIAN;
  for (uint32_t type : {1, 2, 3, 4, 5, 6, 7}) {
    SCOPED_TRACE(type);
    std::vector<uint8_t> wkb;
    AppendWKBHeader(&wkb, type, endian);
    if (type == 1) {
      AppendWKBCoords(&wkb, {NAN, NAN}, endian);
    } else {
      AppendWKBUInt32(&wkb, 0, endian);
    }
    GeoArrowGeometryView geom;
    ASSERT_EQ(
        GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                              &geom, nullptr),
        GEOARROW_OK);
    EXPECT_EQ(geom.root->flags, 0);
    for (int j = 0; j < 4; j++) {
      EXPECT_TRUE(std::isnan(ReadNodeCoord(*geom.root, 0, j)));
    }
  }
  GeoArrowWKBReaderReset(&reader);
}
