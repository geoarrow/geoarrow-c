
#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(ArrayTest, ArrayTestInit) {
  struct GeoArrowArray array;
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowArrayInitFromType(&array, GEOARROW_TYPE_POINT), GEOARROW_OK);
  EXPECT_EQ(array.schema_view.type, GEOARROW_TYPE_POINT);
  GeoArrowArrayReset(&array);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  GeoArrowArrayReset(&array);
  schema.release(&schema);
}

TEST(ArrayTest, ArrayTestEmpty) {
  struct GeoArrowArray array;
  struct ArrowArray array_out;
  array_out.release = nullptr;

  EXPECT_EQ(GeoArrowArrayInitFromType(&array, GEOARROW_TYPE_POINT), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayFinish(&array, &array_out, nullptr), GEOARROW_OK);
  EXPECT_NE(array_out.release, nullptr);
  EXPECT_EQ(array.array.release, nullptr);

  GeoArrowArrayReset(&array);
  array_out.release(&array_out);
}
