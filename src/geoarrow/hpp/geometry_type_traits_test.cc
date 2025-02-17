

#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow.hpp"
#include "geometry_type_traits.hpp"

template <typename Array>
void DoSomethingWithArray(const Array& array, bool* was_called,
                          enum GeoArrowGeometryType expected_geometry_type,
                          enum GeoArrowDimensions expected_dimensions) {
  EXPECT_EQ(array.value.size(), 0);
  EXPECT_EQ(Array::geometry_type, expected_geometry_type);
  EXPECT_EQ(Array::dimensions, expected_dimensions);
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
