
#include <cerrno>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

#include "geoarrow/wkx_testing.hpp"

TEST(ArrayWriterTest, ArrayWriterTestInitFromType) {
  struct GeoArrowArrayWriter writer;

  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);
  GeoArrowArrayWriterReset(&writer);

  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKB), GEOARROW_OK);
  GeoArrowArrayWriterReset(&writer);

  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_POINT), GEOARROW_OK);
  GeoArrowArrayWriterReset(&writer);
}

TEST(ArrayWriterTest, ArrayWriterTestInitFromSchema) {
  struct GeoArrowArrayWriter writer;
  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayWriterInitFromSchema(&writer, &schema), GEOARROW_OK);
  schema.release(&schema);
  GeoArrowArrayWriterReset(&writer);
}

TEST(ArrayWriterTest, ArrayWriterTestWKT) {
  WKXTester tester;

  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);

  struct GeoArrowVisitor v;
  GeoArrowVisitorInitVoid(&v);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);
  tester.ReadWKT("POINT (30 10)", &v);

  struct ArrowArray array;
  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, NULL), GEOARROW_OK);
  ASSERT_EQ(array.length, 1);
  ASSERT_EQ(array.n_buffers, 3);
  ASSERT_EQ(array.n_children, 0);
  array.release(&array);

  ASSERT_EQ(GeoArrowArrayWriterSetPrecision(&writer, 3), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);
  tester.ReadWKT("POINT (30.3333333 10.3333333)", &v);

  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, NULL), GEOARROW_OK);
  ASSERT_EQ(array.length, 1);
  ASSERT_EQ(array.n_buffers, 3);
  ASSERT_EQ(array.n_children, 0);
  const char* answer = "POINT (30.333 10.333)";
  ASSERT_EQ(memcmp(array.buffers[2], answer, strlen(answer)), 0);
  array.release(&array);

  ASSERT_EQ(GeoArrowArrayWriterSetFlatMultipoint(&writer, false), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);
  tester.ReadWKT("MULTIPOINT ((30 10))", &v);

  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, NULL), GEOARROW_OK);
  ASSERT_EQ(array.length, 1);
  ASSERT_EQ(array.n_buffers, 3);
  ASSERT_EQ(array.n_children, 0);
  answer = "MULTIPOINT ((30 10))";
  ASSERT_EQ(memcmp(array.buffers[2], answer, strlen(answer)), 0);
  array.release(&array);

  GeoArrowArrayWriterReset(&writer);
}

TEST(ArrayWriterTest, ArrayWriterTestWKB) {
  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKB), GEOARROW_OK);

  struct GeoArrowVisitor v;
  GeoArrowVisitorInitVoid(&v);
  ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);

  // Can't set WKT options for non-WKT type
  ASSERT_EQ(GeoArrowArrayWriterSetPrecision(&writer, 3), EINVAL);
  ASSERT_EQ(GeoArrowArrayWriterSetFlatMultipoint(&writer, false), EINVAL);

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
