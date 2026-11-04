
#include <cmath>

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
  EXPECT_EQ(values[2], "POINT EMPTY");

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
  EXPECT_EQ(values[2], "POINT EMPTY");

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

TEST_P(WKTRoundtripParameterizedTestFixture, NativeWriterWKTRoundtripGeometry) {
  // Initialize params
  const std::string& wkt = GetParam().first;
  enum GeoArrowType type = GetParam().second;

  // Initialize the builder
  struct GeoArrowNativeWriter builder;
  struct GeoArrowError error;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, type), GEOARROW_OK);

  WKXTester tester;
  auto geom = tester.AsGeometry(wkt);
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // Finalize the output
  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  // Validate it
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  // Visit the output
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

TEST(NativeWriterTest, WritePointGeometry) {
  struct GeoArrowError error;
  WKXTester tester;

  struct GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);

  // A POINT
  auto geom = tester.AsGeometry("POINT (1 2)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A null
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&builder), GEOARROW_OK);

  // An EMPTY of the same type
  geom = tester.AsGeometry("POINT EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // An EMPTY of a different type
  geom = tester.AsGeometry("GEOMETRYCOLLECTION EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A MULTIPOINT with exactly one child
  geom = tester.AsGeometry("MULTIPOINT ((3 4))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // ...but not a MULTIPOINT with >1 child
  geom = tester.AsGeometry("MULTIPOINT ((5 6), (7 8))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message,
               "Can't append geometry with coord count >1 to array of type POINT");

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 5);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 5);
  EXPECT_EQ(values[0], "POINT (1 2)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "POINT EMPTY");
  EXPECT_EQ(values[3], "POINT EMPTY");
  EXPECT_EQ(values[4], "POINT (3 4)");

  ArrowArrayRelease(&array_out);
}

TEST(NativeWriterTest, WriteLinestringGeometry) {
  struct GeoArrowError error;
  WKXTester tester;

  struct GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_LINESTRING), GEOARROW_OK);

  // A LINESTRING
  auto geom = tester.AsGeometry("LINESTRING (1 2, 3 4)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A null
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&builder), GEOARROW_OK);

  // An empty of the same type
  geom = tester.AsGeometry("LINESTRING EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // An empty of a different type
  geom = tester.AsGeometry("GEOMETRYCOLLECTION EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A MULTILINESTRING with 1 child
  geom = tester.AsGeometry("MULTILINESTRING ((5 6, 7 8))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // ...but not a MULTILINESTRING with >1 child
  geom = tester.AsGeometry("MULTILINESTRING ((5 6, 7 8), (9 10, 11 12))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message,
               "Can't append MULTILINESTRING with size >1 to array of type LINESTRING");

  // ...or something unrelated
  geom = tester.AsGeometry("POINT (7 8)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message, "Can't append POINT to array of type LINESTRING");

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, &error), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 5);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_LINESTRING),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 5);
  EXPECT_EQ(values[0], "LINESTRING (1 2, 3 4)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "LINESTRING EMPTY");
  EXPECT_EQ(values[3], "LINESTRING EMPTY");
  EXPECT_EQ(values[4], "LINESTRING (5 6, 7 8)");

  ArrowArrayRelease(&array_out);
}

TEST(NativeWriterTest, WritePolygonGeometry) {
  struct GeoArrowError error;
  WKXTester tester;

  struct GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_POLYGON), GEOARROW_OK);

  // A POLYGON
  auto geom = tester.AsGeometry("POLYGON ((0 0, 10 0, 0 10, 0 0), (1 1, 1 5, 5 1, 1 1))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A null
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&builder), GEOARROW_OK);

  // An EMPTY
  geom = tester.AsGeometry("POLYGON EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // An EMPTY of a different type
  geom = tester.AsGeometry("GEOMETRYCOLLECTION EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A MULTIPOLYGON with 1 child
  geom = tester.AsGeometry("MULTIPOLYGON (((0 0, 0 -10, -10 0, 0 0)))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // ...but not a MULTIPOLYGON with >1 child
  geom = tester.AsGeometry(
      "MULTIPOLYGON (((0 0, 0 -10, -10 0, 0 0)), ((0 0, 1 0, 0 1, 0 0)))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message,
               "Can't append MULTIPOLYGON with size >1 to array of type POLYGON");

  // ...or something unrelated
  geom = tester.AsGeometry("LINESTRING (7 8, 9 10)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message, "Can't append LINESTRING to array of type POLYGON");

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 5);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POLYGON),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 5);
  EXPECT_EQ(values[0], "POLYGON ((0 0, 10 0, 0 10, 0 0), (1 1, 1 5, 5 1, 1 1))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "POLYGON EMPTY");
  EXPECT_EQ(values[3], "POLYGON EMPTY");
  EXPECT_EQ(values[4], "POLYGON ((0 0, 0 -10, -10 0, 0 0))");

  ArrowArrayRelease(&array_out);
}

TEST(NativeWriterTest, WriteMultiPointGeometry) {
  struct GeoArrowError error;
  WKXTester tester;

  struct GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);

  // A MULTIPOINT
  auto geom = tester.AsGeometry("MULTIPOINT ((1 2), (3 4))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A null
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&builder), GEOARROW_OK);

  // An EMPTY
  geom = tester.AsGeometry("MULTIPOINT EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // An EMPTY of another type
  geom = tester.AsGeometry("GEOMETRYCOLLECTION EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A POINT
  geom = tester.AsGeometry("POINT (5 6)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // ...but not something unrelated
  geom = tester.AsGeometry("LINESTRING (7 8, 9 10)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message, "Can't append LINESTRING to array of type MULTIPOINT");

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 5);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOINT),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 5);
  EXPECT_EQ(values[0], "MULTIPOINT ((1 2), (3 4))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "MULTIPOINT EMPTY");
  EXPECT_EQ(values[3], "MULTIPOINT EMPTY");
  EXPECT_EQ(values[4], "MULTIPOINT ((5 6))");

  ArrowArrayRelease(&array_out);
}

TEST(NativeWriterTest, WriteMultiLinestringGeometry) {
  struct GeoArrowError error;
  WKXTester tester;

  struct GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);

  // A geometry
  auto geom = tester.AsGeometry("MULTILINESTRING ((1 2, 3 4), (5 6, 7 8))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A null
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&builder), GEOARROW_OK);

  // An EMPTY
  geom = tester.AsGeometry("MULTILINESTRING EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // An EMPTY of another type
  geom = tester.AsGeometry("GEOMETRYCOLLECTION EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A LINESTRING
  geom = tester.AsGeometry("LINESTRING (9 10, 11 12)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // ...but not something unrelated
  geom = tester.AsGeometry("POINT (7 8)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message, "Can't append POINT to array of type MULTILINESTRING");

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 5);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 5);
  EXPECT_EQ(values[0], "MULTILINESTRING ((1 2, 3 4), (5 6, 7 8))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "MULTILINESTRING EMPTY");
  EXPECT_EQ(values[3], "MULTILINESTRING EMPTY");
  EXPECT_EQ(values[4], "MULTILINESTRING ((9 10, 11 12))");

  ArrowArrayRelease(&array_out);
}

TEST(NativeWriterTest, WriteMultiPolygonGeometry) {
  struct GeoArrowError error;
  WKXTester tester;

  struct GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_MULTIPOLYGON), GEOARROW_OK);

  // A geometry
  auto geom = tester.AsGeometry(
      "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, "
      "20 35), (30 20, 20 15, 20 25, 30 20)))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A null
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&builder), GEOARROW_OK);

  // An EMPTY
  geom = tester.AsGeometry("MULTIPOLYGON EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // An EMPTY of another type
  geom = tester.AsGeometry("GEOMETRYCOLLECTION EMPTY");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // A POLYGON
  geom = tester.AsGeometry("POLYGON ((0 0, 1 0, 0 1, 0 0))");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            GEOARROW_OK);

  // ...but not something unrelated
  geom = tester.AsGeometry("POINT (7 8)");
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), &error),
            EINVAL);
  ASSERT_STREQ(error.message, "Can't append POINT to array of type MULTIPOLYGON");

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&builder, &array_out, &error), GEOARROW_OK);
  GeoArrowNativeWriterReset(&builder);

  EXPECT_EQ(array_out.length, 5);
  EXPECT_EQ(array_out.null_count, 1);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  EXPECT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 5);
  EXPECT_EQ(values[0],
            "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, "
            "45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "MULTIPOLYGON EMPTY");
  EXPECT_EQ(values[3], "MULTIPOLYGON EMPTY");
  EXPECT_EQ(values[4], "MULTIPOLYGON (((0 0, 1 0, 0 1, 0 0)))");

  ArrowArrayRelease(&array_out);
}

class NativeAppendParameterizedTest : public ::testing::TestWithParam<GeoArrowType> {};

static std::vector<GeoArrowType> NativeAppendTypes() {
  std::vector<GeoArrowType> types;
  for (int type = 1; type <= 6; type++) {
    for (int dims = 1; dims <= 4; dims++) {
      for (auto layout :
           {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
        types.push_back(GeoArrowMakeType(static_cast<GeoArrowGeometryType>(type),
                                         static_cast<GeoArrowDimensions>(dims), layout));
      }
    }
  }
  return types;
}

INSTANTIATE_TEST_SUITE_P(NativeWriterTest, NativeAppendParameterizedTest,
                         ::testing::ValuesIn(NativeAppendTypes()));

static std::string NativeAppendWKT(GeoArrowGeometryType type, GeoArrowDimensions dims) {
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
  return prefix + " " + contents[type];
}

TEST_P(NativeAppendParameterizedTest, AppendGeometry) {
  GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GetParam()), GEOARROW_OK);
  GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaViewInitFromType(&schema_view, GetParam()), GEOARROW_OK);
  auto type = schema_view.geometry_type;
  auto dims = schema_view.dimensions;
  std::string wkt = NativeAppendWKT(type, dims);
  std::string empty = wkt.substr(0, wkt.find(" (")) + " EMPTY";
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
      ASSERT_EQ((is_null ? GeoArrowNativeWriterAppendNull(&builder)
                         : GeoArrowNativeWriterAppend(
                               &builder, GeoArrowGeometryAsView(&geom), &error)),
                GEOARROW_OK)
          << error.message;
    }
    tester.AsGeometry("POINT (9 10)");
    ArrowArray array;
    ASSERT_EQ(GeoArrowNativeWriterFinish(&builder, &array, &error), GEOARROW_OK)
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
  GeoArrowNativeWriterReset(&builder);
}

TEST(NativeWriterTest, AppendGeometryPreservesEmptyParts) {
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
    GeoArrowNativeWriter builder;
    ASSERT_EQ(GeoArrowNativeWriterInit(&builder, type), GEOARROW_OK);
    std::vector<GeoArrowGeometryNode> nodes(geom.root, geom.root + geom.size_nodes);
    for (int i = 0; i < 2; i++) {
      ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, geom, nullptr), GEOARROW_OK);
      for (auto& node : nodes) {
        if (node.geometry_type == GEOARROW_GEOMETRY_TYPE_POINT) {
          double x;
          memcpy(&x, node.coords[0], sizeof(x));
          if (std::isnan(x)) node.size = 0;
        }
      }
      geom.root = nodes.data();
    }
    ArrowArray array;
    ASSERT_EQ(GeoArrowNativeWriterFinish(&builder, &array, nullptr), GEOARROW_OK);
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
    GeoArrowNativeWriterReset(&builder);
    GeoArrowWKBReaderReset(&reader);
  }
}

TEST(NativeWriterTest, AppendGeometryStridesDimensionsAndEndian) {
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
      for (auto& coord : node.coords) coord = _GeoArrowkEmptyPointCoords;
      node.geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
      node.dimensions = GEOARROW_DIMENSIONS_XY;
      node.flags =
          static_cast<uint8_t>(swap ? GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN : 0);
      node.size = 2;
      node.coords[0] = bytes + 9;
      node.coords[1] = bytes + 17;
      node.coord_stride[0] = -8;
      GeoArrowNativeWriter builder;
      auto type = GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                   GEOARROW_DIMENSIONS_XYZM, layout);
      ASSERT_EQ(GeoArrowNativeWriterInit(&builder, type), GEOARROW_OK);
      ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, {&node, 1}, nullptr), GEOARROW_OK);
      ArrowArray array;
      ASSERT_EQ(GeoArrowNativeWriterFinish(&builder, &array, nullptr), GEOARROW_OK);
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
      GeoArrowNativeWriterReset(&builder);
    }
  }

  WKXTester tester;
  const auto& geom = tester.AsGeometry("POINT ZM (1 2 3 4)");
  GeoArrowNativeWriter builder;
  ASSERT_EQ(GeoArrowNativeWriterInit(&builder, GEOARROW_TYPE_POINT_M), GEOARROW_OK);
  ASSERT_EQ(GeoArrowNativeWriterAppend(&builder, GeoArrowGeometryAsView(&geom), nullptr),
            GEOARROW_OK);
  ArrowArray array;
  ASSERT_EQ(GeoArrowNativeWriterFinish(&builder, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(static_cast<const double*>(array.children[2]->buffers[1])[0], 4);
  array.release(&array);
  GeoArrowNativeWriterReset(&builder);
}

TEST(NativeWriterTest, AppendGeometryInvalidAndOverflow) {
  GeoArrowNativeWriter writer;
  ASSERT_EQ(GeoArrowNativeWriterInit(&writer, GEOARROW_TYPE_MULTIPOLYGON), GEOARROW_OK);
  WKXTester tester;
  const auto& geom = tester.AsGeometry("MULTIPOLYGON (((1 2, 3 4)))");
  std::vector<GeoArrowGeometryNode> nodes(geom.root, geom.root + geom.size_nodes);
  ASSERT_EQ(GeoArrowNativeWriterAppend(&writer, GeoArrowGeometryAsView(&geom), nullptr),
            GEOARROW_OK);
  auto reject = [&](GeoArrowGeometryView view, int code) {
    EXPECT_EQ(GeoArrowNativeWriterAppend(&writer, view, nullptr), code);
  };
  reject({nullptr, 1}, EINVAL);
  reject({nodes.data(), -1}, EINVAL);
  reject({nodes.data(), 0}, EINVAL);
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
  nodes[2].dimensions = GEOARROW_DIMENSIONS_XYZM + 1;
  reject({nodes.data(), 3}, EINVAL);
  nodes[2].dimensions = GEOARROW_DIMENSIONS_XY;
  nodes[2].size = UINT32_MAX;
  reject({nodes.data(), 3}, EOVERFLOW);
  nodes[2].size = INT32_MAX;  // Individually valid, but overflows the existing offset.
  reject({nodes.data(), 3}, EOVERFLOW);

  // Rejected appends leave exactly the preceding feature, and allow another one.
  ASSERT_EQ(GeoArrowNativeWriterAppend(&writer, GeoArrowGeometryAsView(&geom), nullptr),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&writer), GEOARROW_OK);
  ArrowArray array;
  ASSERT_EQ(GeoArrowNativeWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 3);
  EXPECT_EQ(array.null_count, 1);
  GeoArrowArrayView view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&view, GEOARROW_TYPE_MULTIPOLYGON),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);
  GeoArrowGeometry out;
  ASSERT_EQ(GeoArrowGeometryInit(&out), GEOARROW_OK);
  for (int i = 0; i < 2; i++) {
    ASSERT_EQ(GeoArrowArrayViewGetGeometry(&view, i, &out), GEOARROW_OK);
    EXPECT_EQ(tester.AsWKT(out), "MULTIPOLYGON (((1 2, 3 4)))");
  }
  EXPECT_EQ(view.coords.n_coords, 4);
  GeoArrowGeometryReset(&out);
  array.release(&array);
  GeoArrowNativeWriterReset(&writer);

  ASSERT_EQ(GeoArrowNativeWriterInit(&writer, GEOARROW_TYPE_POINT), GEOARROW_OK);
  nodes[0].geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  nodes[0].size = 2;
  EXPECT_EQ(GeoArrowNativeWriterAppend(&writer, {nodes.data(), 1}, nullptr), EINVAL);
  GeoArrowNativeWriterReset(&writer);
}

TEST(NativeWriterTest, AppendGeometryCoordinateCountDoesNotWrap) {
  GeoArrowNativeWriter writer;
  ASSERT_EQ(GeoArrowNativeWriterInit(&writer, GEOARROW_TYPE_MULTILINESTRING),
            GEOARROW_OK);
  GeoArrowGeometryNode nodes[3] = {};
  for (auto& node : nodes) {
    node.dimensions = GEOARROW_DIMENSIONS_XY;
    for (auto& coord : node.coords) coord = _GeoArrowkEmptyPointCoords;
  }
  nodes[0].geometry_type = GEOARROW_GEOMETRY_TYPE_MULTILINESTRING;
  nodes[0].size = 2;
  for (int i = 1; i < 3; i++) {
    nodes[i].geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
    nodes[i].level = 1;
    nodes[i].size = uint32_t{1} << 31;
  }
  // The old uint32_t sum wrapped to zero, silently writing an empty feature.
  EXPECT_EQ(GeoArrowNativeWriterAppend(&writer, {nodes, 3}, nullptr), EOVERFLOW);
  ArrowArray array;
  ASSERT_EQ(GeoArrowNativeWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 0);
  array.release(&array);
  GeoArrowNativeWriterReset(&writer);
}

TEST(NativeWriterTest, AppendGeometryEmptyConversions) {
  WKXTester tester;
  struct Case {
    const char* input;
    GeoArrowType type;
    const char* expected;
  };
  for (const auto& item :
       {Case{"POINT EMPTY", GEOARROW_TYPE_MULTIPOINT, "MULTIPOINT (EMPTY)"},
        Case{"LINESTRING EMPTY", GEOARROW_TYPE_MULTILINESTRING,
             "MULTILINESTRING (EMPTY)"},
        Case{"POLYGON (EMPTY)", GEOARROW_TYPE_MULTIPOLYGON, "MULTIPOLYGON ((EMPTY))"},
        Case{"MULTIPOLYGON ((EMPTY))", GEOARROW_TYPE_POLYGON, "POLYGON (EMPTY)"},
        Case{"MULTILINESTRING (EMPTY)", GEOARROW_TYPE_LINESTRING, "LINESTRING EMPTY"},
        Case{"GEOMETRYCOLLECTION EMPTY", GEOARROW_TYPE_POINT, "POINT EMPTY"},
        Case{"MULTILINESTRING (EMPTY, EMPTY)", GEOARROW_TYPE_LINESTRING,
             "LINESTRING EMPTY"},
        Case{"GEOMETRYCOLLECTION (POINT (1 2))", GEOARROW_TYPE_POINT, "POINT (1 2)"}}) {
    SCOPED_TRACE(item.input);
    auto wkb = tester.AsWKB(item.input);
    GeoArrowWKBReader reader;
    ASSERT_EQ(GeoArrowWKBReaderInit(&reader), GEOARROW_OK);
    GeoArrowGeometryView geom;
    ASSERT_EQ(
        GeoArrowWKBReaderRead(&reader, {wkb.data(), static_cast<int64_t>(wkb.size())},
                              &geom, nullptr),
        GEOARROW_OK);
    GeoArrowNativeWriter writer;
    ASSERT_EQ(GeoArrowNativeWriterInit(&writer, item.type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowNativeWriterAppend(&writer, geom, nullptr), GEOARROW_OK);
    ArrowArray array;
    ASSERT_EQ(GeoArrowNativeWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
    GeoArrowArrayView view;
    ASSERT_EQ(GeoArrowArrayViewInitFromType(&view, item.type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);
    GeoArrowGeometry out;
    ASSERT_EQ(GeoArrowGeometryInit(&out), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayViewGetGeometry(&view, 0, &out), GEOARROW_OK);
    EXPECT_EQ(tester.AsWKT(out), item.expected);
    GeoArrowGeometryReset(&out);
    array.release(&array);
    GeoArrowNativeWriterReset(&writer);
    GeoArrowWKBReaderReset(&reader);
  }
}

TEST_P(NativeAppendParameterizedTest, AppendGeometryAfterVisitor) {
  GeoArrowNativeWriter writer;
  ASSERT_EQ(GeoArrowNativeWriterInit(&writer, GetParam()), GEOARROW_OK);
  GeoArrowVisitor visitor = {};
  ASSERT_EQ(GeoArrowNativeWriterInitVisitor(&writer, &visitor), GEOARROW_OK);
  GeoArrowSchemaView schema;
  ASSERT_EQ(GeoArrowSchemaViewInitFromType(&schema, GetParam()), GEOARROW_OK);
  WKXTester tester;
  std::string wkt = NativeAppendWKT(schema.geometry_type, schema.dimensions);
  const auto& geom = tester.AsGeometry(wkt);
  // In particular, appending a null after a visitor feature must backfill validity
  // using the actual array length, not a counter tracking only direct appends.
  ASSERT_EQ(GeoArrowGeometryViewVisit(GeoArrowGeometryAsView(&geom), &visitor),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowNativeWriterAppendNull(&writer), GEOARROW_OK);
  ASSERT_EQ(GeoArrowNativeWriterAppend(&writer, GeoArrowGeometryAsView(&geom), nullptr),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowGeometryViewVisit(GeoArrowGeometryAsView(&geom), &visitor),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowNativeWriterAppend(&writer, GeoArrowGeometryAsView(&geom), nullptr),
            GEOARROW_OK);
  ArrowArray array;
  ASSERT_EQ(GeoArrowNativeWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  ASSERT_EQ(array.length, 5);
  ASSERT_EQ(array.null_count, 1);
  for (int i = 0; i < 5; i++)
    EXPECT_EQ(ArrowBitGet(static_cast<const uint8_t*>(array.buffers[0]), i), i != 1);
  GeoArrowArrayView view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&view, GetParam()), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);
  GeoArrowGeometry out;
  ASSERT_EQ(GeoArrowGeometryInit(&out), GEOARROW_OK);
  for (int i : {0, 2, 3, 4}) {
    ASSERT_EQ(GeoArrowArrayViewGetGeometry(&view, i, &out), GEOARROW_OK);
    EXPECT_EQ(tester.AsWKT(out), wkt);
  }
  GeoArrowGeometryReset(&out);
  array.release(&array);
  GeoArrowNativeWriterReset(&writer);
}

TEST(NativeWriterTest, AppendGeometryUnsupportedOutput) {
  for (auto type : {GEOARROW_TYPE_BOX, GEOARROW_TYPE_WKB, GEOARROW_TYPE_WKT}) {
    GeoArrowNativeWriter writer;
    ASSERT_EQ(GeoArrowNativeWriterInit(&writer, type), GEOARROW_OK);
    WKXTester tester;
    const auto& geom = tester.AsGeometry("POINT (1 2)");
    EXPECT_EQ(GeoArrowNativeWriterAppend(&writer, GeoArrowGeometryAsView(&geom), nullptr),
              ENOTSUP);
    EXPECT_EQ(GeoArrowNativeWriterAppendNull(&writer), ENOTSUP);
    GeoArrowNativeWriterReset(&writer);
  }
}
