

#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.hpp"
#include "geoarrow/wkx_testing.hpp"

template <typename Array>
void DoSomethingWithArray(const Array& array, bool* was_called,
                          enum GeoArrowGeometryType expected_geometry_type,
                          enum GeoArrowDimensions expected_dimensions) {
  EXPECT_EQ(array.value.size(), 0);
  EXPECT_EQ(Array::geometry_type, expected_geometry_type);
  EXPECT_EQ(Array::dimensions, expected_dimensions);

  constexpr enum GeoArrowType type_id = geoarrow::type_traits::GeometryTypeTraits<
      Array::geometry_type, Array::dimensions>::type_id(GEOARROW_COORD_TYPE_SEPARATE);
  using type_traits = geoarrow::type_traits::TypeTraits<type_id>;
  EXPECT_EQ(type_traits::geometry_type, expected_geometry_type);
  EXPECT_EQ(type_traits::dimensions, expected_dimensions);
  EXPECT_EQ(type_traits::coord_type_id, GEOARROW_COORD_TYPE_SEPARATE);

  constexpr enum GeoArrowType type_id_interleaved =
      geoarrow::type_traits::GeometryTypeTraits<Array::geometry_type, Array::dimensions>::
          type_id(GEOARROW_COORD_TYPE_INTERLEAVED);
  using type_traits_interleaved = geoarrow::type_traits::TypeTraits<type_id_interleaved>;
  EXPECT_EQ(type_traits_interleaved::geometry_type, expected_geometry_type);
  EXPECT_EQ(type_traits_interleaved::dimensions, expected_dimensions);
  EXPECT_EQ(type_traits_interleaved::coord_type_id, GEOARROW_COORD_TYPE_INTERLEAVED);

  *was_called = true;
}

TEST(GeoArrowHppTest, DispatchNativeArray) {
  enum GeoArrowGeometryType all_geometry_types[] = {
      GEOARROW_GEOMETRY_TYPE_POINT,           GEOARROW_GEOMETRY_TYPE_LINESTRING,
      GEOARROW_GEOMETRY_TYPE_POLYGON,         GEOARROW_GEOMETRY_TYPE_MULTIPOINT,
      GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON};
  enum GeoArrowDimensions all_dimensions[] = {
      GEOARROW_DIMENSIONS_XY, GEOARROW_DIMENSIONS_XYZ, GEOARROW_DIMENSIONS_XYM,
      GEOARROW_DIMENSIONS_XYZM};

  for (auto geometry_type : all_geometry_types) {
    for (auto dimensions : all_dimensions) {
      SCOPED_TRACE(std::string(GeoArrowGeometryTypeString(geometry_type)) + " / " +
                   GeoArrowDimensionsString(dimensions));
      bool was_called = false;
      enum GeoArrowType type_id =
          GeoArrowMakeType(geometry_type, dimensions, GEOARROW_COORD_TYPE_SEPARATE);
      GEOARROW_DISPATCH_NATIVE_ARRAY_CALL(type_id, DoSomethingWithArray, &was_called,
                                          geometry_type, dimensions);
    }
  }
}

using BoxXYZM = geoarrow::array_util::BoxXYZM<double>;
using XYZM = BoxXYZM::bound_type;

template <typename Array>
void GenericBoundsXYZM(Array& array, const struct GeoArrowArrayView* array_view,
                       BoxXYZM* out) {
  array.Init(array_view);
  BoxXYZM bounds = *out;

  array.template VisitVertices<XYZM>([&](XYZM xyzm) {
    for (size_t i = 0; i < xyzm.size(); i++) {
      bounds[i] = std::min(bounds[i], xyzm[i]);
      bounds[xyzm.size() + i] = std::max(bounds[xyzm.size() + i], xyzm[i]);
    }
  });

  *out = bounds;
}

TEST(GeoArrowHppTest, DispatchBounds) {
  for (enum GeoArrowType type_id : {GEOARROW_TYPE_POINT, GEOARROW_TYPE_WKB}) {
    geoarrow::ArrayWriter writer(type_id);
    WKXTester tester;
    tester.ReadWKT("POINT (0 1)", writer.visitor());
    tester.ReadWKT("POINT (2 3)", writer.visitor());
    tester.ReadNulls(2, writer.visitor());

    struct ArrowArray array_feat;
    writer.Finish(&array_feat);

    geoarrow::ArrayReader reader(type_id);
    reader.SetArray(&array_feat);

    BoxXYZM bounds{std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity()};

    GEOARROW_DISPATCH_ARRAY_CALL(type_id, GenericBoundsXYZM, reader.View().array_view(),
                                 &bounds);
    ASSERT_EQ(bounds.xmin(), 0);
    ASSERT_EQ(bounds.ymin(), 1);
    ASSERT_EQ(bounds.xmax(), 2);
    ASSERT_EQ(bounds.ymax(), 3);
    ASSERT_EQ(bounds.zmin(), std::numeric_limits<double>::infinity());
    ASSERT_EQ(bounds.zmax(), -std::numeric_limits<double>::infinity());
  }
}
