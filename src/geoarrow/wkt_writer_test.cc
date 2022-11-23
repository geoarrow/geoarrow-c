#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(WKTWriterTest, WKTWriterTestBasic) {
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&v);
  GeoArrowWKTWriterReset(&v);
}

TEST(WKTWriterTest, WKTWriterTestPointEmpty) {
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&v, &array), GEOARROW_OK);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT EMPTY");

  array.release(&array);
  ArrowArrayViewReset(&view);
  GeoArrowWKTWriterReset(&v);
}
