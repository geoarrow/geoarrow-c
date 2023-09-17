
#include <gtest/gtest.h>

#include "geoarrow.h"

#include "wkx_testing.hpp"

TEST(ArrayReaderTest, ArrayReaderTestBasic) {
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInit(&reader), GEOARROW_OK);
  GeoArrowArrayReaderReset(&reader);
}

TEST(ArrayReaderTest, ArrayReaderTestVisitWKT) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_WKT;

  // Build the array for [POINT (30 10), null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView("POINT (30 10)")), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  WKXTester tester;
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInit(&reader), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, &array_view, 0, array.length,
                                     tester.WKTVisitor()),
            GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");

  schema.release(&schema);
  array.release(&array);
  GeoArrowArrayReaderReset(&reader);
}

TEST(ArrayReaderTest, ArrayReaderTestVisitWKB) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_WKB;

  std::basic_string<uint8_t> point({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});
  struct ArrowBufferView point_view;
  point_view.data.as_uint8 = point.data();
  point_view.size_bytes = point.size();

  // Build the array for [POINT (30 10), null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendBytes(&array, point_view), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  WKXTester tester;
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInit(&reader), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, &array_view, 0, array.length,
                                     tester.WKTVisitor()),
            GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");

  schema.release(&schema);
  array.release(&array);
  GeoArrowArrayReaderReset(&reader);
}

TEST(ArrayReaderTest, ArrayReaderTestVisitGeoArrow) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_POINT;

  // Build the array for [POINT (30 10), null]
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Set the array view
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  // Check its contents
  WKXTester tester;
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInit(&reader), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, &array_view, 0, array.length,
                                     tester.WKTVisitor()),
            GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");

  schema.release(&schema);
  array.release(&array);
  GeoArrowArrayReaderReset(&reader);
}
