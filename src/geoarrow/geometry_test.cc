
#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

TEST(KernelTest, KernelTestVoid) {
  struct GeoArrowOwningGeometry geom;
  ASSERT_EQ(GeoArrowOwningGeometryInit(&geom), GEOARROW_OK);

  GeoArrowOwningGeometryReset(&geom);
}
