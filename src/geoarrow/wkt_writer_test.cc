#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

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
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

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

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 0));
  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 1));
  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT EMPTY");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestErrors) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  // Invalid because level < 0
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), EINVAL);
  EXPECT_EQ(v.coords(&v, nullptr, 0, 2), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, nullptr, 1, 2), EINVAL);

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
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XY));

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XYZ));

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
            WKTEmpty(geometry_type, GEOARROW_DIMENSIONS_XYM));

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
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

  double xs[] = {1};
  double ys[] = {2};
  double zs[] = {3};
  double ms[] = {4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZ),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 3), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords_m, 1, 3), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 4), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT (1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT Z (1 2 3)");

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT M (1 2 4)");

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POINT ZM (1 2 3 4)");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestLinestring) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZ),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 3), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords_m, 4, 3), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZM),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 4), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "LINESTRING (1 2, 2 3, 3 4, 1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
            "LINESTRING Z (1 2 3, 2 3 4, 3 4 5, 1 2 3)");

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
            "LINESTRING M (1 2 4, 2 3 5, 3 4 6, 1 2 4)");

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
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

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  // One ring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two rings
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "POLYGON ((1 2, 2 3, 3 4, 1 2))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
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

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  // One point
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
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
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 4);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "MULTIPOINT (1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "MULTIPOINT (1 2, 1 2)");

  value = ArrowArrayViewGetStringUnsafe(&view, 2);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "MULTIPOINT ((1 2))");

  value = ArrowArrayViewGetStringUnsafe(&view, 3);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "MULTIPOINT ((1 2), (1 2))");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestMultilinestring) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  // One linestring
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 2, 2), GEOARROW_OK);
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
  EXPECT_EQ(v.coords(&v, (const double**)coords, 2, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 2, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "MULTILINESTRING ((1 2, 2 3))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
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

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  // Two points
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
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
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.ring_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 4, 2), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
            "MULTIPOLYGON (((1 2, 2 3, 3 4, 1 2)))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
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

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  // One point
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(
      v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION, GEOARROW_DIMENSIONS_XY),
      GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
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
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 1, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "GEOMETRYCOLLECTION (POINT (1 2))");

  value = ArrowArrayViewGetStringUnsafe(&view, 1);
  EXPECT_EQ(std::string(value.data, value.n_bytes),
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

  double xs[] = {1, 2, 3, 1};
  double ys[] = {2, 3, 4, 2};
  double zs[] = {3, 4, 5, 3};
  double ms[] = {4, 5, 6, 4};
  double* coords[] = {xs, ys, zs, ms};
  double* coords_m[] = {xs, ys, ms};

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 2, 2), GEOARROW_OK);
  EXPECT_EQ(v.coords(&v, (const double**)coords, 2, 2), GEOARROW_OK);
  EXPECT_EQ(v.geom_end(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKTWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 1);
  EXPECT_EQ(array.null_count, 0);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&view, 0);
  EXPECT_EQ(std::string(value.data, value.n_bytes), "LINESTRING (1 2, 2 3, 1 2, 2 3)");

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestVeryLongCoords) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  double thirds[1024];
  for (int i = 0; i < 1024; i++) {
    // The longest ordinate I can think of
    thirds[i] = 1.333333333333333e-100;
  }
  double* coords[] = {thirds, thirds, thirds, thirds};

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);

  for (int i = 0; i < 128; i++) {
    EXPECT_EQ(v.coords(&v, (const double**)coords, 1024, 2), GEOARROW_OK);
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
