
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
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, &error), GEOARROW_OK);
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

TEST(KernelTest, KernelTestVisitVoidAggWKB) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out;

  std::basic_string<uint8_t> point({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});
  struct ArrowBufferView data;
  data.data.data = point.data();
  data.size_bytes = point.size();

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKB), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendBytes(&array_in, data), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "visit_void_agg", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "n");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, &error), GEOARROW_OK);
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

TEST(KernelTest, KernelTestVisitVoidAggGeoArow) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_POINT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendInt(array_in.children[0], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendInt(array_in.children[1], 2), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "visit_void_agg", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "n");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, &error), GEOARROW_OK);
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

TEST(KernelTest, KernelTestAsWKT) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;
  struct ArrowArray array_out2;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (30 10)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "as_wkt", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "u");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out2, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, nullptr, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  EXPECT_EQ(array_out1.length, 2);
  EXPECT_EQ(array_out1.null_count, 1);

  struct ArrowArrayView array_view;
  struct ArrowStringView item;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);
  item = ArrowArrayViewGetStringUnsafe(&array_view, 0);
  EXPECT_EQ(std::string(item.data, item.size_bytes), "POINT (30 10)");
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out2, nullptr), GEOARROW_OK);
  item = ArrowArrayViewGetStringUnsafe(&array_view, 0);
  EXPECT_EQ(std::string(item.data, item.size_bytes), "POINT (30 10)");
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  array_out2.release(&array_out2);
}

TEST(KernelTest, KernelTestAsWKB) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;
  struct ArrowArray array_out2;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (30 10)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "as_wkb", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "z");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out2, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, nullptr, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  EXPECT_EQ(array_out1.length, 2);
  EXPECT_EQ(array_out1.null_count, 1);
  EXPECT_EQ(array_out1.length, 2);
  EXPECT_EQ(array_out2.null_count, 1);

  // Will be different on big-endian
  std::basic_string<uint8_t> point({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});

  struct ArrowArrayView array_view;
  struct ArrowBufferView item;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);
  item = ArrowArrayViewGetBytesUnsafe(&array_view, 0);
  EXPECT_EQ(std::basic_string<uint8_t>(item.data.as_uint8, item.size_bytes), point);
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out2, nullptr), GEOARROW_OK);
  item = ArrowArrayViewGetBytesUnsafe(&array_view, 0);
  EXPECT_EQ(std::basic_string<uint8_t>(item.data.as_uint8, item.size_bytes), point);
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  array_out2.release(&array_out2);
}

TEST(KernelTest, KernelTestAsGeoArrow) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;
  struct ArrowArray array_out2;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (30 10)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array_in, nullptr), GEOARROW_OK);

  struct ArrowBuffer buffer;
  ASSERT_EQ(ArrowMetadataBuilderInit(&buffer, nullptr), GEOARROW_OK);
  ASSERT_EQ(
      ArrowMetadataBuilderAppend(&buffer, ArrowCharView("type"), ArrowCharView("1")),
      GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "as_geoarrow", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, reinterpret_cast<char*>(buffer.data),
                         &schema_out, &error),
            GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "+s");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out2, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, nullptr, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  EXPECT_EQ(array_out1.length, 2);
  EXPECT_EQ(array_out1.null_count, 1);
  EXPECT_EQ(array_out2.length, 2);
  EXPECT_EQ(array_out2.null_count, 1);

  struct ArrowArrayView array_view;
  struct ArrowStringView item;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[0], 0), 30);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[1], 0), 10);
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out2, nullptr), GEOARROW_OK);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[0], 0), 30);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[1], 0), 10);
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ArrowBufferReset(&buffer);
  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  array_out2.release(&array_out2);
}
