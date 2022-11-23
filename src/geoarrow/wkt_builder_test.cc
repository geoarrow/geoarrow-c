#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(WKTBuilderTest, WKTBuilderTestBasic) {
  struct GeoArrowVisitor v;
  GeoArrowWKTBuilderInit(&v);
  GeoArrowWKTBuilderReset(&v);
}

TEST(WKTBuilderTest, WKTBuilderTestPointEmpty) {
  struct GeoArrowVisitor v;
  GeoArrowWKTBuilderInit(&v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTBuilderFinish(&v, &array), GEOARROW_OK);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT EMPTY");

  array.release(&array);
  ArrowArrayViewReset(&view);
  GeoArrowWKTBuilderReset(&v);
}
