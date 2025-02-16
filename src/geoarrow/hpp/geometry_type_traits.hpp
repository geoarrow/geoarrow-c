#ifndef GEOARROW_HPP_TYPE_TRAITS_INCLUDED
#define GEOARROW_HPP_TYPE_TRAITS_INCLUDED

#include "geoarrow.h"
#include "hpp/array_util.hpp"
#include "hpp/wkb_util.hpp"

namespace geoarrow {

namespace type_traits {

namespace internal {

template <enum GeoArrowDimensions dimensions>
struct DimensionTraits;

template <>
struct DimensionTraits<GEOARROW_DIMENSIONS_XY> {
  using coord_type = array_util::XY<double>;
};

template <>
struct DimensionTraits<GEOARROW_DIMENSIONS_XYZ> {
  using coord_type = array_util::XYZ<double>;
};

template <>
struct DimensionTraits<GEOARROW_DIMENSIONS_XYM> {
  using coord_type = array_util::XYM<double>;
};

template <>
struct DimensionTraits<GEOARROW_DIMENSIONS_XYZM> {
  using coord_type = array_util::XYZM<double>;
};

template <enum GeoArrowDimensions dimensions_id>
struct ResolveArrayType {
  using coord_type = typename DimensionTraits<dimensions_id>::coord_type;

  template <enum GeoArrowGeometryType geometry_type>
  struct Inner;

  template <>
  struct Inner<GEOARROW_GEOMETRY_TYPE_POINT> {
    using array_type = array_util::PointArray<coord_type>;
  };

  template <>
  struct Inner<GEOARROW_GEOMETRY_TYPE_LINESTRING> {
    using array_type = array_util::LinestringArray<coord_type>;
  };

  template <>
  struct Inner<GEOARROW_GEOMETRY_TYPE_POLYGON> {
    using array_type = array_util::PolygonArray<coord_type>;
  };

  template <>
  struct Inner<GEOARROW_GEOMETRY_TYPE_MULTIPOINT> {
    using array_type = array_util::MultipointArray<coord_type>;
  };

  template <>
  struct Inner<GEOARROW_GEOMETRY_TYPE_MULTILINESTRING> {
    using array_type = array_util::MultiLinestringArray<coord_type>;
  };

  template <>
  struct Inner<GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON> {
    using array_type = array_util::MultiPolygonArray<coord_type>;
  };
};

}  // namespace internal

template <enum GeoArrowGeometryType geometry_type, enum GeoArrowDimensions dimensions>
struct GeometryTypeTraits {
  using coord_type = typename internal::DimensionTraits<dimensions>::coord_type;
  using sequence_type = array_util::CoordSequence<coord_type>;
  using array_type = typename internal::ResolveArrayType<dimensions>::template Inner<
      geometry_type>::array_type;

  static constexpr enum GeoArrowType type_id(enum GeoArrowCoordType coord_type =
                                                 GEOARROW_COORD_TYPE_SEPARATE){
      return static_cast<enum GeoArrowType>((coord_type - 1) * 10000 +
                                            (dimensions - 1) * 1000 + geometry_type);}

};  // namespace type_traits

template <enum GeoArrowType type>
struct TypeTraits {
  static constexpr enum GeoArrowCoordType coord_type_id =
      static_cast<enum GeoArrowCoordType>(type / 10000 + 1);
  static constexpr enum GeoArrowDimensions dimensions =
      static_cast<enum GeoArrowDimensions>((type % 10000) / 1000 + 1);
  static constexpr enum GeoArrowGeometryType geometry_type =
      static_cast<enum GeoArrowGeometryType>((type % 10000) % 1000);

  using coord_type = typename internal::DimensionTraits<dimensions>::coord_type;
  using sequence_type = array_util::CoordSequence<coord_type>;
  using array_type = typename internal::ResolveArrayType<dimensions>::template Inner<
      geometry_type>::array_type;
};

template <>
struct TypeTraits<GEOARROW_TYPE_WKT> {};

template <>
struct TypeTraits<GEOARROW_TYPE_LARGE_WKT> {};

template <>
struct TypeTraits<GEOARROW_TYPE_WKB> {
  using array_type = wkb_util::WKBArray<int32_t>;
};

template <>
struct TypeTraits<GEOARROW_TYPE_LARGE_WKB> {
  using array_type = wkb_util::WKBArray<int64_t>;
};
}  // namespace geoarrow

}  // namespace geoarrow

#endif
