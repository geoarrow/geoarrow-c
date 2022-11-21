#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(WKTBuilderTest, WKTBuilderTestBasic) {
  struct GeoArrowVisitor v;
  GeoArrowWKTBuilderInit(&v);
  GeoArrowWKTBuilderReset(&v);
}