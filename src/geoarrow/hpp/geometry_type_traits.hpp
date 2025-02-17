#ifndef GEOARROW_HPP_TYPE_TRAITS_INCLUDED
#define GEOARROW_HPP_TYPE_TRAITS_INCLUDED

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

#define _GEOARROW_NATIVE_ARRAY_CALL_INTERNAL(geometry_type, dimensions, expr, ...)    \
  do {                                                                                \
    using array_type_internal =                                                       \
        typename ::geoarrow::type_traits::GeometryTypeTraits<geometry_type,           \
                                                             dimensions>::array_type; \
    array_type_internal array_instance_internal;                                      \
    expr(array_instance_internal, __VA_ARGS__);                                       \
  } while (false)

#define _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(geometry_type, dimensions, expr, \
                                                        ...)                             \
  do {                                                                                   \
    switch (dimensions) {                                                                \
      case GEOARROW_DIMENSIONS_XY:                                                       \
        _GEOARROW_NATIVE_ARRAY_CALL_INTERNAL(geometry_type, GEOARROW_DIMENSIONS_XY,      \
                                             expr, __VA_ARGS__);                         \
        break;                                                                           \
      case GEOARROW_DIMENSIONS_XYZ:                                                      \
        _GEOARROW_NATIVE_ARRAY_CALL_INTERNAL(geometry_type, GEOARROW_DIMENSIONS_XYZ,     \
                                             expr, __VA_ARGS__);                         \
        break;                                                                           \
      case GEOARROW_DIMENSIONS_XYM:                                                      \
        _GEOARROW_NATIVE_ARRAY_CALL_INTERNAL(geometry_type, GEOARROW_DIMENSIONS_XYM,     \
                                             expr, __VA_ARGS__);                         \
        break;                                                                           \
      case GEOARROW_DIMENSIONS_XYZM:                                                     \
        _GEOARROW_NATIVE_ARRAY_CALL_INTERNAL(geometry_type, GEOARROW_DIMENSIONS_XYZM,    \
                                             expr, __VA_ARGS__);                         \
        break;                                                                           \
      default:                                                                           \
        throw ::geoarrow::Exception("Unknown dimension type");                           \
    }                                                                                    \
  } while (false)

#define GEOARROW_DISPATCH_NATIVE_ARRAY_CALL(type_id, expr, ...)                         \
  do {                                                                                  \
    auto geometry_type_internal = GeoArrowGeometryTypeFromType(type_id);                \
    auto dimensions_internal = GeoArrowDimensionsFromType(type_id);                     \
    switch (geometry_type_internal) {                                                   \
      case GEOARROW_GEOMETRY_TYPE_POINT:                                                \
        _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(                                \
            GEOARROW_GEOMETRY_TYPE_POINT, dimensions_internal, expr, __VA_ARGS__);      \
        break;                                                                          \
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:                                           \
        _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(                                \
            GEOARROW_GEOMETRY_TYPE_LINESTRING, dimensions_internal, expr, __VA_ARGS__); \
        break;                                                                          \
      case GEOARROW_GEOMETRY_TYPE_POLYGON:                                              \
        _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(                                \
            GEOARROW_GEOMETRY_TYPE_POLYGON, dimensions_internal, expr, __VA_ARGS__);    \
        break;                                                                          \
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:                                           \
        _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(                                \
            GEOARROW_GEOMETRY_TYPE_MULTIPOINT, dimensions_internal, expr, __VA_ARGS__); \
        break;                                                                          \
      case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:                                      \
        _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(                                \
            GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, dimensions_internal, expr,          \
            __VA_ARGS__);                                                               \
        break;                                                                          \
      case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:                                         \
        _GEOARROW_DISPATCH_NATIVE_ARRAY_CALL_DIMENSIONS(                                \
            GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, dimensions_internal, expr,             \
            __VA_ARGS__);                                                               \
        break;                                                                          \
      default:                                                                          \
        throw ::geoarrow::Exception("Unknown geometry type");                           \
    }                                                                                   \
  } while (false)

#endif
