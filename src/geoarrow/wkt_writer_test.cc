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

  GeoArrowWKTWriterReset(&writer);
}

TEST(WKTWriterTest, WKTWriterTestOneValidOneNull) {
  struct GeoArrowWKTWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKTWriterInit(&writer);
  GeoArrowWKTWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY), GEOARROW_OK);
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
