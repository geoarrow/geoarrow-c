

#include <gtest/gtest.h>

#include "geoarrow.h"

#include "wkx_testing.hpp"

TEST(ArrayWriterTest, ArrayWriterTestBasic) {
  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);
  GeoArrowArrayWriterReset(&writer);
}

TEST(ArrayWriterTest, ArrayWriterTestWKT) {
  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);

  struct GeoArrowVisitor v;
  GeoArrowVisitorInitVoid(&v);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);

  struct ArrowArray array;
  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, NULL), GEOARROW_OK);
  ASSERT_EQ(array.length, 0);
  ASSERT_EQ(array.n_buffers, 3);
  ASSERT_EQ(array.n_children, 0);

  array.release(&array);
  GeoArrowArrayWriterReset(&writer);
}

TEST(ArrayWriterTest, ArrayWriterTestWKB) {
  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKB), GEOARROW_OK);

  struct GeoArrowVisitor v;
  GeoArrowVisitorInitVoid(&v);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);

  struct ArrowArray array;
  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, NULL), GEOARROW_OK);
  ASSERT_EQ(array.length, 0);
  ASSERT_EQ(array.n_buffers, 3);
  ASSERT_EQ(array.n_children, 0);

  array.release(&array);
  GeoArrowArrayWriterReset(&writer);
}

TEST(ArrayWriterTest, ArrayWriterTestGeoArrow) {
  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_POINT), GEOARROW_OK);

  struct GeoArrowVisitor v;
  GeoArrowVisitorInitVoid(&v);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);

  struct ArrowArray array;
  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, NULL), GEOARROW_OK);
  ASSERT_EQ(array.length, 0);
  ASSERT_EQ(array.n_buffers, 1);
  ASSERT_EQ(array.n_children, 2);

  array.release(&array);
  GeoArrowArrayWriterReset(&writer);
}
