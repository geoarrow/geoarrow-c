
#include <errno.h>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

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
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

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
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

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
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

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
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "as_geoarrow", nullptr), GEOARROW_OK);

  struct ArrowBuffer buffer;
  ASSERT_EQ(ArrowMetadataBuilderInit(&buffer, nullptr), GEOARROW_OK);
  ASSERT_EQ(
      ArrowMetadataBuilderAppend(&buffer, ArrowCharView("type"), ArrowCharView("100003")),
      GEOARROW_OK);

  EXPECT_EQ(kernel.start(&kernel, &schema_in, reinterpret_cast<char*>(buffer.data),
                         &schema_out, &error),
            GEOARROW_OK);
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

  ArrowBufferReset(&buffer);
  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  array_out2.release(&array_out2);
}

TEST(KernelTest, KernelTestFormatWKTFromWKT) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;

  // Use at least one LINESTRING, which forces a slightly different path through the
  // visitor.
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING (30.1234 10, 0 0)")),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (31.1234 11)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  struct ArrowBuffer buffer;
  ASSERT_EQ(ArrowMetadataBuilderInit(&buffer, nullptr), GEOARROW_OK);
  ASSERT_EQ(
      ArrowMetadataBuilderAppend(&buffer, ArrowCharView("precision"), ArrowCharView("3")),
      GEOARROW_OK);
  ASSERT_EQ(ArrowMetadataBuilderAppend(&buffer, ArrowCharView("max_element_size_bytes"),
                                       ArrowCharView("16")),
            GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "format_wkt", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, (char*)buffer.data, &schema_out, &error),
            GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "u");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, nullptr, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  EXPECT_EQ(array_out1.length, 3);
  EXPECT_EQ(array_out1.null_count, 1);

  struct ArrowArrayView array_view;
  struct ArrowStringView item;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);
  item = ArrowArrayViewGetStringUnsafe(&array_view, 0);
  EXPECT_EQ(std::string(item.data, item.size_bytes), "LINESTRING (30.1");
  item = ArrowArrayViewGetStringUnsafe(&array_view, 1);
  EXPECT_EQ(std::string(item.data, item.size_bytes), "POINT (31.123 11");
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 2));

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  ArrowBufferReset(&buffer);
}

TEST(KernelTest, KernelTestFormatWKTFromWKB) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;

  // Use at least one LINESTRING, which forces a slightly different path through the
  // visitor.
  std::basic_string<uint8_t> linestring(
      {0x01, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x3e, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  struct ArrowBufferView data;
  data.data.data = linestring.data();
  data.size_bytes = linestring.size();

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKB), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendBytes(&array_in, data), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  struct ArrowBuffer buffer;
  ASSERT_EQ(ArrowMetadataBuilderInit(&buffer, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowMetadataBuilderAppend(&buffer, ArrowCharView("max_element_size_bytes"),
                                       ArrowCharView("14")),
            GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "format_wkt", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, (char*)buffer.data, &schema_out, &error),
            GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "u");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
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
  EXPECT_EQ(std::string(item.data, item.size_bytes), "LINESTRING (30");
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  ArrowBufferReset(&buffer);
}

TEST(KernelTest, KernelTestFormatWKTFromGeoArrow) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;

  // Use at least one LINESTRING, which forces a slightly different path through the
  // visitor.
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_LINESTRING),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array_in.children[0]->children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array_in.children[0]->children[1], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array_in.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array_in.children[0]->children[0], 0), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array_in.children[0]->children[1], 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(array_in.children[0]), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array_in), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  struct ArrowBuffer buffer;
  ASSERT_EQ(ArrowMetadataBuilderInit(&buffer, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowMetadataBuilderAppend(&buffer, ArrowCharView("max_element_size_bytes"),
                                       ArrowCharView("14")),
            GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "format_wkt", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, (char*)buffer.data, &schema_out, &error),
            GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "u");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
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
  EXPECT_EQ(std::string(item.data, item.size_bytes), "LINESTRING (30");
  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
  ArrowBufferReset(&buffer);
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
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "as_geoarrow", nullptr), GEOARROW_OK);

  struct ArrowBuffer buffer;
  ASSERT_EQ(ArrowMetadataBuilderInit(&buffer, nullptr), GEOARROW_OK);
  ASSERT_EQ(
      ArrowMetadataBuilderAppend(&buffer, ArrowCharView("type"), ArrowCharView("100001")),
      GEOARROW_OK);

  EXPECT_EQ(kernel.start(&kernel, &schema_in, reinterpret_cast<char*>(buffer.data),
                         &schema_out, &error),
            GEOARROW_OK);
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

  ArrowBufferReset(&buffer);
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
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

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

TEST(KernelTest, KernelTestUniqueGeometryTypes) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (30 10)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("POINT (0 1)")), GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING Z (30 10 1, 0 0 2)")),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING M EMPTY")),
            GEOARROW_OK);
  ASSERT_EQ(
      ArrowArrayAppendString(
          &array_in, ArrowCharView("MULTIPOLYGON M (((0 0 0, 1 0 0, 0 1 0, 0 0 0)))")),
      GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(
                &array_in, ArrowCharView("GEOMETRYCOLLECTION ZM (POINT ZM (30 10 0 0))")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "unique_geometry_types_agg", nullptr),
            GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);
  EXPECT_STREQ(schema_out.format, "i");
  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, &array_out1, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  ASSERT_EQ(array_out1.length, 4);
  EXPECT_EQ(array_out1.null_count, 0);

  struct ArrowArrayView array_view;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);
  EXPECT_EQ(ArrowArrayViewGetIntUnsafe(&array_view, 0), 1);
  EXPECT_EQ(ArrowArrayViewGetIntUnsafe(&array_view, 1), 1002);
  EXPECT_EQ(ArrowArrayViewGetIntUnsafe(&array_view, 2), 2006);
  EXPECT_EQ(ArrowArrayViewGetIntUnsafe(&array_view, 3), 3007);

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
}

TEST(KernelTest, KernelTestBox) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING (3 -1, 0 10)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING (20 -40, 21 5)")),
            GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "box", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);

  struct GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema_out, &error), GEOARROW_OK);
  EXPECT_EQ(schema_view.type, GEOARROW_TYPE_BOX);

  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, &array_out1, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, nullptr, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  EXPECT_EQ(array_out1.length, 3);
  EXPECT_EQ(array_out1.null_count, 1);

  struct ArrowArrayView array_view;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);

  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[0], 0), 0);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[1], 0), -1);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[2], 0), 3);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[3], 0), 10);

  EXPECT_TRUE(ArrowArrayViewIsNull(&array_view, 1));

  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[0], 2), 20);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[1], 2), -40);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[2], 2), 21);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[3], 2), 5);

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
}

TEST(KernelTest, KernelTestBoxAgg) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;
  struct ArrowArray array_in;
  struct ArrowArray array_out1;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array_in, &schema_in, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array_in), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING (3 -1, 0 10)")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array_in, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array_in, ArrowCharView("LINESTRING (20 -40, 21 5)")),
            GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array_in, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowKernelInit(&kernel, "box_agg", nullptr), GEOARROW_OK);
  EXPECT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);

  struct GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema_out, &error), GEOARROW_OK);
  EXPECT_EQ(schema_view.type, GEOARROW_TYPE_BOX);

  EXPECT_EQ(kernel.push_batch(&kernel, &array_in, nullptr, &error), GEOARROW_OK);
  EXPECT_EQ(kernel.finish(&kernel, &array_out1, &error), GEOARROW_OK);

  kernel.release(&kernel);
  EXPECT_EQ(kernel.release, nullptr);

  EXPECT_EQ(array_out1.length, 1);
  EXPECT_EQ(array_out1.null_count, 0);

  struct ArrowArrayView array_view;
  ASSERT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema_out, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayViewSetArray(&array_view, &array_out1, nullptr), GEOARROW_OK);

  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[0], 0), 0);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[1], 0), -40);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[2], 0), 21);
  EXPECT_EQ(ArrowArrayViewGetDoubleUnsafe(array_view.children[3], 0), 10);

  ArrowArrayViewReset(&array_view);
  schema_in.release(&schema_in);
  schema_out.release(&schema_out);
  array_in.release(&array_in);
  array_out1.release(&array_out1);
}

TEST(KernelTest, KernelTestBoxMetadata) {
  struct GeoArrowKernel kernel;
  struct GeoArrowError error;

  struct ArrowSchema schema_in;
  struct ArrowSchema schema_out;

  struct GeoArrowMetadataView metadata_in {};
  struct GeoArrowMetadataView metadata_out;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema_in, GEOARROW_TYPE_WKT), GEOARROW_OK);
  metadata_in.crs.data = "EPSG:1234";
  metadata_in.crs.size_bytes = 9;
  metadata_in.crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema_in, &metadata_in), GEOARROW_OK);

  ASSERT_EQ(GeoArrowKernelInit(&kernel, "box", nullptr), GEOARROW_OK);
  ASSERT_EQ(kernel.start(&kernel, &schema_in, nullptr, &schema_out, &error), GEOARROW_OK);

  struct GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema_out, &error), GEOARROW_OK);
  EXPECT_EQ(schema_view.type, GEOARROW_TYPE_BOX);

  ASSERT_EQ(
      GeoArrowMetadataViewInit(&metadata_out, schema_view.extension_metadata, &error),
      GEOARROW_OK);
  ASSERT_EQ(std::string(metadata_out.crs.data, metadata_out.crs.size_bytes),
            "\"EPSG:1234\"");

  ArrowSchemaRelease(&schema_in);
  ArrowSchemaRelease(&schema_out);
  kernel.release(&kernel);
}
