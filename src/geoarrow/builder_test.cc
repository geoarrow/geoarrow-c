
#include <cmath>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/wkx_testing.hpp"

static void CustomFreeDoubleVector(uint8_t* ptr, int64_t size, void* private_data) {
  GEOARROW_UNUSED(ptr);
  GEOARROW_UNUSED(size);
  auto vector = reinterpret_cast<std::vector<double>*>(private_data);
  delete vector;
}

TEST(BuilderTest, BuilderTestOwnedBuffer) {
  struct GeoArrowBuilder builder;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);

  // Array release should delete these objects if our deallocator worked
  std::vector<double>* xs = new std::vector<double>();
  xs->push_back(123);
  std::vector<double>* ys = new std::vector<double>();
  ys->push_back(456);

  struct GeoArrowBufferView view;
  view.data = reinterpret_cast<const uint8_t*>(xs->data());
  view.size_bytes = xs->size() * sizeof(double);
  EXPECT_EQ(GeoArrowBuilderSetOwnedBuffer(&builder, 1, view, &CustomFreeDoubleVector, xs),
            GEOARROW_OK);

  view.data = reinterpret_cast<const uint8_t*>(ys->data());
  view.size_bytes = ys->size() * sizeof(double);
  EXPECT_EQ(GeoArrowBuilderSetOwnedBuffer(&builder, 2, view, &CustomFreeDoubleVector, ys),
            GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array, nullptr), GEOARROW_OK);

  EXPECT_EQ(reinterpret_cast<const double*>(array.children[0]->buffers[1])[0], 123);
  EXPECT_EQ(reinterpret_cast<const double*>(array.children[1]->buffers[1])[0], 456);

  array.release(&array);
  GeoArrowBuilderReset(&builder);
}

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, BuilderTestInit) {
  struct GeoArrowBuilder builder;
  struct ArrowSchema schema;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
  EXPECT_EQ(builder.view.schema_view.type, type);
  GeoArrowBuilderReset(&builder);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderInitFromSchema(&builder, &schema, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);
  schema.release(&schema);
}

TEST_P(TypeParameterizedTestFixture, BuilderTestEmpty) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;
  array_out.release = nullptr;
  GeoArrowError error;
  error.message[0] = '\0';
  enum GeoArrowType type = GetParam();

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
  ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array_out, &error), GEOARROW_OK)
      << error.message;
  EXPECT_NE(array_out.release, nullptr);
  GeoArrowBuilderReset(&builder);

  // Make sure this is a valid zero-length array
  struct ArrowArrayView array_view;
  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  EXPECT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema, nullptr), GEOARROW_OK);

  ArrowArrayViewReset(&array_view);
  array_out.release(&array_out);
  schema.release(&schema);
}

INSTANTIATE_TEST_SUITE_P(
    BuilderTest, TypeParameterizedTestFixture,
    ::testing::Values(
        GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKB, GEOARROW_TYPE_WKT,
        GEOARROW_TYPE_LARGE_WKT, GEOARROW_TYPE_BOX, GEOARROW_TYPE_BOX_Z,
        GEOARROW_TYPE_BOX_M, GEOARROW_TYPE_BOX_ZM,

        GEOARROW_TYPE_POINT, GEOARROW_TYPE_LINESTRING, GEOARROW_TYPE_POLYGON,
        GEOARROW_TYPE_MULTIPOINT, GEOARROW_TYPE_MULTILINESTRING,
        GEOARROW_TYPE_MULTIPOLYGON,

        GEOARROW_TYPE_POINT_Z, GEOARROW_TYPE_LINESTRING_Z, GEOARROW_TYPE_POLYGON_Z,
        GEOARROW_TYPE_MULTIPOINT_Z, GEOARROW_TYPE_MULTILINESTRING_Z,
        GEOARROW_TYPE_MULTIPOLYGON_Z,

        GEOARROW_TYPE_POINT_M, GEOARROW_TYPE_LINESTRING_M, GEOARROW_TYPE_POLYGON_M,
        GEOARROW_TYPE_MULTIPOINT_M, GEOARROW_TYPE_MULTILINESTRING_M,
        GEOARROW_TYPE_MULTIPOLYGON_M,

        GEOARROW_TYPE_POINT_ZM, GEOARROW_TYPE_LINESTRING_ZM, GEOARROW_TYPE_POLYGON_ZM,
        GEOARROW_TYPE_MULTIPOINT_ZM, GEOARROW_TYPE_MULTILINESTRING_ZM,
        GEOARROW_TYPE_MULTIPOLYGON_ZM,

        GEOARROW_TYPE_INTERLEAVED_POINT, GEOARROW_TYPE_INTERLEAVED_LINESTRING,
        GEOARROW_TYPE_INTERLEAVED_POLYGON, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING, GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON,
        GEOARROW_TYPE_INTERLEAVED_POINT_Z, GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z,
        GEOARROW_TYPE_INTERLEAVED_POLYGON_Z, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_Z,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_Z,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_Z, GEOARROW_TYPE_INTERLEAVED_POINT_M,
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_M, GEOARROW_TYPE_INTERLEAVED_POLYGON_M,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_M,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_M,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_M, GEOARROW_TYPE_INTERLEAVED_POINT_ZM,
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM, GEOARROW_TYPE_INTERLEAVED_POLYGON_ZM,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_ZM,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_ZM,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_ZM));

TEST(BuilderTest, BuilderTestSerializedFromBuffers) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;
  struct GeoArrowError error;

  uint8_t validity = 0b00000101;
  std::vector<int32_t> offsets32 = {0, 11, 11, 21};
  std::vector<int64_t> offsets64 = {0, 11, 11, 21};
  std::string data = "POINT (0 1)POINT(1 2)";

  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_LARGE_WKT}) {
    SCOPED_TRACE(type);

    ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, {&validity, 1}), GEOARROW_OK);

    if (type == GEOARROW_TYPE_WKT) {
      ASSERT_EQ(GeoArrowBuilderAppendBuffer(
                    &builder, 1,
                    {reinterpret_cast<uint8_t*>(offsets32.data()),
                     static_cast<int64_t>(offsets32.size() * sizeof(int32_t))}),
                GEOARROW_OK);
    } else {
      ASSERT_EQ(GeoArrowBuilderAppendBuffer(
                    &builder, 1,
                    {reinterpret_cast<uint8_t*>(offsets64.data()),
                     static_cast<int64_t>(offsets64.size() * sizeof(int64_t))}),
                GEOARROW_OK);
    }

    ASSERT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2,
                                          {reinterpret_cast<uint8_t*>(data.data()),
                                           static_cast<int64_t>(data.size())}),
              GEOARROW_OK);
    ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array_out, &error), GEOARROW_OK)
        << error.message;
    GeoArrowBuilderReset(&builder);

    ASSERT_EQ(array_out.length, 3);
    ASSERT_EQ(array_out.null_count, -1);

    struct GeoArrowArrayReader reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array_out, nullptr), GEOARROW_OK);

    WKXTester tester;
    ASSERT_EQ(GeoArrowArrayReaderVisit(&reader, 0, 3, tester.WKTVisitor()), GEOARROW_OK);
    auto values = tester.WKTValues("<null value>");
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], "POINT (0 1)");
    EXPECT_EQ(values[1], "<null value>");
    EXPECT_EQ(values[2], "POINT (1 2)");

    GeoArrowArrayReaderReset(&reader);
    array_out.release(&array_out);
  }
}

TEST(BuilderTest, BuilderTestAppendCoords) {
  struct GeoArrowBuilder builder;
  struct GeoArrowArrayView array_view;
  struct ArrowArray array_out;

  TestCoords coords({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                    {16, 17, 18, 19, 20});

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT_ZM), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderCoordsAppend(&builder, coords.view(), GEOARROW_DIMENSIONS_XYZM,
                                        1, 3),
            GEOARROW_OK);

  ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT_ZM),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT ZM (2 7 12 17)");
  EXPECT_EQ(values[1], "POINT ZM (3 8 13 18)");
  EXPECT_EQ(values[2], "POINT ZM (4 9 14 19)");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilerTestSetBuffersBox) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [BOX (0 1 2 3), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<double> xmins = {0, 0, 0};
  std::vector<double> ymins = {1, 0, 0};
  std::vector<double> xmaxs = {2, 0, 0};
  std::vector<double> ymaxs = {3, 0, 0};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_BOX), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(xmins)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(ymins)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 3, MakeBufferView(xmaxs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 4, MakeBufferView(ymaxs)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 3);
  EXPECT_EQ(array_out.children[1]->length, 3);
  EXPECT_EQ(array_out.children[2]->length, 3);
  EXPECT_EQ(array_out.children[3]->length, 3);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_BOX), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POLYGON ((0 1, 2 1, 2 3, 0 3, 0 1))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilerTestSetBuffersPoint) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [POINT (30 10), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<double> xs = {30, 0, 0};
  std::vector<double> ys = {10, 0, 0};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 3);
  EXPECT_EQ(array_out.children[1]->length, 3);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestSetBuffersLinestring) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [LINESTRING (30 10, 0 1), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<int32_t> offset0 = {0, 2, 2, 2};
  std::vector<double> xs = {30, 0};
  std::vector<double> ys = {10, 1};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_LINESTRING), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(offset0)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 3, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 2);
  EXPECT_EQ(array_out.children[0]->children[0]->length, 2);
  EXPECT_EQ(array_out.children[0]->children[1]->length, 2);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_LINESTRING),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "LINESTRING (30 10, 0 1)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestSetBuffersPolygon) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [POLYGON ((1 2, 2 3, 4 5, 1 2)), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<int32_t> offset0 = {0, 1, 1, 1};
  std::vector<int32_t> offset1 = {0, 4};
  std::vector<double> xs = {1, 2, 4, 1};
  std::vector<double> ys = {2, 3, 5, 2};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POLYGON), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(offset0)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(offset1)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 3, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 4, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 1);
  EXPECT_EQ(array_out.children[0]->children[0]->length, 4);
  EXPECT_EQ(array_out.children[0]->children[0]->children[0]->length, 4);
  EXPECT_EQ(array_out.children[0]->children[0]->children[1]->length, 4);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POLYGON),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POLYGON ((1 2, 2 3, 4 5, 1 2))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestSetBuffersMultipoint) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [MULTIPOINT (30 10, 0 1), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<int32_t> offset0 = {0, 2, 2, 2};
  std::vector<double> xs = {30, 0};
  std::vector<double> ys = {10, 1};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(offset0)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 3, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 2);
  EXPECT_EQ(array_out.children[0]->children[0]->length, 2);
  EXPECT_EQ(array_out.children[0]->children[1]->length, 2);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOINT),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTIPOINT ((30 10), (0 1))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST(ArrayTest, ArrayTestSetBuffersMultilinestring) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [MULTILINESTRING ((1 2, 2 3, 4 5, 1 2)), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<int32_t> offset0 = {0, 1, 1, 1};
  std::vector<int32_t> offset1 = {0, 4};
  std::vector<double> xs = {1, 2, 4, 1};
  std::vector<double> ys = {2, 3, 5, 2};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(offset0)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(offset1)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 3, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 4, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 1);
  EXPECT_EQ(array_out.children[0]->children[0]->length, 4);
  EXPECT_EQ(array_out.children[0]->children[0]->children[0]->length, 4);
  EXPECT_EQ(array_out.children[0]->children[0]->children[1]->length, 4);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTILINESTRING ((1 2, 2 3, 4 5, 1 2))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestSetBuffersMultipolygon) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2))), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<int32_t> offset0 = {0, 1, 1, 1};
  std::vector<int32_t> offset1 = {0, 1};
  std::vector<int32_t> offset2 = {0, 4};
  std::vector<double> xs = {1, 2, 4, 1};
  std::vector<double> ys = {2, 3, 5, 2};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(offset0)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(offset1)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 3, MakeBufferView(offset2)),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 4, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 5, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 1);
  EXPECT_EQ(array_out.children[0]->children[0]->length, 1);
  EXPECT_EQ(array_out.children[0]->children[0]->children[0]->length, 4);
  EXPECT_EQ(array_out.children[0]->children[0]->children[0]->children[0]->length, 4);
  EXPECT_EQ(array_out.children[0]->children[0]->children[0]->children[1]->length, 4);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}

TEST_P(TypeParameterizedTestFixture, BuilderAppendGeometry) {
  GeoArrowBuilder builder;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GetParam()), GEOARROW_OK);
  auto type = builder.view.schema_view.geometry_type;
  GeoArrowGeometryView null_view = {nullptr, 0};
  if (type < GEOARROW_GEOMETRY_TYPE_POINT || type > GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON) {
    EXPECT_EQ(GeoArrowBuilderAppendGeometry(&builder, null_view, nullptr), ENOTSUP);
    GeoArrowBuilderReset(&builder);
    return;
  }

  auto dims = builder.view.schema_view.dimensions;
  std::string dim_suffix[] = {"", "", " Z", " M", " ZM"};
  std::string coord[] = {"", "1 2", "1 2 3", "1 2 3", "1 2 3 4"};
  std::string sequence = coord[dims] + ", " + coord[dims];
  std::string contents[] = {"",
                            "(" + coord[dims] + ")",
                            "(" + sequence + ")",
                            "((" + sequence + "))",
                            "((" + coord[dims] + "))",
                            "((" + sequence + "))",
                            "(((" + sequence + ")))"};
  std::string prefix = std::string(GeoArrowGeometryTypeString(type)) + dim_suffix[dims];
  std::string wkt = prefix + " " + contents[type];
  std::string empty = prefix + " EMPTY";
  WKXTester tester;
  GeoArrowGeometry result;
  ASSERT_EQ(GeoArrowGeometryInit(&result), GEOARROW_OK);
  GeoArrowError error;

  // Reuse the builder across batches, including null-first and entirely empty
  // batches. Cross a bitmap byte boundary, and overwrite the borrowed input.
  for (int batch = 0; batch < 3; batch++) {
    for (int i = 0; i < 12; i++) {
      bool is_null = (i + batch) % 3 == 1;
      const auto& geom = tester.AsGeometry(i % 3 == 0 && batch != 2 ? wkt : empty);
      ASSERT_EQ(
          GeoArrowBuilderAppendGeometry(
              &builder, is_null ? null_view : GeoArrowGeometryAsView(&geom), &error),
          GEOARROW_OK)
          << error.message;
    }
    tester.AsGeometry("POINT (9 10)");
    ArrowArray array;
    ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array, &error), GEOARROW_OK)
        << error.message;
    EXPECT_EQ(array.length, 12);
    EXPECT_EQ(array.null_count, 4);
    GeoArrowArrayView view;
    ASSERT_EQ(GeoArrowArrayViewInitFromType(&view, GetParam()), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayViewSetArray(&view, &array, &error), GEOARROW_OK);
    for (int i = 0; i < 12; i++) {
      ASSERT_EQ(GeoArrowArrayViewGetGeometry(&view, i, &result), GEOARROW_OK);
      if ((i + batch) % 3 == 1) {
        EXPECT_EQ(result.root->geometry_type, 0);
      } else {
        EXPECT_EQ(tester.AsWKT(result), i % 3 == 0 && batch != 2 ? wkt : empty);
      }
    }
    array.release(&array);
  }
  GeoArrowGeometryReset(&result);
  GeoArrowBuilderReset(&builder);
}

TEST(BuilderTest, AppendGeometryPreservesEmptyParts) {
  WKXTester tester;
  for (const auto& wkt :
       {"MULTIPOINT (EMPTY, (1 2), EMPTY)", "MULTILINESTRING (EMPTY, (1 2, 3 4), EMPTY)",
        "POLYGON (EMPTY, (1 2, 3 4, 1 2), EMPTY)",
        "MULTIPOLYGON (EMPTY, ((1 2, 3 4, 1 2), EMPTY), EMPTY)",
        "MULTIPOINT (EMPTY, EMPTY)", "MULTILINESTRING (EMPTY, EMPTY)",
        "POLYGON (EMPTY, EMPTY)", "MULTIPOLYGON (EMPTY, (EMPTY), EMPTY)"}) {
    SCOPED_TRACE(wkt);
    // Read WKB directly so all-empty containers retain their child counts.
    auto wkb = tester.AsWKB(wkt);
    GeoArrowWKBReader reader;
    ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
    GeoArrowGeometryView geom;
    ASSERT_EQ(
        GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                              &geom, nullptr),
        GEOARROW_OK);
    auto type =
        GeoArrowMakeType(static_cast<GeoArrowGeometryType>(geom.root->geometry_type),
                         GEOARROW_DIMENSIONS_XY, GEOARROW_COORD_TYPE_SEPARATE);
    GeoArrowBuilder builder;
    ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
    for (int i = 0; i < 2; i++) {
      ASSERT_EQ(GeoArrowBuilderAppendGeometry(&builder, geom, nullptr), GEOARROW_OK);
    }
    ArrowArray array;
    ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array, nullptr), GEOARROW_OK);
    EXPECT_EQ(array.null_count, 0);
    EXPECT_EQ(array.buffers[0], nullptr);
    GeoArrowArrayView view;
    ASSERT_EQ(GeoArrowArrayViewInitFromType(&view, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);
    GeoArrowGeometry out;
    ASSERT_EQ(GeoArrowGeometryInit(&out), GEOARROW_OK);
    for (int i = 0; i < 2; i++) {
      ASSERT_EQ(GeoArrowArrayViewGetGeometry(&view, i, &out), GEOARROW_OK);
      EXPECT_EQ(out.size_nodes, geom.size_nodes);
      EXPECT_EQ(tester.AsWKT(out), wkt);
    }
    GeoArrowGeometryReset(&out);
    array.release(&array);
    GeoArrowBuilderReset(&builder);
    GeoArrowWKBReaderReset(&reader);
  }
}

TEST(BuilderTest, AppendGeometryStridesDimensionsAndEndian) {
  for (auto layout : {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
    for (int swap : {0, 1}) {
      // Unaligned reversed X, constant Y, and no Z/M in the source.
      uint8_t bytes[25];
      double values[] = {1, 3, 2};
      for (int i = 0; i < 3; i++) {
        uint64_t value;
        memcpy(&value, values + i, sizeof(value));
        if (swap) value = GEOARROW_BSWAP64(value);
        memcpy(bytes + 1 + i * sizeof(value), &value, sizeof(value));
      }
      GeoArrowGeometryNode node = {};
      node.geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
      node.dimensions = GEOARROW_DIMENSIONS_XY;
      node.flags =
          static_cast<uint8_t>(swap ? GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN : 0);
      node.size = 2;
      node.coords[0] = bytes + 9;
      node.coords[1] = bytes + 17;
      node.coord_stride[0] = -8;
      GeoArrowBuilder builder;
      auto type = GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                   GEOARROW_DIMENSIONS_XYZM, layout);
      ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
      ASSERT_EQ(GeoArrowBuilderAppendGeometry(&builder, {&node, 1}, nullptr),
                GEOARROW_OK);
      ArrowArray array;
      ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array, nullptr), GEOARROW_OK);
      GeoArrowArrayView view;
      ASSERT_EQ(GeoArrowArrayViewInitFromType(&view, type), GEOARROW_OK);
      ASSERT_EQ(GeoArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);
      EXPECT_EQ(GEOARROW_COORD_VIEW_VALUE(&view.coords, 0, 0), 3);
      EXPECT_EQ(GEOARROW_COORD_VIEW_VALUE(&view.coords, 1, 0), 1);
      EXPECT_EQ(GEOARROW_COORD_VIEW_VALUE(&view.coords, 0, 1), 2);
      EXPECT_EQ(GEOARROW_COORD_VIEW_VALUE(&view.coords, 1, 1), 2);
      EXPECT_TRUE(std::isnan(GEOARROW_COORD_VIEW_VALUE(&view.coords, 0, 2)));
      EXPECT_TRUE(std::isnan(GEOARROW_COORD_VIEW_VALUE(&view.coords, 1, 3)));
      array.release(&array);
      GeoArrowBuilderReset(&builder);
    }
  }

  WKXTester tester;
  const auto& geom = tester.AsGeometry("POINT ZM (1 2 3 4)");
  GeoArrowBuilder builder;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT_M), GEOARROW_OK);
  ASSERT_EQ(
      GeoArrowBuilderAppendGeometry(&builder, GeoArrowGeometryAsView(&geom), nullptr),
      GEOARROW_OK);
  ArrowArray array;
  ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(static_cast<const double*>(array.children[2]->buffers[1])[0], 4);
  array.release(&array);
  GeoArrowBuilderReset(&builder);
}

TEST(BuilderTest, AppendGeometryInvalidAndOverflow) {
  GeoArrowBuilder builder;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  WKXTester tester;
  const auto& geom = tester.AsGeometry("MULTIPOLYGON (((1 2, 3 4)))");
  std::vector<GeoArrowGeometryNode> nodes(geom.root, geom.root + geom.size_nodes);
  ASSERT_EQ(
      GeoArrowBuilderAppendGeometry(&builder, GeoArrowGeometryAsView(&geom), nullptr),
      GEOARROW_OK);
  auto reject = [&](GeoArrowGeometryView view, int code) {
    int64_t sizes[9];
    for (int i = 0; i < builder.view.n_buffers; i++)
      sizes[i] = builder.view.buffers[i].size_bytes;
    auto size_coords = builder.view.coords.size_coords;
    EXPECT_EQ(GeoArrowBuilderAppendGeometry(&builder, view, nullptr), code);
    EXPECT_EQ(builder.view.coords.size_coords, size_coords);
    for (int i = 0; i < builder.view.n_buffers; i++)
      EXPECT_EQ(builder.view.buffers[i].size_bytes, sizes[i]);
  };
  reject({nullptr, 1}, EINVAL);
  reject({nullptr, -1}, EINVAL);
  reject({nodes.data(), 2}, EINVAL);
  nodes.push_back(nodes.back());
  reject({nodes.data(), 4}, EINVAL);
  nodes.pop_back();
  nodes[1].level = 2;
  reject({nodes.data(), 3}, EINVAL);
  nodes[1].level = 1;
  nodes[1].geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
  reject({nodes.data(), 3}, EINVAL);
  nodes[1].geometry_type = GEOARROW_GEOMETRY_TYPE_POLYGON;
  nodes[2].dimensions = 0;
  reject({nodes.data(), 3}, EINVAL);
  nodes[2].dimensions = GEOARROW_DIMENSIONS_XY;
  nodes[2].size = UINT32_MAX;
  reject({nodes.data(), 3}, EOVERFLOW);
  nodes[2].size = INT32_MAX;  // Individually valid, but overflows the existing offset.
  reject({nodes.data(), 3}, EOVERFLOW);
  nodes[0].geometry_type = 0;
  reject({nodes.data(), 1}, EINVAL);

  // A failed append leaves the preceding geometry intact and allows another append.
  ASSERT_EQ(
      GeoArrowBuilderAppendGeometry(&builder, GeoArrowGeometryAsView(&geom), nullptr),
      GEOARROW_OK);
  GeoArrowGeometryNode null_node = {};
  ASSERT_EQ(GeoArrowBuilderAppendGeometry(&builder, {&null_node, 1}, nullptr),
            GEOARROW_OK);
  ArrowArray array;
  ASSERT_EQ(GeoArrowBuilderFinish(&builder, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 3);
  EXPECT_EQ(array.null_count, 1);
  EXPECT_EQ(array.children[0]->children[0]->children[0]->length, 4);
  array.release(&array);
  GeoArrowBuilderReset(&builder);

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);
  nodes[0].geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  nodes[0].size = 2;
  EXPECT_EQ(GeoArrowBuilderAppendGeometry(&builder, {nodes.data(), 1}, nullptr), EINVAL);
  GeoArrowBuilderReset(&builder);
}
