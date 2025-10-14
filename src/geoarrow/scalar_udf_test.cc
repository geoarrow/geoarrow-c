#include <errno.h>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.hpp"

TEST(ScalarUdfTest, InitError) {
  struct GeoArrowScalarUdfFactory factory;
  struct GeoArrowError error;
  ASSERT_EQ(GeoArrowScalarUdfFactoryInit(&factory, "does not exist", nullptr, &error),
            ENOTSUP);
  ASSERT_STREQ(error.message, "GeoArrow C scalar implementation with name 'does not exist' does not exist");
}

TEST(ScalarUdfTest, Void) {
  struct GeoArrowScalarUdfFactory factory;
  struct GeoArrowScalarUdf udf;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowScalarUdfFactoryInit(&factory, "void", nullptr, &error), GEOARROW_OK);
  factory.new_scalar_udf_impl(&factory, &udf);

  struct ArrowSchema schema_out;
  struct ArrowArray array_out;

  // Calculate return type
  EXPECT_EQ(udf.init(&udf, nullptr, nullptr, 0, &schema_out), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "n");
  ArrowSchemaRelease(&schema_out);

  // Execute a batch
  EXPECT_EQ(udf.execute(&udf, nullptr, 0, 123, &array_out), GEOARROW_OK);
  EXPECT_EQ(array_out.length, 123);
  EXPECT_EQ(array_out.null_count, 123);
  ArrowArrayRelease(&array_out);

  udf.release(&udf);
  factory.release(&factory);
}
