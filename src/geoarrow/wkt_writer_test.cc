
#include <cmath>
#include <cstring>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/wkx_testing.hpp"

std::string WKTEmpty(enum GeoArrowGeometryType geometry_type,
                     enum GeoArrowDimensions dimensions) {
  std::stringstream ss;
  ss << GeoArrowGeometryTypeString(geometry_type);

  switch (dimensions) {
    case GEOARROW_DIMENSIONS_XYZ:
      ss << " Z";
      break;
    case GEOARROW_DIMENSIONS_XYM:
      ss << " M";

      break;
    case GEOARROW_DIMENSIONS_XYZM:
      ss << " ZM";

      break;
    default:
      break;
  }

  ss << " EMPTY";
  return ss.str();
}

TEST(WKTWriterTest, WKTWriterTestBasic) {
  struct GeoArrowWKTWriter writer;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterReset(&writer);
}

int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result);

TEST(WKTWriterTest, WKTWriterTestPrintDouble) {
  // No more than 40 character should ever be written to the output buffer
  std::array<char, 40> out{};

  int64_t n_chars = GeoArrowPrintDouble(1e17, 16, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "100000000000000000");

  std::memset(out.data(), 0, sizeof(out));
  n_chars = GeoArrowPrintDouble(-1e17, 16, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "-100000000000000000");

  std::memset(out.data(), 0, sizeof(out));
  n_chars = GeoArrowPrintDouble(std::numeric_limits<double>::max(), 16, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "1.7976931348623157e+308");

  std::memset(out.data(), 0, sizeof(out));
  n_chars = GeoArrowPrintDouble(std::numeric_limits<double>::lowest(), 16, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "-1.7976931348623157e+308");

  // Check that our definition of precision is definitely digits after the decimal point
  n_chars = GeoArrowPrintDouble(123.456, 3, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "123.456");

  std::memset(out.data(), 0, sizeof(out));
  n_chars = GeoArrowPrintDouble(-234.567, 3, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "-234.567");

  // ...and that implementations strip trailing zeroes
  n_chars = GeoArrowPrintDouble(123.456, 4, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "123.456");

  std::memset(out.data(), 0, sizeof(out));
  n_chars = GeoArrowPrintDouble(-234.567, 4, out.data());
  EXPECT_EQ(std::string(out.data(), n_chars), "-234.567");

  // ryu and snprintf() serialize the last few decimal places differently
  std::memset(out.data(), 0, sizeof(out));
  n_chars = GeoArrowPrintDouble(M_PI * 100, 16, out.data());
  EXPECT_GE(n_chars, 17);
  EXPECT_EQ(std::string(out.data(), 17), "314.1592653589793");
}

TEST(WKTWriterTest, WKTWriterTestOneNull) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 1);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), NANOARROW_OK);

  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 0));

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestOneValidOneNull) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 3);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 0));
  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 1));
  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 2));
  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "POINT EMPTY");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestErrors) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({}, {});

  // Invalid because level < 0
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), EINVAL);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  coords.view()->n_coords = 1;
  EXPECT_EQ(v.coords(&v, coords.view()), EINVAL);

  GeoArrowWKTWriterReset(&writer);
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  // Invalid because of too much nesting
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  for (int i = 0; i < 32; i++) {
    EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
              GEOARROW_OK);
  }
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            EINVAL);

  GeoArrowWKTWriterReset(&writer);
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  // Invalid geometry type
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_GEOMETRY, GEOARROW_DIMENSIONS_XY),
            EINVAL);

  GeoArrowWKTWriterReset(&writer);
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  // Invalid dimensions
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_UNKNOWN),
            EINVAL);

  GeoArrowWKTWriterReset(&writer);
}

class GeometryTypeParameterizedTestFixture
    : public ::testing::TestWithParam<enum GeoArrowGeometryType> {
 protected:
  enum GeoArrowGeometryType type;
};

TEST_P(GeometryTypeParameterizedTestFixture, WKTWriterTestEmpty) {
  enum GeoArrowGeometryType geometry_type = GetParam();

  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, geometry_type, GEOARROW_DIMENSIONS_XY), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, geometry_type, GEOARROW_DIMENSIONS_XYZ), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, geometry_type, GEOARROW_DIMENSIONS_XYM), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, geometry_type, GEOARROW_DIMENSIONS_XYZM), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);
  EXPECT_EQ(array.buffers[0], nullptr);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XY));

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XYZ));

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XYM));

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XYZM));

  array.release(&array);
  ArrowArrayViewReset(&view);
  GeoArrowWKTWriterReset(&writer);
}

INSTANTIATE_TEST_SUITE_P(WKTWriterTest, GeometryTypeParameterizedTestFixture,
                         ::testing::Values(GEOARROW_GEOMETRY_TYPE_POINT,
                                           GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                           GEOARROW_GEOMETRY_TYPE_POLYGON,
                                           GEOARROW_GEOMETRY_TYPE_MULTIPOINT,
                                           GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                                           GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                                           GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION));

TEST(WKTWriterTest, WKTWriterTestPoint) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1}, {2}, {3}, {4});

  coords.view()->n_values = 2;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  coords.view()->n_values = 3;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZ),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  coords.view()->n_values = 3;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  coords.view()->n_values = 4;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "POINT (1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "POINT Z (1 2 3)");

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "POINT M (1 2 3)");

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "POINT ZM (1 2 3 4)");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestLinestring) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1, 2, 3, 1}, {2, 3, 4, 2}, {3, 4, 5, 3}, {4, 5, 6, 4});

  coords.view()->n_values = 2;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  coords.view()->n_values = 3;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZ),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  coords.view()->n_values = 3;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  coords.view()->n_values = 4;
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "LINESTRING (1 2, 2 3, 3 4, 1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "LINESTRING Z (1 2 3, 2 3 4, 3 4 5, 1 2 3)");

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "LINESTRING M (1 2 3, 2 3 4, 3 4 5, 1 2 3)");

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "LINESTRING ZM (1 2 3 4, 2 3 4 5, 3 4 5 6, 1 2 3 4)");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestPolygon) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1, 2, 3, 1}, {2, 3, 4, 2});

  // One ring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two rings
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "POLYGON ((1 2, 2 3, 3 4, 1 2))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "POLYGON ((1 2, 2 3, 3 4, 1 2), (1 2, 2 3, 3 4, 1 2))");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestMultipoint) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1}, {2});

  // One point
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Also check verbose multipoint output
  writer.use_flat_multipoint = 0;
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  // One point
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "MULTIPOINT (1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "MULTIPOINT (1 2, 1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "MULTIPOINT ((1 2))");

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "MULTIPOINT ((1 2), (1 2))");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestMultilinestring) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1, 2}, {2, 3});

  // One linestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two linestrings
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "MULTILINESTRING ((1 2, 2 3))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "MULTILINESTRING ((1 2, 2 3), (1 2, 2 3))");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestMultipolygon) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1, 2, 3, 1}, {2, 3, 4, 2});

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)), ((1 2, 2 3, 3 4, 1 2)))");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestGeometrycollection) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1}, {2});

  // One point
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "GEOMETRYCOLLECTION (POINT (1 2))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes),
            "GEOMETRYCOLLECTION (POINT (1 2), POINT (1 2))");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestStreamingCoords) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1, 2}, {2, 3});

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 1);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "LINESTRING (1 2, 2 3, 1 2, 2 3)");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestMaxFeatLen) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  writer.max_element_size_bytes = 6;
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  TestCoords coords({1, 2}, {2, 3});

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), EAGAIN);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Make sure we can try again with another feature
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, coords.view()), EAGAIN);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInitFromType(&view, NANOARROW_TYPE_STRING);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "LINEST");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.size_bytes), "LINEST");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestVeryLongCoords) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  std::vector<double> thirds(1024);
  for (int i = 0; i < 1024; i++) {
    // The longest ordinate I can think of
    thirds[i] = 1.333333333333333e-100;
  }
  TestCoords coords(thirds, thirds);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  for (int i = 0; i < 128; i++) {
    EXPECT_EQ(v.coords(&v, coords.view()), GEOARROW_OK);
  }

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 1);
  EXPECT_EQ(array.null_count, 0);

  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}
