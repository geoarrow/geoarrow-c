
#ifndef GEOARROW_HPP_BINARY_UTIL_INCLUDED
#define GEOARROW_HPP_BINARY_UTIL_INCLUDED

#include <vector>

#include "hpp/array_util.hpp"
#include "hpp/exception.hpp"

/// \defgroup hpp-binary-utility Binary iteration utilities
///
/// This header provides utilities for iterating over serialized GeoArrow arrays
/// (e.g., WKB).
///
/// @{

namespace geoarrow {

namespace binary_util {

namespace internal {

static constexpr uint8_t kBigEndian = 0x00;
static constexpr uint8_t kLittleEndian = 0x01;

#if defined(GEOARROW_BIG_ENDIAN)
static constexpr uint8_t kNativeEndian = kBigEndian;
static constexpr uint8_t kSwappedEndian = kLittleEndian;
#else
static constexpr uint8_t kNativeEndian = kLittleEndian;
static constexpr uint8_t kSwappedEndian = kBigEndian;

#endif

template <uint8_t endian>
struct Endian;

template <>
struct Endian<kNativeEndian> {
  using LoadUInt32 = array_util::internal::LoadIdentity<uint32_t>;
  using LoadDouble = array_util::internal::LoadIdentity<double>;
};

template <>
struct Endian<kSwappedEndian> {
  using LoadUInt32 = array_util::internal::LoadSwapped<uint32_t>;
  using LoadDouble = array_util::internal::LoadSwapped<double>;
};

}  // namespace internal

/// \brief Location and structure of a coordiante sequence within a WKB blob
struct WKBSequence {
  const uint8_t* data{};
  uint32_t size{};
  uint32_t stride{};
  enum GeoArrowDimensions dimensions {};
  uint8_t endianness;

  using NativeXYSequence = array_util::UnalignedCoordSequence<array_util::XY<double>>;
  NativeXYSequence ViewAsNativeXY() {
    NativeXYSequence seq;
    seq.InitInterleaved(size, data, stride);
    return seq;
  }

  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& f) const {
    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XY:
        VisitVerticesInternal<array_util::XY<double>, CoordDst>(f);
        break;
      case GEOARROW_DIMENSIONS_XYZ:
        VisitVerticesInternal<array_util::XYZ<double>, CoordDst>(f);
        break;
      case GEOARROW_DIMENSIONS_XYM:
        VisitVerticesInternal<array_util::XYM<double>, CoordDst>(f);
        break;
      case GEOARROW_DIMENSIONS_XYZM:
        VisitVerticesInternal<array_util::XYZM<double>, CoordDst>(f);
        break;
      default:
        throw Exception("Unknown dimensions");
    }
  }

  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& f) const {
    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XY:
        VisitEdgesInternal<array_util::XY<double>, CoordDst>(f);
        break;
      case GEOARROW_DIMENSIONS_XYZ:
        VisitEdgesInternal<array_util::XYZ<double>, CoordDst>(f);
        break;
      case GEOARROW_DIMENSIONS_XYM:
        VisitEdgesInternal<array_util::XYM<double>, CoordDst>(f);
        break;
      case GEOARROW_DIMENSIONS_XYZM:
        VisitEdgesInternal<array_util::XYZM<double>, CoordDst>(f);
        break;
      default:
        throw Exception("Unknown dimensions");
    }
  }

 private:
  template <typename CoordSrc, typename CoordDst, typename Func>
  void VisitVerticesInternal(Func&& f) const {
    if (endianness == internal::kLittleEndian) {
      using LoadDouble = internal::Endian<internal::kLittleEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(size, data, stride);
      seq.template VisitVertices<CoordDst>(f);
    } else {
      using LoadDouble = internal::Endian<internal::kBigEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(size, data, stride);
      seq.template VisitVertices<CoordDst>(f);
    }
  }

  template <typename CoordSrc, typename CoordDst, typename Func>
  void VisitEdgesInternal(Func&& f) const {
    if (endianness == internal::kLittleEndian) {
      using LoadDouble = internal::Endian<internal::kLittleEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(size, data, stride);
      seq.template VisitEdges<CoordDst>(f);
    } else {
      using LoadDouble = internal::Endian<internal::kBigEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(size, data, stride);
      seq.template VisitEdges<CoordDst>(f);
    }
  }
};

class WKBParser;

/// \brief Tokenized WKB Geometry
///
/// The result of parsing a well-known binary blob. Geometries are represented as:
///
/// - Point: A single sequence that contains exactly one point.
/// - Linestring: A single sequence.
/// - Polygon: Each ring's sequence is stored in sequences.
/// - Multipoint, Multilinestring, Multipolygon, Geometrycollection: Each child feature
//    is given its own WKBGeometry.
///
/// The motivation for this structure is to ensure that, given a single stack-allocated
/// WKBGeometry, many well-known binary blobs can be parsed and efficiently reuse the
/// heap-allocated vectors.
class WKBGeometry {
 public:
  static constexpr uint32_t kSridUnset = 0xFFFFFFFF;

  enum GeoArrowGeometryType geometry_type {};
  enum GeoArrowDimensions dimensions {};
  uint32_t srid{kSridUnset};

  const WKBSequence& Sequence(uint32_t i) const { return sequences_[i]; }

  uint32_t NumSequences() const { return num_sequences_; }

  const WKBGeometry& Geometry(uint32_t i) const { return geometries_[i]; }

  uint32_t NumGeometries() const { return num_geometries_; }

  WKBSequence* AppendSequence() {
    ++num_sequences_;
    if (sequences_.size() < num_sequences_) {
      sequences_.resize(num_sequences_);
    }
    return &sequences_.back();
  }

  WKBGeometry* AppendGeometry() {
    ++num_geometries_;
    if (geometries_.size() < num_geometries_) {
      geometries_.resize(num_geometries_);
    }
    geometries_.back().Reset();
    return &geometries_.back();
  }

  void Reset() {
    geometry_type = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
    dimensions = GEOARROW_DIMENSIONS_UNKNOWN;
    srid = kSridUnset;
    num_sequences_ = 0;
    num_geometries_ = 0;
  }

 private:
  // Vectors managed privately to ensure that all the stack-allocated memory associated
  // with the geometries is not freed when the vector is resized to zero.
  std::vector<WKBSequence> sequences_;
  uint32_t num_sequences_;
  std::vector<WKBGeometry> geometries_;
  uint32_t num_geometries_;
};

class WKBParser {
 public:
  enum Status {
    OK = 0,
    TOO_FEW_BYTES,
    TOO_MANY_BYTES,
    INVALID_ENDIAN,
    INVALID_GEOMETRY_TYPE
  };

  WKBParser() = default;

  Status Parse(struct GeoArrowBufferView data, WKBGeometry* out,
               const uint8_t** cursor = nullptr) {
    return Parse(data.data, static_cast<uint32_t>(data.size_bytes), out, cursor);
  }

  Status Parse(const uint8_t* data, uint32_t size, WKBGeometry* out,
               const uint8_t** cursor = nullptr) {
    cursor_ = data;
    remaining_ = size;
    out->Reset();
    Status status = ParseGeometry(out);
    if (cursor != nullptr) {
      *cursor = cursor_;
    }
    return status;
  }

 private:
  const uint8_t* cursor_{};
  uint32_t remaining_{};
  uint8_t last_endian_;
  enum GeoArrowDimensions last_dimensions_;
  uint32_t last_coord_stride_;
  internal::Endian<internal::kBigEndian>::LoadUInt32 load_uint32_be_;
  internal::Endian<internal::kLittleEndian>::LoadUInt32 load_uint32_le_;

  static constexpr uint32_t kEWKBZ = 0x80000000;
  static constexpr uint32_t kEWKBM = 0x40000000;
  static constexpr uint32_t kEWKBSrid = 0x20000000;
  static constexpr uint32_t kEWKBMask = 0x00FFFFFF;

  Status ParseGeometry(WKBGeometry* out) {
    Status status = CheckRemaining(sizeof(uint8_t) + sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    status = ReadEndianUnchecked();
    if (status != OK) {
      return status;
    }

    uint32_t geometry_type = ReadUInt32Unchecked();

    if (geometry_type & kEWKBSrid) {
      status = CheckRemaining(sizeof(uint32_t));
      if (status != OK) {
        return status;
      }

      out->srid = ReadUInt32Unchecked();
    }

    bool has_z = geometry_type & kEWKBZ;
    bool has_m = geometry_type & kEWKBM;
    geometry_type &= kEWKBMask;

    switch (geometry_type / 1000) {
      case 1:
        has_z = true;
        break;
      case 2:
        has_m = true;
        break;
      case 4:
        has_z = true;
        has_m = true;
        break;
    }

    last_coord_stride_ = 2 + has_z + has_m;
    if (has_z && has_m) {
      last_dimensions_ = out->dimensions = GEOARROW_DIMENSIONS_XYZM;
    } else if (has_z) {
      last_dimensions_ = out->dimensions = GEOARROW_DIMENSIONS_XYZ;
    } else if (has_m) {
      last_dimensions_ = out->dimensions = GEOARROW_DIMENSIONS_XYM;
    } else {
      last_dimensions_ = out->dimensions = GEOARROW_DIMENSIONS_XY;
    }

    geometry_type = geometry_type % 1000;
    switch (geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
        out->geometry_type = static_cast<enum GeoArrowGeometryType>(geometry_type);
        return ParsePoint(out);
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
        out->geometry_type = static_cast<enum GeoArrowGeometryType>(geometry_type);
        return ParseSequence(out);
      case GEOARROW_GEOMETRY_TYPE_POLYGON:
        out->geometry_type = static_cast<enum GeoArrowGeometryType>(geometry_type);
        return ParsePolygon(out);
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION:
        out->geometry_type = static_cast<enum GeoArrowGeometryType>(geometry_type);
        return ParseCollection(out);
      default:
        return INVALID_GEOMETRY_TYPE;
    }
  }

  Status ParsePoint(WKBGeometry* out) {
    cursor_ += NewSequenceAtCursor(out, 1);
    return OK;
  }

  Status ParseSequence(WKBGeometry* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    uint32_t size = ReadUInt32Unchecked();
    status = CheckRemaining(sizeof(double) * size * last_coord_stride_);
    if (status != OK) {
      return status;
    }

    cursor_ += NewSequenceAtCursor(out, size);
    return OK;
  }

  Status ParsePolygon(WKBGeometry* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    uint32_t size = ReadUInt32Unchecked();
    for (uint32_t i = 0; i < size; ++i) {
      status = ParseSequence(out);
      if (status != OK) {
        return status;
      }
    }

    return OK;
  }

  Status ParseCollection(WKBGeometry* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    uint32_t size = ReadUInt32Unchecked();
    for (uint32_t i = 0; i < size; ++i) {
      status = ParseGeometry(out->AppendGeometry());
      if (status != OK) {
        return status;
      }
    }

    return OK;
  }

  uint32_t NewSequenceAtCursor(WKBGeometry* out, uint32_t size) {
    WKBSequence* seq = out->AppendSequence();
    seq->size = size;
    seq->data = cursor_;
    seq->dimensions = last_dimensions_;
    seq->stride = last_coord_stride_;
    seq->endianness = last_endian_;
    return size * last_coord_stride_ * sizeof(double);
  }

  Status CheckRemaining(uint32_t bytes) {
    if (bytes <= remaining_) {
      return OK;
    } else {
      return TOO_FEW_BYTES;
    }
  }

  Status ReadEndianUnchecked() {
    last_endian_ = *cursor_;
    switch (last_endian_) {
      case internal::kLittleEndian:
      case internal::kBigEndian:
        ++cursor_;
        return OK;
      default:
        return INVALID_ENDIAN;
    }
  }

  uint32_t ReadUInt32Unchecked() {
    uint32_t out;
    if (last_endian_ == internal::kLittleEndian) {
      out = load_uint32_le_(cursor_);
    } else {
      load_uint32_be_(cursor_);
    }

    cursor_ += sizeof(uint32_t);
    return out;
  }
};

}  // namespace binary_util

}  // namespace geoarrow

/// @}

#endif
