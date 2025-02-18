
#include <cmath>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/wkx_testing.hpp"

// Such that kNumOffsets[geometry_type] gives the right answer
static int kNumOffsets[] = {-1, 0, 1, 2, 1, 2, 3, -1};

// Such that kNumDimensions[dimensions] gives the right answer
static int kNumDimensions[] = {-1, 2, 3, 3, 4};

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, ArrayViewTestInitType) {
  struct GeoArrowArrayView array_view;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(array_view.schema_view.type, type);
  EXPECT_EQ(array_view.length[0], 0);
  EXPECT_EQ(array_view.validity_bitmap, nullptr);
  EXPECT_EQ(array_view.n_offsets, kNumOffsets[array_view.schema_view.geometry_type]);
  EXPECT_EQ(array_view.coords.n_coords, 0);

  if (array_view.schema_view.geometry_type == GEOARROW_GEOMETRY_TYPE_BOX) {
    EXPECT_EQ(array_view.coords.n_values,
              kNumDimensions[array_view.schema_view.dimensions] * 2);

  } else {
    EXPECT_EQ(array_view.coords.n_values,
              kNumDimensions[array_view.schema_view.dimensions]);
  }

  if (array_view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    EXPECT_EQ(array_view.coords.coords_stride, 1);
  } else {
    EXPECT_EQ(array_view.coords.coords_stride,
              kNumDimensions[array_view.schema_view.dimensions]);
  }
}

TEST_P(TypeParameterizedTestFixture, ArrayViewTestInitSchema) {
  struct GeoArrowArrayView array_view;
  struct ArrowSchema schema;
  enum GeoArrowType type = GetParam();

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewInitFromSchema(&array_view, &schema, nullptr), GEOARROW_OK);
  EXPECT_EQ(array_view.schema_view.type, type);

  schema.release(&schema);
}

TEST_P(TypeParameterizedTestFixture, ArrayViewTestInitEmptyArray) {
  struct GeoArrowArrayView array_view;
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GetParam();

  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  schema.release(&schema);
  array.release(&array);
}

INSTANTIATE_TEST_SUITE_P(
    ArrayViewTest, TypeParameterizedTestFixture,
    ::testing::Values(
        GEOARROW_TYPE_BOX, GEOARROW_TYPE_BOX_Z, GEOARROW_TYPE_BOX_M, GEOARROW_TYPE_BOX_ZM,

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
        GEOARROW_TYPE_MULTIPOLYGON_ZM, GEOARROW_TYPE_INTERLEAVED_POINT,
        GEOARROW_TYPE_INTERLEAVED_LINESTRING, GEOARROW_TYPE_INTERLEAVED_POLYGON,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT, GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON, GEOARROW_TYPE_INTERLEAVED_POINT_Z,
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z, GEOARROW_TYPE_INTERLEAVED_POLYGON_Z,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_Z,
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

TEST(ArrayViewTest, ArrayViewTestSetArrayErrors) {
  struct GeoArrowArrayView array_view;
  struct GeoArrowError error;
  struct ArrowArray array;
  struct ArrowArray dummy_childx;
  struct ArrowArray* children[] = {&dummy_childx, &dummy_childx, &dummy_childx,
                                   &dummy_childx};

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_BOX), GEOARROW_OK);
  array.offset = 0;
  array.n_children = 1;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Unexpected number of children for box array struct in "
               "GeoArrowArrayViewSetArray()");

  array.n_children = 4;
  array.children = reinterpret_cast<struct ArrowArray**>(children);
  dummy_childx.n_buffers = 1;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Unexpected number of buffers for box array child in "
               "GeoArrowArrayViewSetArray()");

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT), GEOARROW_OK);
  array.n_children = 0;
  array.children = nullptr;
  array.offset = 0;
  array.n_children = 1;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Unexpected number of children for struct coordinate array in "
               "GeoArrowArrayViewSetArray()");

  array.n_children = 2;
  array.children = reinterpret_cast<struct ArrowArray**>(children);
  dummy_childx.n_buffers = 1;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Unexpected number of buffers for struct coordinate array child in "
               "GeoArrowArrayViewSetArray()");

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_LINESTRING),
            GEOARROW_OK);
  array.n_buffers = 0;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Unexpected number of buffers in list array in GeoArrowArrayViewSetArray()");

  array.n_buffers = 2;
  array.n_children = 0;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Unexpected number of children in list array in GeoArrowArrayViewSetArray()");
}

TEST(ArrayViewTest, ArrayViewTestSetInterleavedArrayErrors) {
  struct GeoArrowArrayView array_view;
  struct GeoArrowError error;
  struct ArrowArray array;

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_INTERLEAVED_POINT),
            GEOARROW_OK);

  array.offset = 0;
  array.n_children = 0;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Unexpected number of children for interleaved coordinate array in "
               "GeoArrowArrayViewSetArray()");

  struct ArrowArray dummy_childxy;
  struct ArrowArray* children[] = {&dummy_childxy};
  array.n_children = 1;
  array.children = reinterpret_cast<struct ArrowArray**>(children);
  dummy_childxy.n_buffers = 1;
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Unexpected number of buffers for interleaved coordinate array child in "
               "GeoArrowArrayViewSetArray()");
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidBox) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_BOX;

  // Build the array for [BOX (0 1 => 2 3), BOX EMPTY, null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 0), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[2], 2), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[3], 3), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], INFINITY), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], INFINITY), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[2], -INFINITY), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[3], -INFINITY), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 3);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 2));
  EXPECT_EQ(array_view.coords.n_values, 4);
  EXPECT_EQ(array_view.coords.n_coords, 3);
  EXPECT_EQ(array_view.coords.values[0][0], 0);
  EXPECT_EQ(array_view.coords.values[1][0], 1);
  EXPECT_EQ(array_view.coords.values[2][0], 2);
  EXPECT_EQ(array_view.coords.values[3][0], 3);

  EXPECT_EQ(array_view.coords.values[0][1], INFINITY);
  EXPECT_EQ(array_view.coords.values[1][1], INFINITY);
  EXPECT_EQ(array_view.coords.values[2][1], -INFINITY);
  EXPECT_EQ(array_view.coords.values[3][1], -INFINITY);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POLYGON ((0 1, 2 1, 2 3, 0 3, 0 1))");
  EXPECT_EQ(values[1], "POLYGON EMPTY");
  EXPECT_EQ(values[2], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidBoxNonXY) {
  struct ArrowSchema schema;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_BOX_Z), GEOARROW_OK);
  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_BOX_Z), GEOARROW_OK);

  struct GeoArrowVisitor v;
  GeoArrowVisitorInitVoid(&v);
  v.error = &error;
  ASSERT_EQ(GeoArrowArrayViewVisitNative(&array_view, 0, 0, &v), ENOTSUP);
  ASSERT_STREQ(error.message, "Can't visit box with non-XY dimensions");

  ArrowSchemaRelease(&schema);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidPoint) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_POINT;

  // Build the array for [POINT (30 10), null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_EQ(array_view.coords.n_coords, 2);
  EXPECT_EQ(array_view.coords.values[0][0], 30);
  EXPECT_EQ(array_view.coords.values[1][0], 10);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidInterleavedPoint) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_INTERLEAVED_POINT;

  // Build the array for [POINT (30 10), null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_EQ(array_view.coords.n_coords, 2);
  EXPECT_EQ(array_view.coords.values[0][0], 30);
  EXPECT_EQ(array_view.coords.values[1][0], 10);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidPointWithOffset) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_POINT;

  // Build the array for [null, POINT (31 11)]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  // First null won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // First ordinate won't be used because of offsets on struct children
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Actual first ordinate
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 31), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], 11), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset to the array and to each ordinate array
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 1;
  array.children[0]->length -= 1;
  array.children[1]->offset = 1;
  array.children[1]->length -= 1;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "POINT (31 11)");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidInterleavedPointWithOffset) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_INTERLEAVED_POINT;

  // Build the array for [null, POINT (31 11)]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  // First null won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // First ordinate won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Actual first ordinate
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 31), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 11), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset to the array and to each ordinate array
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 2;
  array.children[0]->length -= 2;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "POINT (31 11)");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidLinestring) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_LINESTRING;

  // Build the array for [LINESTRING (30 10, 0 1), null, null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 0), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 3);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 2));
  EXPECT_EQ(array_view.n_offsets, 1);
  EXPECT_EQ(array_view.last_offset[0], 2);
  EXPECT_EQ(array_view.offsets[0][0], 0);
  EXPECT_EQ(array_view.offsets[0][1], 2);

  EXPECT_EQ(array_view.coords.n_coords, 2);
  EXPECT_EQ(array_view.coords.values[0][0], 30);
  EXPECT_EQ(array_view.coords.values[1][0], 10);
  EXPECT_EQ(array_view.coords.values[0][1], 0);
  EXPECT_EQ(array_view.coords.values[1][1], 1);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "LINESTRING (30 10, 0 1)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidLinestringWithOffset) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_LINESTRING;

  // Build the array for [null, null, LINESTRING (0 1, 2 3, 4 5)]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // This first point will be skipped by the offset
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 0), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 2), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 3), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Add an extra point so we can offset into that array
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 4), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 5), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset to each level
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 1;
  array.children[0]->length -= 1;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_EQ(array_view.offset[1], 1);
  EXPECT_EQ(array_view.length[1], 3);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "LINESTRING (0 1, 2 3, 4 5)");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidPolygon) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_POLYGON;

  // Build the array for [POLYGON ((1 2, 2 3, 4 5, 1 2)), null, null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 3),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 4),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 5),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 3);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 2));
  EXPECT_EQ(array_view.n_offsets, 2);
  EXPECT_EQ(array_view.last_offset[0], 1);
  EXPECT_EQ(array_view.last_offset[1], 4);
  EXPECT_EQ(array_view.offsets[0][0], 0);
  EXPECT_EQ(array_view.offsets[0][1], 1);
  EXPECT_EQ(array_view.offsets[1][0], 0);
  EXPECT_EQ(array_view.offsets[1][1], 4);

  EXPECT_EQ(array_view.coords.n_coords, 4);
  EXPECT_EQ(array_view.coords.values[0][0], 1);
  EXPECT_EQ(array_view.coords.values[1][0], 2);
  EXPECT_EQ(array_view.coords.values[0][1], 2);
  EXPECT_EQ(array_view.coords.values[1][1], 3);
  EXPECT_EQ(array_view.coords.values[0][2], 4);
  EXPECT_EQ(array_view.coords.values[1][2], 5);
  EXPECT_EQ(array_view.coords.values[0][3], 1);
  EXPECT_EQ(array_view.coords.values[1][3], 2);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POLYGON ((1 2, 2 3, 4 5, 1 2))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidPolygonWithOffset) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_POLYGON;

  // Build the array for [null, null, POLYGON ((1 2, 2 3, 4 5, 1 2))]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // First ring won't be used because we will set the offset 1
  ASSERT_EQ(ArrowArrayAppendEmpty(array.children[0], 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Add the actual ring that will be used
  // First coord won't be used because we will set the offset to 1
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], -1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], -1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 3),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 4),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 5),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  // ..and add the actual last coordinate
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset at every level
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 1;
  array.children[0]->length -= 1;
  array.children[0]->children[0]->offset = 1;
  array.children[0]->children[0]->length -= 1;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_EQ(array_view.offset[1], 1);
  EXPECT_EQ(array_view.length[1], 1);
  EXPECT_EQ(array_view.offset[2], 1);
  EXPECT_EQ(array_view.length[2], 4);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "POLYGON ((1 2, 2 3, 4 5, 1 2))");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidMultipoint) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_MULTIPOINT;

  // Build the array for [MULTIPOINT (30 10, 0 1), null, null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 0), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 3);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 2));
  EXPECT_EQ(array_view.n_offsets, 1);
  EXPECT_EQ(array_view.last_offset[0], 2);
  EXPECT_EQ(array_view.offsets[0][0], 0);
  EXPECT_EQ(array_view.offsets[0][1], 2);

  EXPECT_EQ(array_view.coords.n_coords, 2);
  EXPECT_EQ(array_view.coords.values[0][0], 30);
  EXPECT_EQ(array_view.coords.values[1][0], 10);
  EXPECT_EQ(array_view.coords.values[0][1], 0);
  EXPECT_EQ(array_view.coords.values[1][1], 1);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTIPOINT ((30 10), (0 1))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidMultiPointWithOffset) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_MULTIPOINT;

  // Build the array for [null, MULTIPOINT (0 1, 2 3, 4 5)]
  // (with an offset at every level)
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  // First null won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // This first point will be skipped by the offset
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], -1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 0), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 2), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 3), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Add an extra point so we can offset into that array
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0], 4), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[1], 5), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset to each level
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 1;
  array.children[0]->length -= 1;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_EQ(array_view.offset[1], 1);
  EXPECT_EQ(array_view.length[1], 3);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "MULTIPOINT ((0 1), (2 3), (4 5))");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidMultilinestring) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_MULTILINESTRING;

  // Build the array for [MULTILINESTRING ((30 10, 0 1), (31, 11, 1 2)), null, null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 30),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 10),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 0),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 31),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 11),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 3);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 2));
  EXPECT_EQ(array_view.n_offsets, 2);
  EXPECT_EQ(array_view.last_offset[0], 2);
  EXPECT_EQ(array_view.last_offset[1], 4);
  EXPECT_EQ(array_view.offsets[0][0], 0);
  EXPECT_EQ(array_view.offsets[0][1], 2);
  EXPECT_EQ(array_view.offsets[1][0], 0);
  EXPECT_EQ(array_view.offsets[1][1], 2);
  EXPECT_EQ(array_view.offsets[1][2], 4);

  EXPECT_EQ(array_view.coords.n_coords, 4);
  EXPECT_EQ(array_view.coords.values[0][0], 30);
  EXPECT_EQ(array_view.coords.values[1][0], 10);
  EXPECT_EQ(array_view.coords.values[0][1], 0);
  EXPECT_EQ(array_view.coords.values[1][1], 1);
  EXPECT_EQ(array_view.coords.values[0][2], 31);
  EXPECT_EQ(array_view.coords.values[1][2], 11);
  EXPECT_EQ(array_view.coords.values[0][3], 1);
  EXPECT_EQ(array_view.coords.values[1][3], 2);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTILINESTRING ((30 10, 0 1), (31 11, 1 2))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidMultiLinestringWithOffset) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_MULTILINESTRING;

  // Build the array for [null, MULTILINESTRING ((1 2, 2 3, 4 5, 1 2))]
  // (using an offset at every level)
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  // First null won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // First ring won't be used because we will set the offset 1
  ASSERT_EQ(ArrowArrayAppendEmpty(array.children[0], 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Add the actual ring that will be used
  // First coord won't be used because we will set the offset to 1
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], -1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], -1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 3),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 4),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 5),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  // ..and add the actual last coordinate
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[0], 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0]->children[0]->children[1], 2),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset at every level
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 1;
  array.children[0]->length -= 1;
  array.children[0]->children[0]->offset = 1;
  array.children[0]->children[0]->length -= 1;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_EQ(array_view.offset[1], 1);
  EXPECT_EQ(array_view.length[1], 1);
  EXPECT_EQ(array_view.offset[2], 1);
  EXPECT_EQ(array_view.length[2], 4);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "MULTILINESTRING ((1 2, 2 3, 4 5, 1 2))");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidMultipolygon) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_MULTIPOLYGON;

  // Build the array for [MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2))), null, null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 1),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 2),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 2),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 3),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 4),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 5),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 1),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 2),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.length[0], 3);
  EXPECT_TRUE(ArrowBitGet(array_view.validity_bitmap, 0));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 1));
  EXPECT_FALSE(ArrowBitGet(array_view.validity_bitmap, 2));
  EXPECT_EQ(array_view.n_offsets, 3);
  EXPECT_EQ(array_view.last_offset[0], 1);
  EXPECT_EQ(array_view.last_offset[1], 1);
  EXPECT_EQ(array_view.last_offset[2], 4);
  EXPECT_EQ(array_view.offsets[0][0], 0);
  EXPECT_EQ(array_view.offsets[0][1], 1);
  EXPECT_EQ(array_view.offsets[1][0], 0);
  EXPECT_EQ(array_view.offsets[1][1], 1);
  EXPECT_EQ(array_view.offsets[2][0], 0);
  EXPECT_EQ(array_view.offsets[2][1], 4);

  EXPECT_EQ(array_view.coords.n_coords, 4);
  EXPECT_EQ(array_view.coords.values[0][0], 1);
  EXPECT_EQ(array_view.coords.values[1][0], 2);
  EXPECT_EQ(array_view.coords.values[0][1], 2);
  EXPECT_EQ(array_view.coords.values[1][1], 3);
  EXPECT_EQ(array_view.coords.values[0][2], 4);
  EXPECT_EQ(array_view.coords.values[1][2], 5);
  EXPECT_EQ(array_view.coords.values[0][3], 1);
  EXPECT_EQ(array_view.coords.values[1][3], 2);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  schema.release(&schema);
  array.release(&array);
}

TEST(ArrayViewTest, ArrayViewTestSetArrayValidMultipolygonWithOffsets) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_MULTIPOLYGON;

  // Build the array for [null, MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  // First NULL won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendNull(&array, 2), GEOARROW_OK);

  // Empty polygon that will be skipped because of offset
  ASSERT_EQ(ArrowArrayAppendEmpty(array.children[0], 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);

  // Empty ring that will be skipped because of offset
  ASSERT_EQ(ArrowArrayAppendEmpty(array.children[0]->children[0], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]), GEOARROW_OK);

  // The actual ring
  // First point won't be used because of offset
  ASSERT_EQ(ArrowArrayAppendDouble(
                array.children[0]->children[0]->children[0]->children[0], -1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(
                array.children[0]->children[0]->children[0]->children[1], -1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);

  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 1),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 2),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 2),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 3),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 4),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 5),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]), GEOARROW_OK);

  // The actual last point
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[0], 1),
      GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendDouble(array.children[0]->children[0]->children[0]->children[1], 2),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array.children[0]->children[0]->children[0]),
            GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Apply an offset at every level
  array.offset = 1;
  array.length -= 1;
  array.children[0]->offset = 1;
  array.children[0]->length -= 1;
  array.children[0]->children[0]->offset = 1;
  array.children[0]->children[0]->length -= 1;
  array.children[0]->children[0]->children[0]->offset = 1;
  array.children[0]->children[0]->children[0]->length -= 1;

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  EXPECT_EQ(array_view.offset[0], 1);
  EXPECT_EQ(array_view.length[0], 2);
  EXPECT_EQ(array_view.offset[1], 1);
  EXPECT_EQ(array_view.length[1], 1);
  EXPECT_EQ(array_view.offset[2], 1);
  EXPECT_EQ(array_view.length[2], 1);
  EXPECT_EQ(array_view.offset[3], 1);
  EXPECT_EQ(array_view.length[3], 4);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array.length, tester.WKTVisitor()),
      GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))");

  schema.release(&schema);
  array.release(&array);
}
