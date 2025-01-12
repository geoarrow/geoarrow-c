
#include <gtest/gtest.h>

#include <geoarrow.h>
#include "nanoarrow/nanoarrow.h"

#include "wkx_testing.hpp"

TEST(ArrayWriterTest, WritePoint) {
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
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT (1 2)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "POINT (nan nan)");

  ArrowArrayRelease(&array_out);
}
