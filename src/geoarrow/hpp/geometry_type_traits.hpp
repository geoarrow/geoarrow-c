#ifndef GEOARROW_HPP_TYPE_TRAITS_INCLUDED
#define GEOARROW_HPP_TYPE_TRAITS_INCLUDED

#include "geoarrow/hpp/array_util.hpp"
#include "geoarrow/hpp/wkb_util.hpp"

namespace geoarrow {

namespace type_traits {

namespace internal {

/// \brief A template to resolve the coordinate type based on a dimensions constant
///
/// This may change in the future if more coordinate storage options are added
/// (e.g., float) or if the coordinate sequence is updated such that the Coord level
/// of templating is no longer needed.
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

template <enum GeoArrowGeometryType geometry_type, enum GeoArrowDimensions dimensions>
struct ResolveArrayType;

// GCC Won't compile template specializations within a template, so we stamp this
// out using a macro (there may be a better way).
#define _GEOARROW_SPECIALIZE_ARRAY_TYPE(geometry_type, dimensions, array_cls) \
  template <>                                                                 \
  struct ResolveArrayType<geometry_type, dimensions> {                        \
    using coord_type = typename DimensionTraits<dimensions>::coord_type;      \
    using array_type = array_util::array_cls<coord_type>;                     \
  }

#define _GEOARROW_SPECIALIZE_GEOMETRY_TYPE(geometry_type, array_cls)                  \
  _GEOARROW_SPECIALIZE_ARRAY_TYPE(geometry_type, GEOARROW_DIMENSIONS_XY, array_cls);  \
  _GEOARROW_SPECIALIZE_ARRAY_TYPE(geometry_type, GEOARROW_DIMENSIONS_XYZ, array_cls); \
  _GEOARROW_SPECIALIZE_ARRAY_TYPE(geometry_type, GEOARROW_DIMENSIONS_XYM, array_cls); \
  _GEOARROW_SPECIALIZE_ARRAY_TYPE(geometry_type, GEOARROW_DIMENSIONS_XYZM, array_cls);

_GEOARROW_SPECIALIZE_GEOMETRY_TYPE(GEOARROW_GEOMETRY_TYPE_POINT, PointArray);
_GEOARROW_SPECIALIZE_GEOMETRY_TYPE(GEOARROW_GEOMETRY_TYPE_LINESTRING, LinestringArray);
_GEOARROW_SPECIALIZE_GEOMETRY_TYPE(GEOARROW_GEOMETRY_TYPE_POLYGON, PolygonArray);
_GEOARROW_SPECIALIZE_GEOMETRY_TYPE(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, MultipointArray);
_GEOARROW_SPECIALIZE_GEOMETRY_TYPE(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                                   MultiLinestringArray);
_GEOARROW_SPECIALIZE_GEOMETRY_TYPE(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                                   MultiPolygonArray);

#undef _GEOARROW_SPECIALIZE_GEOMETRY_TYPE
#undef _GEOARROW_SPECIALIZE_ARRAY_TYPE

}  // namespace internal

/// \brief Resolve compile-time type definitions and constants from a geometry type and
/// dimension identifier.
template <enum GeoArrowGeometryType geometry_type, enum GeoArrowDimensions dimensions>
struct GeometryTypeTraits {
  using coord_type = typename internal::DimensionTraits<dimensions>::coord_type;
  using sequence_type = array_util::CoordSequence<coord_type>;
  using array_type =
      typename internal::ResolveArrayType<geometry_type, dimensions>::array_type;

  static constexpr enum GeoArrowType type_id(enum GeoArrowCoordType coord_type =
                                                 GEOARROW_COORD_TYPE_SEPARATE){
      return static_cast<enum GeoArrowType>((coord_type - 1) * 10000 +
                                            (dimensions - 1) * 1000 + geometry_type);}

};  // namespace type_traits

/// \brief Resolve compile-time type definitions and constants from a type identifier
template <enum GeoArrowType type>
struct TypeTraits {
  static constexpr enum GeoArrowCoordType coord_type_id =
      static_cast<enum GeoArrowCoordType>(type / 10000 + 1);
  static constexpr enum GeoArrowDimensions dimensions =
      static_cast<enum GeoArrowDimensions>((type % 10000) / 1000 + 1);
  static constexpr enum GeoArrowGeometryType geometry_type =
      static_cast<enum GeoArrowGeometryType>((type % 10000) % 1000);

  using coord_type = typename internal::DimensionTraits<dimensions>::coord_type;
  using array_type =
      typename internal::ResolveArrayType<geometry_type, dimensions>::array_type;
};

template <>
struct TypeTraits<GEOARROW_TYPE_WKT> {};

template <>
struct TypeTraits<GEOARROW_TYPE_LARGE_WKT> {};

template <>
struct TypeTraits<GEOARROW_TYPE_WKB> {
  using array_type = wkb_util::WKBArray<int32_t>;
  using coord_type = array_util::XYZM<double>;
};

template <>
struct TypeTraits<GEOARROW_TYPE_LARGE_WKB> {
  using array_type = wkb_util::WKBArray<int64_t>;
  using coord_type = array_util::XYZM<double>;
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

/// \brief Dispatch a call to a function accepting a native Array specialization
///
/// This allows writing generic code against any Array type in the form
/// `template <typename Array> DoSomething(Array& array, ...) { ... }`, which can be
/// dispatched using `GEOARROW_DISPATCH_NATIVE_ARRAY_CALL(type_id, DoSomething, ...);`.
/// Currently requires that `DoSomething` handles all native array types and accepts
/// a parameter other than array (e.g., a `GeoArrowArrayView`).
///
/// This version only dispatches to "native" array types. Use
/// `GEOARROW_DISPATCH_ARRAY_CALL` to also dispatch serialized types.
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

/// \brief Dispatch a call to a function accepting a native or serialized Array
/// specialization
///
/// Identical to `GEOARROW_DISPATCH_NATIVE_ARRAY_CALL()`, but also handles dispatching to
/// WKB arrays. WKB arrays might not support the same operations or have the same syntax
/// and may need to be dispatched separately for anything that can't use VisitEdges()
/// or VisitVertices().
#define GEOARROW_DISPATCH_ARRAY_CALL(type_id, expr, ...)                                 \
  do {                                                                                   \
    switch (type_id) {                                                                   \
      case GEOARROW_TYPE_WKB: {                                                          \
        using array_type_internal =                                                      \
            typename ::geoarrow::type_traits::TypeTraits<GEOARROW_TYPE_WKB>::array_type; \
        array_type_internal array_instance_internal;                                     \
        expr(array_instance_internal, __VA_ARGS__);                                      \
        break;                                                                           \
      }                                                                                  \
      case GEOARROW_TYPE_LARGE_WKB:                                                      \
      case GEOARROW_TYPE_WKT:                                                            \
      case GEOARROW_TYPE_LARGE_WKT:                                                      \
        throw ::geoarrow::Exception("WKT/Large WKB not handled by generic dispatch");    \
      default:                                                                           \
        GEOARROW_DISPATCH_NATIVE_ARRAY_CALL(type_id, expr, __VA_ARGS__);                 \
    }                                                                                    \
  } while (false)

#endif
