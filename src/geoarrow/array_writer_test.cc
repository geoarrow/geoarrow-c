

#include <gtest/gtest.h>

#include "geoarrow.h"

#include "wkx_testing.hpp"

TEST(ArrayWriterTest, ArrayWriterTestBasic) {
  struct GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);
  GeoArrowArrayWriterReset(&writer);
}
