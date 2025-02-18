
#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/wkx_testing.hpp"

TEST(NativeWriterTest, WritePoint) {
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

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
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT (1 2)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "POINT (nan nan)");

  ArrowArrayRelease(&array_out);
}

TEST(NativeWriterTest, WriteInterleavedPoint) {
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_INTERLEAVED_POINT),
            GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

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
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_INTERLEAVED_POINT),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT (1 2)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "POINT (nan nan)");

  array_out.release(&array_out);
}

TEST(NativeWriterTest, WriteMultipoint) {
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

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
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOINT),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
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

TEST(NativeWriterTest, WriteInterleavedMultipoint) {
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT),
            GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

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
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(
      GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT),
      GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
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

TEST(NativeWriterTest, WriteMultiLinestring) {
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

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
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
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

TEST(NativeWriterTest, WriteMultiPolygon) {
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_MULTIPOLYGON), GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

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
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 6);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
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

TEST_P(WKTRoundtripParameterizedTestFixture, NativeWriterWKTRoundtrip) {
  // Initialize params
  const std::string& wkt = GetParam().first;
  struct GeoArrowStringView wkt_view;
  wkt_view.data = wkt.data();
  wkt_view.size_bytes = wkt.size();
  enum GeoArrowType type = GetParam().second;

  // Initialize the builder
  struct GeoArrowNativeWriter builder;
  struct GeoArrowVisitor v;
  struct GeoArrowError error;
  v.error = &error;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, type), GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&builder, &v);

  // Visit the WKT item
  struct GeoArrowWKTReader reader;
  GeoArrowWKTReaderInit(&reader);
  ASSERT_EQ(GeoArrowWKTReaderVisit(&reader, wkt_view, &v), GEOARROW_OK);
  GeoArrowWKTReaderReset(&reader);

  // Finalize the output
  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  // Validate it
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  // Visit the output
  WKXTester tester;
  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 1);
  EXPECT_EQ(values[0], wkt);

  array_out.release(&array_out);
}

#define WKT_PAIR(a, b) std::pair<std::string, enum GeoArrowType> { a, b }

INSTANTIATE_TEST_SUITE_P(
    NativeWriterTest, WKTRoundtripParameterizedTestFixture,
    ::testing::Values(
        // Point
        WKT_PAIR("POINT (0 1)", GEOARROW_TYPE_POINT),
        WKT_PAIR("POINT Z (0 1 2)", GEOARROW_TYPE_POINT_Z),
        WKT_PAIR("POINT M (0 1 2)", GEOARROW_TYPE_POINT_M),
        WKT_PAIR("POINT ZM (0 1 2 3)", GEOARROW_TYPE_POINT_ZM),

        // Interleaved Point
        WKT_PAIR("POINT (0 1)", GEOARROW_TYPE_INTERLEAVED_POINT),
        WKT_PAIR("POINT Z (0 1 2)", GEOARROW_TYPE_INTERLEAVED_POINT_Z),
        WKT_PAIR("POINT M (0 1 2)", GEOARROW_TYPE_INTERLEAVED_POINT_M),
        WKT_PAIR("POINT ZM (0 1 2 3)", GEOARROW_TYPE_INTERLEAVED_POINT_ZM),

        // Linestring
        WKT_PAIR("LINESTRING EMPTY", GEOARROW_TYPE_LINESTRING),
        WKT_PAIR("LINESTRING (30 10, 12 16)", GEOARROW_TYPE_LINESTRING),
        WKT_PAIR("LINESTRING Z (30 10 11, 12 16 15)", GEOARROW_TYPE_LINESTRING_Z),
        WKT_PAIR("LINESTRING M (30 10 11, 12 16 15)", GEOARROW_TYPE_LINESTRING_M),
        WKT_PAIR("LINESTRING ZM (30 10 11 10, 12 16 15 98)", GEOARROW_TYPE_LINESTRING_ZM),

        // Interleaved Linestring
        WKT_PAIR("LINESTRING EMPTY", GEOARROW_TYPE_INTERLEAVED_LINESTRING),
        WKT_PAIR("LINESTRING (30 10, 12 16)", GEOARROW_TYPE_INTERLEAVED_LINESTRING),
        WKT_PAIR("LINESTRING Z (30 10 11, 12 16 15)",
                 GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z),
        WKT_PAIR("LINESTRING M (30 10 11, 12 16 15)",
                 GEOARROW_TYPE_INTERLEAVED_LINESTRING_M),
        WKT_PAIR("LINESTRING ZM (30 10 11 10, 12 16 15 98)",
                 GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM),

        // Polygon
        WKT_PAIR("POLYGON EMPTY", GEOARROW_TYPE_POLYGON),
        WKT_PAIR("POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))", GEOARROW_TYPE_POLYGON),
        WKT_PAIR("POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 "
                 "20, 20 30))",
                 GEOARROW_TYPE_POLYGON),

        // Interleaved Polygon
        WKT_PAIR("POLYGON EMPTY", GEOARROW_TYPE_INTERLEAVED_POLYGON),
        WKT_PAIR("POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))",
                 GEOARROW_TYPE_INTERLEAVED_POLYGON),
        WKT_PAIR("POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 "
                 "20, 20 30))",
                 GEOARROW_TYPE_INTERLEAVED_POLYGON),

        // Multipoint
        WKT_PAIR("MULTIPOINT EMPTY", GEOARROW_TYPE_MULTIPOINT),
        WKT_PAIR("MULTIPOINT ((30 10), (12 16))", GEOARROW_TYPE_MULTIPOINT),

        // Interleaved Multipoint
        WKT_PAIR("MULTIPOINT EMPTY", GEOARROW_TYPE_INTERLEAVED_MULTIPOINT),
        WKT_PAIR("MULTIPOINT ((30 10), (12 16))", GEOARROW_TYPE_INTERLEAVED_MULTIPOINT),

        // Multilinestring
        WKT_PAIR("MULTILINESTRING EMPTY", GEOARROW_TYPE_MULTILINESTRING),
        WKT_PAIR("MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))",
                 GEOARROW_TYPE_MULTILINESTRING),

        // Interleaved Multilinestring
        WKT_PAIR("MULTILINESTRING EMPTY", GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING),
        WKT_PAIR("MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))",
                 GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING),

        // Multipolygon
        WKT_PAIR("MULTIPOLYGON EMPTY", GEOARROW_TYPE_MULTIPOLYGON),
        WKT_PAIR("MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, "
                 "30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))",
                 GEOARROW_TYPE_MULTIPOLYGON),

        // Interleaved Multipolygon
        WKT_PAIR("MULTIPOLYGON EMPTY", GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON),
        WKT_PAIR("MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, "
                 "30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))",
                 GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON)

        // Comment to keep the last line on its own
        ));
