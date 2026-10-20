
#include <cmath>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/wkx_testing.hpp"

// Exercise scalar access on the same arrays used to test native iteration,
// including arrays sliced at every nesting level and at coordinate children.
static void CheckArrayViewGeometries(const struct GeoArrowArrayView* array_view,
                                     const std::vector<std::string>& expected) {
  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  WKXTester tester;
  // Read in reverse order to exercise random access and reuse after null/empty values.
  for (int64_t i = array_view->length[0] - 1; i >= 0; i--) {
    ASSERT_EQ(GeoArrowArrayViewGetGeometry(array_view, i, &geom), GEOARROW_OK);
    ASSERT_GE(geom.size_nodes, 1);
    if (expected[i] == "<null value>") {
      EXPECT_EQ(geom.size_nodes, 1);
      EXPECT_EQ(geom.root->geometry_type, GEOARROW_GEOMETRY_TYPE_GEOMETRY);
      EXPECT_EQ(geom.root->size, 0);
    } else {
      EXPECT_EQ(tester.AsWKT(geom), expected[i]);
      EXPECT_EQ(tester.AsWKTUsingVisitor(geom), expected[i]);
      const auto& parsed = tester.AsGeometry(expected[i]);
      ASSERT_EQ(geom.size_nodes, parsed.size_nodes);
      for (int64_t n = 0; n < geom.size_nodes; n++) {
        EXPECT_EQ(geom.root[n].level, parsed.root[n].level);
        EXPECT_EQ(geom.root[n].geometry_type, parsed.root[n].geometry_type);
        EXPECT_EQ(geom.root[n].dimensions, parsed.root[n].dimensions);
      }
    }

    for (int64_t n = 0; n < geom.size_nodes; n++) {
      const auto& node = geom.root[n];
      EXPECT_EQ(node.flags, 0);
      EXPECT_EQ(node.user_data, nullptr);
      for (int j = 0; j < 4; j++) {
        ASSERT_NE(node.coords[j], nullptr);
        if (node.size == 0 || j >= array_view->coords.n_values ||
            (node.geometry_type != GEOARROW_GEOMETRY_TYPE_POINT &&
             node.geometry_type != GEOARROW_GEOMETRY_TYPE_LINESTRING)) {
          double value;
          memcpy(&value, node.coords[j], sizeof(value));
          EXPECT_TRUE(std::isnan(value));
          EXPECT_EQ(node.coord_stride[j], 0);
        }
      }
    }
  }

  GeoArrowGeometryReset(&geom);
}

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
  CheckArrayViewGeometries(&array_view, values);
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

TEST(ArrayViewTest, ArrayViewGetGeometryBoxCrossingAntimeridian) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  ASSERT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_BOX), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  int child_i = 0;
  for (double value : {170.0, -10.0, -170.0, 20.0}) {
    ASSERT_EQ(ArrowArrayAppendDouble(array.children[child_i++], value), GEOARROW_OK);
  }
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_BOX), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  CheckArrayViewGeometries(
      &array_view, {"MULTIPOLYGON (((170 -10, inf -10, inf 20, 170 20, 170 -10)), "
                    "((-inf -10, -170 -10, -170 20, -inf 20, -inf -10)))"});

  array.release(&array);
  schema.release(&schema);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
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
  CheckArrayViewGeometries(&array_view, values);
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "<null value>");
  EXPECT_EQ(values[1], "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))");

  schema.release(&schema);
  array.release(&array);
}

struct ArrayViewGeometryTestCase {
  enum GeoArrowType type;
  const char* wkt;
  const char* empty_wkt;
};

class ArrayViewGeometryTestFixture
    : public ::testing::TestWithParam<ArrayViewGeometryTestCase> {};

TEST_P(ArrayViewGeometryTestFixture, ArrayViewGetGeometry) {
  const auto& test_case = GetParam();
  const std::string wkt(test_case.wkt);
  const std::string empty(test_case.empty_wkt);

  // Exercise both coordinate layouts with the same geometry fixture.
  for (auto coord_type :
       {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
    enum GeoArrowType type =
        GeoArrowMakeType(GeoArrowGeometryTypeFromType(test_case.type),
                         GeoArrowDimensionsFromType(test_case.type), coord_type);
    SCOPED_TRACE(coord_type == GEOARROW_COORD_TYPE_SEPARATE ? "separate" : "interleaved");

    struct GeoArrowArrayView array_view;
    ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);

    struct GeoArrowNativeWriter writer;
    ASSERT_EQ(GeoArrowNativeWriterInit(&writer, type), GEOARROW_OK);
    WKXTester tester;
    for (const auto& value : {wkt, std::string("<null value>"), empty, wkt}) {
      if (value == "<null value>") {
        ASSERT_EQ(GeoArrowNativeWriterAppendNull(&writer), GEOARROW_OK);
      } else {
        const auto& parsed = tester.AsGeometry(value);
        ASSERT_EQ(
            GeoArrowNativeWriterAppend(&writer, GeoArrowGeometryAsView(&parsed), nullptr),
            GEOARROW_OK);
      }
    }
    struct ArrowArray array;
    ASSERT_EQ(GeoArrowNativeWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
    GeoArrowNativeWriterReset(&writer);
    ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);
    CheckArrayViewGeometries(&array_view, {wkt, "<null value>", empty, wkt});

    struct GeoArrowGeometry geom;
    ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 0, &geom), GEOARROW_OK);
    // The first coordinate sequence must point directly into the array buffers.
    const struct GeoArrowGeometryNode* sequence_node = geom.root;
    while (sequence_node->geometry_type != GEOARROW_GEOMETRY_TYPE_POINT &&
           sequence_node->geometry_type != GEOARROW_GEOMETRY_TYPE_LINESTRING) {
      ++sequence_node;
    }
    for (int j = 0; j < array_view.coords.n_values; j++) {
      EXPECT_EQ(sequence_node->coords[j],
                reinterpret_cast<const uint8_t*>(array_view.coords.values[j]));
      EXPECT_EQ(sequence_node->coord_stride[j],
                array_view.coords.coords_stride * static_cast<int32_t>(sizeof(double)));
    }

    auto* nodes = geom.root;
    int64_t size_nodes = geom.size_nodes;
    EXPECT_EQ(GeoArrowArrayViewGetGeometry(&array_view, -1, &geom), EINVAL);
    EXPECT_EQ(GeoArrowArrayViewGetGeometry(&array_view, array.length, &geom), EINVAL);
    EXPECT_EQ(geom.root, nodes);
    EXPECT_EQ(geom.size_nodes, size_nodes);
    EXPECT_EQ(tester.AsWKT(geom), wkt);
    EXPECT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 1, &geom), GEOARROW_OK);
    EXPECT_EQ(geom.root, nodes);
    EXPECT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 0, &geom), GEOARROW_OK);
    EXPECT_EQ(geom.root, nodes);
    GeoArrowGeometryReset(&geom);

    // Top-level slicing changes both the logical index and validity bitmap lookup.
    array.offset = 1;
    array.length = 3;
    ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);
    CheckArrayViewGeometries(&array_view, {"<null value>", empty, wkt});
    array.release(&array);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ArrayViewTest, ArrayViewGeometryTestFixture,
    ::testing::Values(
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POINT, "POINT (1 2)", "POINT EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POINT_Z, "POINT Z (1 2 3)",
                                  "POINT Z EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POINT_M, "POINT M (1 2 4)",
                                  "POINT M EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POINT_ZM, "POINT ZM (1 2 3 4)",
                                  "POINT ZM EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_LINESTRING, "LINESTRING (1 2, 3 4, 5 6)",
                                  "LINESTRING EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_LINESTRING_Z,
                                  "LINESTRING Z (1 2 3, 4 5 6)", "LINESTRING Z EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_LINESTRING_M,
                                  "LINESTRING M (1 2 3, 4 5 6)", "LINESTRING M EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_LINESTRING_ZM,
                                  "LINESTRING ZM (1 2 3 4, 5 6 7 8)",
                                  "LINESTRING ZM EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_POLYGON,
            "POLYGON ((0 0, 10 0, 0 10, 0 0), (1 1, 2 1, 1 2, 1 1))", "POLYGON EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POLYGON_Z,
                                  "POLYGON Z ((0 0 1, 10 0 2, 0 10 3, 0 0 1))",
                                  "POLYGON Z EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POLYGON_M,
                                  "POLYGON M ((0 0 1, 10 0 2, 0 10 3, 0 0 1))",
                                  "POLYGON M EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_POLYGON_ZM,
                                  "POLYGON ZM ((0 0 1 2, 10 0 3 4, 0 10 5 6, 0 0 1 2))",
                                  "POLYGON ZM EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_MULTIPOINT, "MULTIPOINT ((1 2), (3 4))",
                                  "MULTIPOINT EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_MULTIPOINT_Z,
                                  "MULTIPOINT Z ((1 2 3), (4 5 6))",
                                  "MULTIPOINT Z EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_MULTIPOINT_M,
                                  "MULTIPOINT M ((1 2 3), (4 5 6))",
                                  "MULTIPOINT M EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_MULTIPOINT_ZM,
                                  "MULTIPOINT ZM ((1 2 3 4), (5 6 7 8))",
                                  "MULTIPOINT ZM EMPTY"},
        ArrayViewGeometryTestCase{GEOARROW_TYPE_MULTILINESTRING,
                                  "MULTILINESTRING ((1 2, 3 4), EMPTY, (5 6, 7 8))",
                                  "MULTILINESTRING EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTILINESTRING_Z,
            "MULTILINESTRING Z ((1 2 3, 4 5 6), EMPTY, (7 8 9, 10 11 12))",
            "MULTILINESTRING Z EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTILINESTRING_M,
            "MULTILINESTRING M ((1 2 3, 4 5 6), EMPTY, (7 8 9, 10 11 12))",
            "MULTILINESTRING M EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTILINESTRING_ZM,
            "MULTILINESTRING ZM ((1 2 3 4, 5 6 7 8), EMPTY, (9 10 11 12, 13 14 "
            "15 16))",
            "MULTILINESTRING ZM EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTIPOLYGON,
            "MULTIPOLYGON (((0 0, 10 0, 0 10, 0 0)), EMPTY, ((2 2, 3 2, 2 3, 2 "
            "2)))",
            "MULTIPOLYGON EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTIPOLYGON_Z,
            "MULTIPOLYGON Z (((0 0 1, 10 0 2, 0 10 3, 0 0 1)), EMPTY)",
            "MULTIPOLYGON Z EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTIPOLYGON_M,
            "MULTIPOLYGON M (((0 0 1, 10 0 2, 0 10 3, 0 0 1)), EMPTY)",
            "MULTIPOLYGON M EMPTY"},
        ArrayViewGeometryTestCase{
            GEOARROW_TYPE_MULTIPOLYGON_ZM,
            "MULTIPOLYGON ZM (((0 0 1 2, 10 0 3 4, 0 10 5 6, 0 0 1 2)), EMPTY)",
            "MULTIPOLYGON ZM EMPTY"}));

TEST(ArrayViewTest, GetGeometryUnsupported) {
  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  for (auto type :
       {GEOARROW_TYPE_BOX_Z, GEOARROW_TYPE_BOX_M, GEOARROW_TYPE_BOX_ZM, GEOARROW_TYPE_WKT,
        GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKT, GEOARROW_TYPE_LARGE_WKB}) {
    struct GeoArrowArrayView array_view;
    ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
    EXPECT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 0, &geom), ENOTSUP);
    EXPECT_EQ(geom.size_nodes, 0);
  }
  GeoArrowGeometryReset(&geom);
}

TEST_P(TypeParameterizedTestFixture, ArrayViewGetGeometryEmptyBuffers) {
  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GetParam()), GEOARROW_OK);
  if (array_view.n_offsets == 0) {
    return;
  }

  struct ArrowSchema schema;
  struct ArrowArray array;
  ASSERT_EQ(GeoArrowSchemaInit(&schema, GetParam()), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendEmpty(&array, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 0, &geom), GEOARROW_OK);
  EXPECT_EQ(geom.size_nodes, 1);
  EXPECT_EQ(geom.root->size, 0);
  EXPECT_EQ(geom.root->geometry_type, array_view.schema_view.geometry_type);
  EXPECT_EQ(geom.root->dimensions, array_view.schema_view.dimensions);
  for (int j = 0; j < 4; j++) {
    double value;
    memcpy(&value, geom.root->coords[j], sizeof(value));
    EXPECT_TRUE(std::isnan(value));
  }

  array.length = 0;
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 0, &geom), EINVAL);
  GeoArrowGeometryReset(&geom);
  array.release(&array);
  schema.release(&schema);
}
