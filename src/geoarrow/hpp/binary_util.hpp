
#ifndef GEOARROW_HPP_BINARY_UTIL_INCLUDED
#define GEOARROW_HPP_BINARY_UTIL_INCLUDED

#include <vector>

#include "hpp/array_util.hpp"

/// \defgroup hpp-binary-utility Binary iteration utilities
///
/// This header provides utilities for iterating over serialized GeoArrow arrays
/// (e.g., WKB).
///
/// @{

namespace geoarrow {

namespace binary_util {

namespace wkb {

static constexpr uint8_t kBigEndian = 0x00;
static constexpr uint8_t kLittleEndian = 0x01;

struct NativeEndian {
  using LoadUInt32 = array_util::internal::LoadIdentity<uint32_t>;
  using LoadDouble = array_util::internal::LoadIdentity<double>;
  using SequenceXY = array_util::UnalignedCoordSequence<array_util::XY<double>>;
  using SequenceXYZ = array_util::UnalignedCoordSequence<array_util::XYZ<double>>;
  using SequenceXYM = array_util::UnalignedCoordSequence<array_util::XYM<double>>;
  using SequenceXYZM = array_util::UnalignedCoordSequence<array_util::XYZM<double>>;
};

struct SwappedEndian {
  using LoadUInt32 = array_util::internal::LoadIdentity<uint32_t>;
  using LoadDouble = array_util::internal::LoadSwapped<double>;
  using SequenceXY =
      array_util::UnalignedCoordSequence<array_util::XY<double>, LoadDouble>;
  using SequenceXYZ =
      array_util::UnalignedCoordSequence<array_util::XYZ<double>, LoadDouble>;
  using SequenceXYM =
      array_util::UnalignedCoordSequence<array_util::XYM<double>, LoadDouble>;
  using SequenceXYZM =
      array_util::UnalignedCoordSequence<array_util::XYZM<double>, LoadDouble>;
};

/// \brief Tokenized WKB Geometry
///
/// The result of parsing a well-known binary blob. Geometries are represented as:
///
/// - Point: A single sequence that contains exactly one point.
/// - Linestring: A single sequence.
/// - Polygon: Each ring's sequence is stored in sequences.
/// - Multipoint: A single sequence. This works because (valid) multipoints
///   have child geometries whose coordinates are equally spaced (althoug not
///   contiguous).
/// - Multilinestring: Each child linestring's sequence is stored in sequences.
/// - Multipolygon: Each ring's sequence is stored in sequences, with the starting
///   index of each child polygon stored in multipolygons.
/// - Geometrycollection: Each child feature is given its own WKBGeometry.
///
/// The motivation for this structure is to ensure that, given a single stack-allocated
/// WKBGeometry, many well-known binary blobs can be parsed and efficiently reuse the
/// heap-allocated vectors (except for geometrycollections, whose efficient parsing is
/// not usually a motivator).
///
/// Several optimizations here do restrict the WKB that can be represented. First,
/// all WKB within a feature most have the same endian. Second,
/// multi-geometries and collections MUST have child geometries whose dimensionality
/// matches the parent. This is needed to ensure that (1) multipoint coordinates
/// are evenly spaced (a requirement of a Sequence) and (2) the same sequence template
/// applies equally to all child sequences. If handling non-standard WKB is a requirement
/// clients will have to use the C runtime to parse it into a native representation.
template <typename Sequence>
struct WKBGeometry {
  using coord_type = typename Sequence::value_type;
  enum GeoArrowGeometryType geometry_type;
  enum GeoArrowDimensions dimensions;
  std::vector<Sequence> sequences;
  std::vector<uint32_t> multipolygons;
  std::vector<WKBGeometry> geometrycollection;
};

using GeometryXY = WKBGeometry<NativeEndian::SequenceXY>;
using GeometryXYZ = WKBGeometry<NativeEndian::SequenceXYZ>;
using GeometryXYM = WKBGeometry<NativeEndian::SequenceXYM>;
using GeometryXYZM = WKBGeometry<NativeEndian::SequenceXYZM>;

using GeometrySwappedXY = WKBGeometry<SwappedEndian::SequenceXY>;
using GeometrySwappedXYZ = WKBGeometry<SwappedEndian::SequenceXYZ>;
using GeometrySwappedXYM = WKBGeometry<SwappedEndian::SequenceXYM>;
using GeometrySwappedXYZM = WKBGeometry<SwappedEndian::SequenceXYZM>;

}  // namespace wkb

}  // namespace binary_util

}  // namespace geoarrow

/// @}

#endif
