
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

TEST(ArrayWriterTest, AppendGeometryAndNull) {
  WKXTester tester;
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_WKB, GEOARROW_TYPE_POINT,
                    GEOARROW_TYPE_INTERLEAVED_POINT}) {
    SCOPED_TRACE(type);
    GeoArrowArrayWriter writer;
    ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, type), GEOARROW_OK);
    // Finish and reuse, including mixing visitor and direct append operations.
    for (int batch = 0; batch < 2; batch++) {
      auto geom = GeoArrowGeometryAsView(&tester.AsGeometry("POINT (30 10)"));
      ASSERT_EQ(GeoArrowArrayWriterAppend(&writer, geom, nullptr), GEOARROW_OK);
      ASSERT_EQ(GeoArrowArrayWriterAppendNull(&writer), GEOARROW_OK);
      GeoArrowVisitor v{};
      ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);
      tester.ReadWKT("POINT (40 20)", &v);
      GeoArrowGeometryNode null_node{};
      ASSERT_EQ(GeoArrowArrayWriterAppend(&writer, {&null_node, 1}, nullptr),
                GEOARROW_OK);
      geom = GeoArrowGeometryAsView(&tester.AsGeometry("POINT EMPTY"));
      ASSERT_EQ(GeoArrowArrayWriterAppend(&writer, geom, nullptr), GEOARROW_OK);
      ArrowArray array;
      ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
      EXPECT_EQ(array.length, 5);
      EXPECT_EQ(array.null_count, 2);
      GeoArrowArrayReader reader;
      ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
      ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
      ASSERT_EQ(GeoArrowArrayReaderVisit(&reader, 0, 5, tester.WKTVisitor()),
                GEOARROW_OK);
      EXPECT_EQ(tester.WKTValues("null"),
                (std::vector<std::string>{"POINT (30 10)", "null", "POINT (40 20)",
                                          "null", "POINT EMPTY"}));
      GeoArrowArrayReaderReset(&reader);
      ArrowArrayRelease(&array);
    }
    GeoArrowError error{};
    EXPECT_EQ(GeoArrowArrayWriterAppend(&writer, {nullptr, 0}, &error), EINVAL);
    EXPECT_NE(error.message[0], '\0');
    GeoArrowArrayWriterReset(&writer);
  }
}

TEST(ArrayWriterTest, AppendGeometryOptionsAndErrors) {
  WKXTester tester;
  GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayWriterSetPrecision(&writer, 2), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayWriterSetFlatMultipoint(&writer, 0), GEOARROW_OK);
  auto geom = GeoArrowGeometryAsView(&tester.AsGeometry("MULTIPOINT ((1.234 5.678))"));
  ASSERT_EQ(GeoArrowArrayWriterAppend(&writer, geom, nullptr), GEOARROW_OK);
  ArrowArray array;
  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  auto offsets = static_cast<const int32_t*>(array.buffers[1]);
  EXPECT_EQ(std::string(static_cast<const char*>(array.buffers[2]), offsets[1]),
            "MULTIPOINT ((1.23 5.68))");
  ArrowArrayRelease(&array);
  GeoArrowArrayWriterReset(&writer);

  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_POINT), GEOARROW_OK);
  geom = GeoArrowGeometryAsView(&tester.AsGeometry("LINESTRING (1 2, 3 4)"));
  GeoArrowError error{};
  EXPECT_EQ(GeoArrowArrayWriterAppend(&writer, geom, &error), EINVAL);
  EXPECT_NE(error.message[0], '\0');
  GeoArrowArrayWriterReset(&writer);
}
