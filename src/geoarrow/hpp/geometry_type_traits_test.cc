

#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow.hpp"
#include "geometry_type_traits.hpp"

namespace geoarrow::type_traits {

TEST(GeoArrowHppTest, GeometryTypeTraits) {
  using array_type = typename TypeTraits<GEOARROW_TYPE_POINT>::array_type;
  ASSERT_EQ(array_type::geometry_type, GEOARROW_GEOMETRY_TYPE_POINT);
}

}  // namespace geoarrow::type_traits
