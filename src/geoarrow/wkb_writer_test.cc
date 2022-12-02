#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

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
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

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

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKBWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_BINARY);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 0));
  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 1));
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

  // Invalid because level < 0
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), EINVAL);
  EXPECT_EQ(v.coords(&v, nullptr, 0, 2), GEOARROW_OK);

  GeoArrowWKBWriterReset(&writer);
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  // Invalid because of too much nesting
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  for (int i = 0; i < 32; i++) {
    EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
              GEOARROW_OK);
  }
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            EINVAL);

  GeoArrowWKBWriterReset(&writer);
}
