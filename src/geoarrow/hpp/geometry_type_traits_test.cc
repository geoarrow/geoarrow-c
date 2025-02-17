

#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow.hpp"
#include "geometry_type_traits.hpp"

template <typename Array>
void DoSomethingWithArray(const Array& array, bool* was_called = nullptr) {
  EXPECT_EQ(array.value.size(), 0);
  *was_called = true;
}

TEST(GeoArrowHppTest, DispatchNativeArray) {
  bool was_called = false;
  GEOARROW_DISPATCH_NATIVE_ARRAY_CALL(GEOARROW_TYPE_POINT, DoSomethingWithArray,
                                      &was_called);
  EXPECT_TRUE(was_called);
}
