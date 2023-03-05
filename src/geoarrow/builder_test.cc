
#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

#include "wkx_testing.hpp"

static void CustomFreeDoubleVector(uint8_t* ptr, int64_t size, void* private_data) {
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
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT ZM (2 7 12 17)");
  EXPECT_EQ(values[1], "POINT ZM (3 8 13 18)");
  EXPECT_EQ(values[2], "POINT ZM (4 9 14 19)");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestPoint) {
  struct GeoArrowBuilder builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);
  GeoArrowBuilderInitVisitor(&builder, &v);

  TestCoords coords({1}, {2});

  // Valid
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Null
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Empty
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT (1 2)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "POINT (nan nan)");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestMultipoint) {
  struct GeoArrowBuilder builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  GeoArrowBuilderInitVisitor(&builder, &v);

  TestCoords coords({1, 2, 3, 1}, {2, 3, 4, 2});

  // Valid point
  coords.view()->n_coords = 1;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid linestring
  coords.view()->n_coords = 4;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid polygon
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid multilinestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Null
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Empty
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOINT),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 6);
  EXPECT_EQ(values[0], "MULTIPOINT ((1 2))");
  EXPECT_EQ(values[1], "MULTIPOINT ((1 2), (2 3), (3 4), (1 2))");
  EXPECT_EQ(values[2], "MULTIPOINT ((1 2), (2 3), (3 4), (1 2))");
  EXPECT_EQ(values[3], "MULTIPOINT ((1 2), (2 3), (3 4), (1 2))");
  EXPECT_EQ(values[4], "<null value>");
  EXPECT_EQ(values[5], "MULTIPOINT EMPTY");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestMultiLinestring) {
  struct GeoArrowBuilder builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  GeoArrowBuilderInitVisitor(&builder, &v);

  TestCoords coords({1, 2, 3, 1}, {2, 3, 4, 2});

  // Valid linestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid polygon
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid multilinestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid multipolygon
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Null
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Empty
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 6);
  EXPECT_EQ(values[0], "MULTILINESTRING ((1 2, 2 3, 3 4, 1 2))");
  EXPECT_EQ(values[1], "MULTILINESTRING ((1 2, 2 3, 3 4, 1 2))");
  EXPECT_EQ(values[2], "MULTILINESTRING ((1 2, 2 3, 3 4, 1 2))");
  EXPECT_EQ(values[3], "MULTILINESTRING ((1 2, 2 3, 3 4, 1 2), (1 2, 2 3, 3 4, 1 2))");
  EXPECT_EQ(values[4], "<null value>");
  EXPECT_EQ(values[5], "MULTILINESTRING EMPTY");

  array_out.release(&array_out);
}

TEST(BuilderTest, BuilderTestMultiPolygon) {
  struct GeoArrowBuilder builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  GeoArrowBuilderInitVisitor(&builder, &v);

  TestCoords coords({1, 2, 3, 1}, {2, 3, 4, 2});

  // Valid linestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid polygon
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid multilinestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Valid multipolygon
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Null
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Empty
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 6);
  EXPECT_EQ(values[0], "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)))");
  EXPECT_EQ(values[1], "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)))");
  EXPECT_EQ(values[2], "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)))");
  EXPECT_EQ(values[3], "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)), ((1 2, 2 3, 3 4, 1 2)))");
  EXPECT_EQ(values[4], "<null value>");
  EXPECT_EQ(values[5], "MULTIPOLYGON EMPTY");

  array_out.release(&array_out);
}

class WKTRoundtripParameterizedTestFixture
    : public ::testing::TestWithParam<std::pair<std::string, enum GeoArrowType>> {
 protected:
  std::string wkt;
};

TEST_P(WKTRoundtripParameterizedTestFixture, BuilderTestWKTRoundtrip) {
  // Initialize params
  const std::string& wkt = GetParam().first;
  struct GeoArrowStringView wkt_view;
  wkt_view.data = wkt.data();
  wkt_view.size_bytes = wkt.size();
  enum GeoArrowType type = GetParam().second;

  // Initialize the builder
  struct GeoArrowBuilder builder;
  struct GeoArrowVisitor v;
  struct GeoArrowError error;
  v.error = &error;
  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
  GeoArrowBuilderInitVisitor(&builder, &v);

  // Visit the WKT item
  struct GeoArrowWKTReader reader;
  GeoArrowWKTReaderInit(&reader);
  ASSERT_EQ(GeoArrowWKTReaderVisit(&reader, wkt_view, &v), GEOARROW_OK);
  GeoArrowWKTReaderReset(&reader);

  // Finalize the output
  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  // Validate it
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  // Visit the output
  WKXTester tester;
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  EXPECT_EQ(values.size(), 1);
  EXPECT_EQ(values[0], wkt);

  array_out.release(&array_out);
}

#define WKT_PAIR(a, b) std::pair<std::string, enum GeoArrowType> { a, b }

INSTANTIATE_TEST_SUITE_P(
    BuilderTest, WKTRoundtripParameterizedTestFixture,
    ::testing::Values(
        // Point
        WKT_PAIR("POINT (0 1)", GEOARROW_TYPE_POINT),
        WKT_PAIR("POINT Z (0 1 2)", GEOARROW_TYPE_POINT_Z),
        WKT_PAIR("POINT M (0 1 2)", GEOARROW_TYPE_POINT_M),
        WKT_PAIR("POINT ZM (0 1 2 3)", GEOARROW_TYPE_POINT_ZM),

        // Linestring
        WKT_PAIR("LINESTRING EMPTY", GEOARROW_TYPE_LINESTRING),
        WKT_PAIR("LINESTRING (30 10, 12 16)", GEOARROW_TYPE_LINESTRING),
        WKT_PAIR("LINESTRING Z (30 10 11, 12 16 15)", GEOARROW_TYPE_LINESTRING_Z),
        WKT_PAIR("LINESTRING M (30 10 11, 12 16 15)", GEOARROW_TYPE_LINESTRING_M),
        WKT_PAIR("LINESTRING ZM (30 10 11 10, 12 16 15 98)", GEOARROW_TYPE_LINESTRING_ZM),

        // Polygon
        WKT_PAIR("POLYGON EMPTY", GEOARROW_TYPE_POLYGON),
        WKT_PAIR("POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))", GEOARROW_TYPE_POLYGON),
        WKT_PAIR("POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 "
                 "20, 20 30))",
                 GEOARROW_TYPE_POLYGON),

        // Multipoint
        WKT_PAIR("MULTIPOINT EMPTY", GEOARROW_TYPE_MULTIPOINT),
        WKT_PAIR("MULTIPOINT ((30 10), (12 16))", GEOARROW_TYPE_MULTIPOINT),

        // Multilinestring
        WKT_PAIR("MULTILINESTRING EMPTY", GEOARROW_TYPE_MULTILINESTRING),
        WKT_PAIR("MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))",
                 GEOARROW_TYPE_MULTILINESTRING),

        // Multipolygon
        WKT_PAIR("MULTIPOLYGON EMPTY", GEOARROW_TYPE_MULTIPOLYGON),
        WKT_PAIR("MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, "
                 "30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))",
                 GEOARROW_TYPE_MULTIPOLYGON)

        // Comment to keep the last line on its own
        ));

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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}
