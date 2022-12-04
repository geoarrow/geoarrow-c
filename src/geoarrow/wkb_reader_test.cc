#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(WBTReaderTest, WKBReaderTestBasic) {
  struct GeoArrowWKBReader reader;
  GeoArrowWKBReaderInit(&reader);
  GeoArrowWKBReaderReset(&reader);
}
