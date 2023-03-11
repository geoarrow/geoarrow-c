
#include <errno.h>
#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(KernelTest, KernelTestVoid) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out;

  ASSERT_EQ(ArrowSchemaInitFromType(&schema_in, NANOARROW_TYPE_NA), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  array_in.length = 123;
  array_in.null_count = 123;

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "void", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "n");
  schema_out.release(&schema_out);

  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out, &error), GEOARROW_OK);
  EXPECT_EQ(array_out.length, 123);
  EXPECT_EQ(array_out.null_count, 123);
  array_out.release(&array_out);

  EXPECT_EQ(kernel.finish(&kernel, &array_out, &error), EINVAL);
  EXPECT_EQ(kernel.finish(&kernel, nullptr, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  schema_in.release(&schema_in);
  array_in.release(&array_in);
}

TEST(KernelTest, KernelTestVoidAgg) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out;

  ASSERT_EQ(ArrowSchemaInitFromType(&schema_in, NANOARROW_TYPE_NA), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  array_in.length = 123;
  array_in.null_count = 123;

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "void_agg", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "n");
  schema_out.release(&schema_out);

  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out, &error), EINVAL);
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, &error), GEOARROW_OK);

  EXPECT_EQ(kernel.finish(&kernel, &array_out, &error), GEOARROW_OK);
  EXPECT_EQ(array_out.length, 1);
  EXPECT_EQ(array_out.null_count, 1);
  array_out.release(&array_out);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  schema_in.release(&schema_in);
  array_in.release(&array_in);
}

TEST(KernelTest, KernelTestVisitVoidAggWKT) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (0 1)")), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (2 3)")), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (4 5)")), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "visit_void_agg", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "n");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, &array_out, &error), GEOARROW_OK);

  EXPECT_EQ(array_out.length, 1);
  EXPECT_EQ(array_out.null_count, 1);
  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out.release(&array_out);
}
