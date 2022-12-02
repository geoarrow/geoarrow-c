#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(WKBWriterTest, WKBWriterTestBasic) {
  struct GeoArrowWKBWriter writer;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterReset(&writer);
}
