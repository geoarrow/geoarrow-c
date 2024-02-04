
#include <gtest/gtest.h>

#include <geoarrow.h>
#include "nanoarrow.h"

#include "wkx_testing.hpp"

TEST(WKBWriterTest, WKBWriterTestBasic) {
  struct GeoArrowWKBWriter writer;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestOneNull) {
  struct GeoArrowWKBWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKBWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 1);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 0));

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestOneValidOneNull) {
  struct GeoArrowWKBWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKBWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 3);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_BINARY);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 0));
  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 1));
  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 2));
  struct ArrowBufferView value = ArrowArrayViewGetBytesUnsafe(&view, 0);

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestErrors) {
  struct GeoArrowWKBWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  struct GeoArrowCoordView coords;
  coords.n_coords = 0;
  coords.n_values = 2;
  coords.coords_stride = 1;

  // Invalid because level < 0
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), EINVAL);
  EXPECT_EQ(v.coords(&v, &coords), GEOARROW_OK);

  GeoArrowWKBWriterReset(&writer);
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  // Invalid because of too much nesting
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  for (int i = 0; i < 31; i++) {
    EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
              GEOARROW_OK);
  }
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            EINVAL);

  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestPoint) {
  WKXTester tester;

  EXPECT_EQ(tester.AsWKB("POINT (30 10)"),
            std::basic_string<uint8_t>({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40}));
}

TEST(WKBWriterTest, WKBWriterTestLinestring) {
  WKXTester tester;

  EXPECT_EQ(tester.AsWKB("LINESTRING (30 10, 12 42)"),
            std::basic_string<uint8_t>(
                {0x01, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x40,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x40}));
}

TEST(WKBWriterTest, WKBWriterTestPolygon) {
  WKXTester tester;

  EXPECT_EQ(
      tester.AsWKB(
          "POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))"),
      std::basic_string<uint8_t>(
          {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e,
           0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x80, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x24, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34,
           0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x80, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x41, 0x40, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x3e, 0x40}));
}

TEST(WKBWriterTest, WKBWriterTestMultipoint) {
  WKXTester tester;

  EXPECT_EQ(tester.AsWKB("MULTIPOINT ((10 40), (40 30), (20 20), (30 10))"),
            std::basic_string<uint8_t>(
                {0x01, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x3e, 0x40, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40,
                 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e,
                 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40}));
}

TEST(WKBWriterTest, WKBWriterTestNestedCollection) {
  WKXTester tester;

  EXPECT_EQ(
      tester.AsWKB("GEOMETRYCOLLECTION (POINT (40 10), LINESTRING(10 10, 20 20, 10 40), "
                   "POLYGON((40 40, 20 45, 45 30, 40 40)), GEOMETRYCOLLECTION(POINT(40 "
                   "10), LINESTRING(10 10, 20 20, 10 40), POLYGON((40 40, 20 45, 45 30, "
                   "40 40))), GEOMETRYCOLLECTION EMPTY, POINT(30 10)) "),
      std::basic_string<uint8_t>(
          {0x01, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x24, 0x40, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
           0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01, 0x03, 0x00, 0x00,
           0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
           0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x01, 0x07, 0x00, 0x00, 0x00,
           0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x01,
           0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34,
           0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x44, 0x40, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
           0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x34, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x80, 0x46, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x44, 0x40, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
           0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40}));
}
