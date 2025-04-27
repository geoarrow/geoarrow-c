
#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

TEST(KernelTest, KernelTestVoid) {
  struct GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);

  GeoArrowGeometryReset(&geom);
}
