
#include <gtest/gtest.h>

#include "geoarrow.h"

#include "wkx_testing.hpp"

TEST(ArrayReaderTest, ArrayReaderTestBasic) {
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInit(&reader), GEOARROW_OK);
  GeoArrowArrayReaderReset(&reader);
}
