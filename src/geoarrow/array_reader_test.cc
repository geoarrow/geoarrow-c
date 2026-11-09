
#include <algorithm>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

#include "geoarrow/wkx_testing.hpp"

static int AppendGeometryCallback(GeoArrowGeometryView geom, void* data,
                                  GeoArrowError* error) {
  return GeoArrowArrayWriterAppend(static_cast<GeoArrowArrayWriter*>(data), geom, error);
}

static void ExpectGeometryCallbacks(GeoArrowArrayReader* reader, int64_t length,
                                    const std::vector<std::string>& expected) {
  GeoArrowArrayWriter writer;
  ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayWriterSetFlatMultipoint(&writer, 0), GEOARROW_OK);
  GeoArrowError error{};
  ASSERT_EQ(GeoArrowArrayReaderVisitGeometry(reader, 0, length, AppendGeometryCallback,
                                             &writer, &error),
            GEOARROW_OK)
      << error.message;
  ArrowArray result;
  ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &result, &error), GEOARROW_OK);
  GeoArrowArrayWriterReset(&writer);
  GeoArrowArrayReader result_reader;
  ASSERT_EQ(GeoArrowArrayReaderInitFromType(&result_reader, GEOARROW_TYPE_WKT),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderSetArray(&result_reader, &result, &error), GEOARROW_OK);
  WKXTester tester;
  ASSERT_EQ(GeoArrowArrayReaderVisit(&result_reader, 0, length, tester.WKTVisitor()),
            GEOARROW_OK);
  EXPECT_EQ(tester.WKTValues("null"), expected);
  GeoArrowArrayReaderReset(&result_reader);
  ArrowArrayRelease(&result);
}

TEST(ArrayReaderTest, ArrayReaderTestInit) {
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, GEOARROW_TYPE_POINT), GEOARROW_OK);
  GeoArrowArrayReaderReset(&reader);

  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderInitFromSchema(&reader, &schema, nullptr), GEOARROW_OK);

  const struct GeoArrowArrayView* view = nullptr;
  ASSERT_EQ(GeoArrowArrayReaderArrayView(&reader, &view), GEOARROW_OK);
  ASSERT_NE(view, nullptr);
  ASSERT_EQ(view->schema_view.type, GEOARROW_TYPE_POINT);

  GeoArrowArrayReaderReset(&reader);
  ArrowSchemaRelease(&schema);
}

TEST(ArrayReaderTest, ArrayReaderTestVisitWKT) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_LARGE_WKT, GEOARROW_TYPE_WKT_VIEW}) {
    SCOPED_TRACE(type);

    // Build the array for [POINT (30 10), null]
    ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

    ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView("POINT (30 10)")),
              GEOARROW_OK);
    ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

    ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

    // Check its contents
    WKXTester tester;
    struct GeoArrowArrayReader reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromSchema(&reader, &schema, nullptr), GEOARROW_OK);
    EXPECT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);

    // Check visit of zero length
    EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, 0, 0, tester.WKTVisitor()), GEOARROW_OK);
    EXPECT_EQ(tester.WKTValues("<null value>"), std::vector<std::string>());

    // Check visit
    EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, 0, array.length, tester.WKTVisitor()),
              GEOARROW_OK);
    auto values = tester.WKTValues("<null value>");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], "POINT (30 10)");
    EXPECT_EQ(values[1], "<null value>");

    schema.release(&schema);
    array.release(&array);
    GeoArrowArrayReaderReset(&reader);
  }
}

TEST(ArrayReaderTest, ArrayReaderTestVisitWKB) {
  struct ArrowSchema schema;
  struct ArrowArray array;

  for (auto type : {GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKB, GEOARROW_TYPE_WKB_VIEW}) {
    SCOPED_TRACE(type);
    std::vector<uint8_t> point({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40});
    struct ArrowBufferView point_view;
    point_view.data.as_uint8 = point.data();
    point_view.size_bytes = point.size();

    // Build the array for [POINT (30 10), null]
    ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

    ASSERT_EQ(ArrowArrayAppendBytes(&array, point_view), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

    ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

    // Check its contents
    WKXTester tester;
    struct GeoArrowArrayReader reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromSchema(&reader, &schema, nullptr), GEOARROW_OK);
    EXPECT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);

    // Check visit of zero length
    EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, 0, 0, tester.WKTVisitor()), GEOARROW_OK);
    EXPECT_EQ(tester.WKTValues("<null value>"), std::vector<std::string>());

    // Check visit
    EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, 0, array.length, tester.WKTVisitor()),
              GEOARROW_OK);
    auto values = tester.WKTValues("<null value>");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], "POINT (30 10)");
    EXPECT_EQ(values[1], "<null value>");

    schema.release(&schema);
    array.release(&array);
    GeoArrowArrayReaderReset(&reader);
  }
}

TEST(ArrayReaderTest, ArrayReaderTestVisitGeoArrow) {
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GEOARROW_TYPE_POINT;

  // Build the array for [POINT (30 10), null]
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayAppendDouble(array.children[0], 30), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendDouble(array.children[1], 10), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);

  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);

  // Check its contents
  WKXTester tester;
  struct GeoArrowArrayReader reader;
  ASSERT_EQ(GeoArrowArrayReaderInitFromSchema(&reader, &schema, nullptr), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
  // Check visit of zero length
  EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, 0, 0, tester.WKTVisitor()), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValues("<null value>"), std::vector<std::string>());

  // Check visit

  EXPECT_EQ(GeoArrowArrayReaderVisit(&reader, 0, array.length, tester.WKTVisitor()),
            GEOARROW_OK);
  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 2);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");

  schema.release(&schema);
  array.release(&array);
  GeoArrowArrayReaderReset(&reader);
}

TEST(ArrayReaderTest, ReadSerializedGeometry) {
  WKXTester tester;
  const std::vector<std::string> wkts = {
      "POINT (1 2)", "LINESTRING (1 2, 3 4)", "POINT EMPTY",
      "GEOMETRYCOLLECTION (POINT Z (1 2 3), LINESTRING Z (4 5 6, 7 8 9))"};
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_LARGE_WKT, GEOARROW_TYPE_WKT_VIEW,
                    GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKB, GEOARROW_TYPE_WKB_VIEW}) {
    SCOPED_TRACE(type);
    bool is_wkt = type == GEOARROW_TYPE_WKT || type == GEOARROW_TYPE_LARGE_WKT ||
                  type == GEOARROW_TYPE_WKT_VIEW;
    ArrowSchema schema;
    ArrowArray array;
    ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);
    for (const auto& wkt : wkts) {
      if (is_wkt) {
        ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView(wkt.c_str())),
                  GEOARROW_OK);
      } else {
        auto wkb = tester.AsWKB(wkt);
        ArrowBufferView bytes;
        bytes.data.as_uint8 = wkb.data();
        bytes.size_bytes = static_cast<int64_t>(wkb.size());
        ASSERT_EQ(ArrowArrayAppendBytes(&array, bytes), GEOARROW_OK);
      }
    }
    ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
    // Both the slice offset and the read offset must be applied exactly once.
    array.offset = 1;
    array.length -= 1;
    GeoArrowArrayReader reader;
    GeoArrowGeometry geom;
    GeoArrowGeometry saved;
    ASSERT_EQ(GeoArrowArrayReaderInitFromSchema(&reader, &schema, nullptr), GEOARROW_OK);
    ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
    ASSERT_EQ(GeoArrowGeometryInit(&saved), GEOARROW_OK);
    GeoArrowError error{};
    EXPECT_EQ(GeoArrowArrayReaderRead(&reader, 0, &geom, &error), EINVAL);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, &error), GEOARROW_OK);
    EXPECT_EQ(GeoArrowArrayReaderRead(&reader, -1, &geom, &error), EINVAL);
    EXPECT_EQ(GeoArrowArrayReaderRead(&reader, array.length, &geom, &error), EINVAL);
    ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 0, &saved, &error), GEOARROW_OK);
    for (size_t i = 0; i < wkts.size(); i++) {
      ASSERT_EQ(GeoArrowArrayReaderRead(&reader, static_cast<int64_t>(i), &geom, &error),
                GEOARROW_OK)
          << error.message;
      EXPECT_EQ(tester.AsWKT(geom), wkts[i]);
    }
    ASSERT_EQ(GeoArrowArrayReaderRead(&reader, array.length - 1, &geom, &error),
              GEOARROW_OK);
    ASSERT_EQ(geom.size_nodes, 1);
    EXPECT_EQ(geom.root->geometry_type, GEOARROW_GEOMETRY_TYPE_GEOMETRY);
    auto callback_expected = wkts;
    callback_expected.push_back("null");
    ExpectGeometryCallbacks(&reader, array.length, callback_expected);
    ExpectGeometryCallbacks(&reader, array.length, callback_expected);
    GeoArrowArrayReaderReset(&reader);
    // Reading another feature and destroying the reader must not invalidate nodes
    // or WKT coordinates owned by a different output geometry.
    EXPECT_EQ(tester.AsWKT(saved), wkts[0]);
    GeoArrowGeometryReset(&saved);
    GeoArrowGeometryReset(&geom);
    ArrowArrayRelease(&array);
    ArrowSchemaRelease(&schema);
  }
}

TEST(ArrayReaderTest, ReadNativeGeometry) {
  WKXTester tester;
  const char* dim_names[] = {"", " Z", " M", " ZM"};
  const char* coords[] = {"1 2", "1 2 3", "1 2 3", "1 2 3 4"};
  for (int dim = 0; dim < 4; dim++) {
    std::string xy = coords[dim];
    std::vector<std::string> bodies = {"(" + xy + ")",
                                       "(" + xy + ", " + xy + ")",
                                       "((" + xy + ", " + xy + ", " + xy + "))",
                                       "((" + xy + "), (" + xy + "))",
                                       "((" + xy + ", " + xy + "), EMPTY)",
                                       "(((" + xy + ", " + xy + ", " + xy + ")), EMPTY)"};
    for (int geometry_type = 1; geometry_type <= 6; geometry_type++) {
      for (auto layout :
           {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
        auto type = GeoArrowMakeType(static_cast<GeoArrowGeometryType>(geometry_type),
                                     static_cast<GeoArrowDimensions>(dim + 1), layout);
        SCOPED_TRACE(type);
        std::string prefix = std::string(GeoArrowGeometryTypeString(
                                 static_cast<GeoArrowGeometryType>(geometry_type))) +
                             dim_names[dim];
        std::string wkt = prefix + " " + bodies[geometry_type - 1];
        std::string empty = prefix + " EMPTY";
        GeoArrowArrayWriter writer;
        GeoArrowVisitor v{};
        ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, type), GEOARROW_OK);
        ASSERT_EQ(GeoArrowArrayWriterInitVisitor(&writer, &v), GEOARROW_OK);
        for (const auto& item : {empty, wkt, empty}) {
          ASSERT_EQ(
              GeoArrowArrayWriterAppend(
                  &writer, GeoArrowGeometryAsView(&tester.AsGeometry(item)), nullptr),
              GEOARROW_OK);
        }
        ASSERT_EQ(v.feat_start(&v), GEOARROW_OK);
        ASSERT_EQ(v.null_feat(&v), GEOARROW_OK);
        ASSERT_EQ(v.feat_end(&v), GEOARROW_OK);
        ArrowArray array;
        ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
        GeoArrowArrayWriterReset(&writer);
        array.offset = 1;
        array.length = 3;
        GeoArrowArrayReader reader;
        GeoArrowGeometry geom;
        ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
        ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
        ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
        ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 0, &geom, nullptr), GEOARROW_OK);
        EXPECT_EQ(tester.AsWKT(geom), wkt);
        const GeoArrowArrayView* view;
        ASSERT_EQ(GeoArrowArrayReaderArrayView(&reader, &view), GEOARROW_OK);
        // The first nonempty sequence must borrow the coordinate buffers.
        const GeoArrowGeometryNode* sequence = geom.root;
        while (sequence->geometry_type != GEOARROW_GEOMETRY_TYPE_POINT &&
               sequence->geometry_type != GEOARROW_GEOMETRY_TYPE_LINESTRING) {
          sequence++;
        }
        int64_t coord_offset = geometry_type == 1 ? 1 : 0;
        EXPECT_EQ(sequence->coords[0], reinterpret_cast<const uint8_t*>(
                                           view->coords.values[0] +
                                           coord_offset * view->coords.coords_stride));
        ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 1, &geom, nullptr), GEOARROW_OK);
        EXPECT_EQ(tester.AsWKT(geom), empty);
        ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 2, &geom, nullptr), GEOARROW_OK);
        ASSERT_EQ(geom.size_nodes, 1);
        EXPECT_EQ(geom.root->geometry_type, GEOARROW_GEOMETRY_TYPE_GEOMETRY);
        ExpectGeometryCallbacks(&reader, 3, {wkt, empty, "null"});
        GeoArrowGeometryReset(&geom);
        GeoArrowArrayReaderReset(&reader);
        ArrowArrayRelease(&array);
      }
    }
  }
}

TEST(ArrayReaderTest, ReadMalformedSerializedGeometry) {
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_WKB}) {
    ArrowArray array;
    ASSERT_EQ(
        ArrowArrayInitFromType(&array, type == GEOARROW_TYPE_WKT ? NANOARROW_TYPE_STRING
                                                                 : NANOARROW_TYPE_BINARY),
        GEOARROW_OK);
    ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView("bad")), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
    GeoArrowArrayReader reader;
    GeoArrowGeometry geom;
    GeoArrowError error{};
    ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, &error), GEOARROW_OK);
    EXPECT_EQ(GeoArrowArrayReaderRead(&reader, 0, &geom, &error), EINVAL);
    EXPECT_NE(error.message[0], '\0');
    GeoArrowGeometryReset(&geom);
    GeoArrowArrayReaderReset(&reader);
    ArrowArrayRelease(&array);
  }
}

TEST(ArrayReaderTest, ReadOppositeEndianWKB) {
  WKXTester tester;
  auto wkb = tester.AsWKB("POINT ZM (1 2 3 4)");
  wkb[0] = !wkb[0];
  std::reverse(wkb.begin() + 1, wkb.begin() + 5);
  for (size_t i = 5; i < wkb.size(); i += 8) {
    std::reverse(wkb.begin() + i, wkb.begin() + i + 8);
  }
  ArrowArray array;
  ASSERT_EQ(ArrowArrayInitFromType(&array, NANOARROW_TYPE_BINARY), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
  ArrowBufferView bytes;
  bytes.data.as_uint8 = wkb.data();
  bytes.size_bytes = static_cast<int64_t>(wkb.size());
  ASSERT_EQ(ArrowArrayAppendBytes(&array, bytes), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
  GeoArrowArrayReader reader;
  GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, GEOARROW_TYPE_WKB), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 0, &geom, nullptr), GEOARROW_OK);
  EXPECT_EQ(geom.root->coords[0], static_cast<const uint8_t*>(array.buffers[2]) + 5);
  EXPECT_EQ(tester.AsWKT(geom), "POINT ZM (1 2 3 4)");
  GeoArrowGeometry retained;
  ASSERT_EQ(GeoArrowGeometryInit(&retained), GEOARROW_OK);
  auto retain = [](GeoArrowGeometryView value, void* data, GeoArrowError*) {
    return GeoArrowGeometryDeepCopy(value, static_cast<GeoArrowGeometry*>(data));
  };
  ASSERT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 0, 1, retain, &retained, nullptr),
            GEOARROW_OK);
  EXPECT_EQ(retained.root->flags & GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN, 0);
  EXPECT_EQ(tester.AsWKT(retained), "POINT ZM (1 2 3 4)");
  GeoArrowGeometryReset(&retained);
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_WKB, GEOARROW_TYPE_POINT_ZM,
                    GEOARROW_TYPE_INTERLEAVED_POINT_ZM}) {
    GeoArrowArrayWriter writer;
    ArrowArray output;
    ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayWriterAppend(&writer, GeoArrowGeometryAsView(&geom), nullptr),
              GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &output, nullptr), GEOARROW_OK);
    GeoArrowArrayWriterReset(&writer);
    GeoArrowArrayReader output_reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromType(&output_reader, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&output_reader, &output, nullptr), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderVisit(&output_reader, 0, 1, tester.WKTVisitor()),
              GEOARROW_OK);
    EXPECT_EQ(tester.WKTValue(), "POINT ZM (1 2 3 4)");
    GeoArrowArrayReaderReset(&output_reader);
    ArrowArrayRelease(&output);
  }
  GeoArrowGeometryReset(&geom);
  GeoArrowArrayReaderReset(&reader);
  ArrowArrayRelease(&array);
}

TEST(ArrayReaderTest, ReadBoxGeometry) {
  ArrowSchema schema;
  ArrowArray array;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_BOX), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(ArrowArrayAppendDouble(array.children[i], i + 1), GEOARROW_OK);
  }
  ASSERT_EQ(ArrowArrayFinishElement(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
  GeoArrowArrayReader reader;
  GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, GEOARROW_TYPE_BOX), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 0, &geom, nullptr), GEOARROW_OK);
  GeoArrowArrayReaderReset(&reader);
  ArrowArrayRelease(&array);
  ArrowSchemaRelease(&schema);
  // Expanded box coordinates must be owned by geom, independent of the input.
  WKXTester tester;
  EXPECT_EQ(tester.AsWKT(geom), "POLYGON ((1 2, 3 2, 3 4, 1 4, 1 2))");
  GeoArrowGeometryReset(&geom);
}

TEST(ArrayReaderTest, ReadLargeWKTGeometry) {
  // Exercise multiple WKT coordinate-cache flushes and geometry reallocations.
  std::string text = "LINESTRING (1 2";
  for (int i = 1; i < 1024; i++) text += ", 3 4";
  text += ')';
  ArrowArray array;
  ASSERT_EQ(ArrowArrayInitFromType(&array, NANOARROW_TYPE_STRING), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView(text.c_str())), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
  GeoArrowArrayReader reader;
  GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, GEOARROW_TYPE_WKT), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayReaderRead(&reader, 0, &geom, nullptr), GEOARROW_OK);
  GeoArrowArrayReaderReset(&reader);
  ArrowArrayRelease(&array);
  WKXTester tester;
  EXPECT_EQ(tester.AsWKT(geom), text);
  GeoArrowGeometryReset(&geom);
}

TEST(ArrayReaderTest, GeometryCallbackControlFlow) {
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_WKB, GEOARROW_TYPE_POINT}) {
    WKXTester tester;
    GeoArrowArrayWriter writer;
    ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, type), GEOARROW_OK);
    for (int i = 0; i < 4; i++) {
      if (i == 2) {
        ASSERT_EQ(GeoArrowArrayWriterAppendNull(&writer), GEOARROW_OK);
      } else {
        ASSERT_EQ(GeoArrowArrayWriterAppend(
                      &writer, GeoArrowGeometryAsView(&tester.AsGeometry("POINT (1 2)")),
                      nullptr),
                  GEOARROW_OK);
      }
    }
    ArrowArray array;
    ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
    GeoArrowArrayWriterReset(&writer);
    array.offset = 1;
    array.length = 3;
    GeoArrowArrayReader reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
    GeoArrowGeometry saved;
    ASSERT_EQ(GeoArrowGeometryInit(&saved), GEOARROW_OK);
    struct Context {
      int calls = 0;
      int stop_after = 0;
      GeoArrowGeometry* saved;
      GeoArrowError* expected_error;
    };
    GeoArrowError error{};
    Context context{0, 1, &saved, &error};
    auto callback = [](GeoArrowGeometryView geom, void* data, GeoArrowError* error) {
      auto ctx = static_cast<Context*>(data);
      EXPECT_EQ(error, ctx->expected_error);
      ctx->calls++;
      if (ctx->calls == ctx->stop_after) {
        GeoArrowErrorSet(error, "Callback stopped iteration");
        return EAGAIN;
      }
      if (geom.root->geometry_type != GEOARROW_GEOMETRY_TYPE_GEOMETRY) {
        return GeoArrowGeometryDeepCopy(geom, ctx->saved);
      }
      return GEOARROW_OK;
    };
    EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 1, 2, callback, &context, &error),
              EAGAIN);
    EXPECT_EQ(context.calls, 1);
    EXPECT_STREQ(error.message, "Callback stopped iteration");
    context.calls = 0;
    context.stop_after = 0;
    EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 1, 2, callback, &context, &error),
              GEOARROW_OK);
    EXPECT_EQ(context.calls, 2);
    EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 3, 0, nullptr, nullptr, &error),
              GEOARROW_OK);
    for (auto range : {std::pair<int64_t, int64_t>{-1, 1},
                       {0, -1},
                       {2, 2},
                       {4, 0},
                       {1, INT64_MAX},
                       {INT64_MAX, 1}}) {
      EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, range.first, range.second,
                                                 callback, &context, &error),
                EINVAL);
    }
    EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 0, 1, nullptr, nullptr, &error),
              EINVAL);
    EXPECT_EQ(context.calls, 2);
    GeoArrowArrayReaderReset(&reader);
    ArrowArrayRelease(&array);
    EXPECT_EQ(tester.AsWKT(saved), "POINT (1 2)");
    GeoArrowGeometryReset(&saved);
  }
}

TEST(ArrayReaderTest, GeometryCallbackParseError) {
  WKXTester tester;
  for (auto type : {GEOARROW_TYPE_WKT, GEOARROW_TYPE_LARGE_WKT, GEOARROW_TYPE_WKT_VIEW,
                    GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKB, GEOARROW_TYPE_WKB_VIEW}) {
    bool is_wkt = type == GEOARROW_TYPE_WKT || type == GEOARROW_TYPE_LARGE_WKT ||
                  type == GEOARROW_TYPE_WKT_VIEW;
    ArrowSchema schema;
    ArrowArray array;
    ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayAppendNull(&array, 1), GEOARROW_OK);
    ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView("bad")), GEOARROW_OK);
    if (is_wkt) {
      ASSERT_EQ(ArrowArrayAppendString(&array, ArrowCharView("POINT (1 2)")),
                GEOARROW_OK);
    } else {
      auto wkb = tester.AsWKB("POINT (1 2)");
      ArrowBufferView bytes;
      bytes.data.as_uint8 = wkb.data();
      bytes.size_bytes = static_cast<int64_t>(wkb.size());
      ASSERT_EQ(ArrowArrayAppendBytes(&array, bytes), GEOARROW_OK);
    }
    ASSERT_EQ(ArrowArrayFinishBuildingDefault(&array, nullptr), GEOARROW_OK);
    GeoArrowArrayReader reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
    auto callback = [](GeoArrowGeometryView, void* data, GeoArrowError*) {
      ++*static_cast<int*>(data);
      return GEOARROW_OK;
    };
    int calls = 0;
    GeoArrowError error{};
    EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 0, 3, callback, &calls, &error),
              EINVAL);
    EXPECT_EQ(calls, 1);
    EXPECT_NE(error.message[0], '\0');
    // Reuse the same reader after a parser error, with an optional error pointer.
    EXPECT_EQ(GeoArrowArrayReaderVisitGeometry(&reader, 2, 1, callback, &calls, nullptr),
              GEOARROW_OK);
    EXPECT_EQ(calls, 2);
    GeoArrowArrayReaderReset(&reader);
    ArrowArrayRelease(&array);
    ArrowSchemaRelease(&schema);
  }
}

TEST(ArrayReaderTest, GeometryCallbackCoordinateOffsets) {
  WKXTester tester;
  for (auto type : {GEOARROW_TYPE_POINT, GEOARROW_TYPE_INTERLEAVED_POINT,
                    GEOARROW_TYPE_LINESTRING, GEOARROW_TYPE_INTERLEAVED_LINESTRING}) {
    GeoArrowArrayWriter writer;
    ASSERT_EQ(GeoArrowArrayWriterInitFromType(&writer, type), GEOARROW_OK);
    bool point = GeoArrowGeometryTypeFromType(type) == GEOARROW_GEOMETRY_TYPE_POINT;
    if (point) {
      for (const char* wkt :
           {"POINT (-2 -2)", "POINT (-1 -1)", "POINT (1 2)", "POINT (3 4)"}) {
        ASSERT_EQ(GeoArrowArrayWriterAppend(
                      &writer, GeoArrowGeometryAsView(&tester.AsGeometry(wkt)), nullptr),
                  GEOARROW_OK);
      }
    } else {
      ASSERT_EQ(GeoArrowArrayWriterAppend(&writer,
                                          GeoArrowGeometryAsView(&tester.AsGeometry(
                                              "LINESTRING (-2 -2, -1 -1, 1 2, 3 4)")),
                                          nullptr),
                GEOARROW_OK);
    }
    ArrowArray array;
    ASSERT_EQ(GeoArrowArrayWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
    GeoArrowArrayWriterReset(&writer);
    ArrowArray* coords = point ? &array : array.children[0];
    coords->offset = 1;
    coords->length = point ? 2 : 3;
    if (GeoArrowCoordTypeFromType(type) == GEOARROW_COORD_TYPE_INTERLEAVED) {
      coords->children[0]->offset = 2;
      coords->children[0]->length -= 2;
    } else {
      for (int i = 0; i < 2; i++) {
        coords->children[i]->offset = 1;
        coords->children[i]->length--;
      }
    }
    if (!point) {
      // The sliced coordinate array supplies only the two retained coordinates.
      const_cast<int32_t*>(static_cast<const int32_t*>(array.buffers[1]))[1] = 2;
    }
    GeoArrowArrayReader reader;
    ASSERT_EQ(GeoArrowArrayReaderInitFromType(&reader, type), GEOARROW_OK);
    ASSERT_EQ(GeoArrowArrayReaderSetArray(&reader, &array, nullptr), GEOARROW_OK);
    if (point) {
      ExpectGeometryCallbacks(&reader, 2, {"POINT (1 2)", "POINT (3 4)"});
    } else {
      ExpectGeometryCallbacks(&reader, 1, {"LINESTRING (1 2, 3 4)"});
    }
    GeoArrowArrayReaderReset(&reader);
    ArrowArrayRelease(&array);
  }
}
