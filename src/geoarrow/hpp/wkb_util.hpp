
#ifndef GEOARROW_HPP_WKB_UTIL_INCLUDED
#define GEOARROW_HPP_WKB_UTIL_INCLUDED

#include <vector>

#include "geoarrow/hpp/array_util.hpp"
#include "geoarrow/hpp/exception.hpp"

#ifndef GEOARROW_NATIVE_ENDIAN
#define GEOARROW_NATIVE_ENDIAN 0x01
#endif

/// \defgroup hpp-binary-utility Binary iteration utilities
///
/// This header provides utilities for iterating over serialized GeoArrow arrays
/// (e.g., WKB).
///
/// @{

namespace geoarrow {

namespace wkb_util {

namespace internal {

static constexpr uint8_t kBigEndian = 0x00;
static constexpr uint8_t kLittleEndian = 0x01;

#if GEOARROW_NATIVE_ENDIAN == 0x00
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

/// \brief Location and structure of a coordinate sequence within a WKB blob
struct WKBSequence {
  const uint8_t* data{};
  uint32_t length{};
  uint32_t stride{};
  enum GeoArrowDimensions dimensions {};
  uint8_t endianness;

  /// \brief The number of coordinates in the sequence
  uint32_t size() const { return length; }

  using NativeXYSequence = array_util::UnalignedCoordSequence<array_util::XY<double>>;

  /// \brief For native-endian sequences, return an XY sequence directly
  ///
  /// This may be faster or more convenient than using a visitor-based approach
  /// when only the XY dimensions are needed.
  NativeXYSequence ViewAsNativeXY() const {
    NativeXYSequence seq;
    seq.InitInterleaved(length, data, stride);
    return seq;
  }

  /// \brief Call func once for each vertex in this sequence
  ///
  /// This function is templated on the desired coordinate output type.
  /// This allows, for example, iteration along all XYZM dimensions of an
  /// arbitrary sequence, even if some of those dimensions don't exist
  /// in the sequence. Similarly, one can iterate over fewer dimensions than
  /// are strictly in the output (discarding dimensions not of interest).
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

  /// \brief Call func once for each sequential pair of vertices in this sequence
  ///
  /// This function is templated on the desired coordinate output type, performing
  /// the same coordinate conversion as VisitVertices. Note that sequential vertices
  /// may not be meaningful as edges for some types of sequences.
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
      seq.InitInterleaved(length, data, stride);
      seq.template VisitVertices<CoordDst>(f);
    } else {
      using LoadDouble = internal::Endian<internal::kBigEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(length, data, stride);
      seq.template VisitVertices<CoordDst>(f);
    }
  }

  template <typename CoordSrc, typename CoordDst, typename Func>
  void VisitEdgesInternal(Func&& f) const {
    if (endianness == internal::kLittleEndian) {
      using LoadDouble = internal::Endian<internal::kLittleEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(length, data, stride);
      seq.template VisitEdges<CoordDst>(f);
    } else {
      using LoadDouble = internal::Endian<internal::kBigEndian>::LoadDouble;
      using Sequence = array_util::UnalignedCoordSequence<CoordSrc, LoadDouble>;
      Sequence seq;
      seq.InitInterleaved(length, data, stride);
      seq.template VisitEdges<CoordDst>(f);
    }
  }
};

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
/// heap-allocated vectors. Because of this, care is taken to private manage the
/// storage for WKBSequence and child WKBGeometry objects to ensure we don't call any
/// child deleters when a Reset() is requested and a new geometry is to be parsed.
///
/// This class can also be used to represent serialized geometries that are not WKB
/// but follow the same structure (e.g., gserialized, DuckDB GEOMETRY).
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

  /// \brief Call func once for each vertex in this geometry
  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    for (uint32_t i = 0; i < NumSequences(); i++) {
      Sequence(i).VisitVertices<CoordDst>(func);
    }

    for (uint32_t i = 0; i < NumGeometries(); i++) {
      Geometry(i).VisitVertices<CoordDst>(func);
    }
  }

  /// \brief Call func once for each edge in this geometry
  ///
  /// For the purposes of this function, points are considered degenerate edges.
  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    switch (geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
        VisitVertices<CoordDst>([&](CoordDst v0) { func(v0, v0); });
        return;
      default:
        for (uint32_t i = 0; i < NumSequences(); i++) {
          Sequence(i).VisitEdges<CoordDst>(func);
        }
    }

    for (uint32_t i = 0; i < NumGeometries(); i++) {
      Geometry(i).VisitEdges<CoordDst>(func);
    }
  }

  /// \brief Append a new sequence to this WKBGeometry and return a pointer to it
  WKBSequence* AppendSequence() {
    ++num_sequences_;
    if (sequences_.size() < num_sequences_) {
      sequences_.push_back({});
    }
    return &sequences_[num_sequences_ - 1];
  }

  WKBGeometry* AppendGeometry() {
    ++num_geometries_;
    if (geometries_.size() < num_geometries_) {
      geometries_.push_back({});
    }
    return &geometries_[num_geometries_ - 1];
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

/// \brief Parse Well-known binary blobs
class WKBParser {
 public:
  /// \brief Outcomes of parsing a WKB blob
  enum Status {
    /// \brief Success!
    OK = 0,
    /// \brief The provided size was not sufficient to complete parsing
    TOO_FEW_BYTES,
    /// \brief Parse was successful, but the provided size was larger than the parsed WKB
    TOO_MANY_BYTES,
    /// \brief An endian value other than 0x00 or 0x01 was encountered (e.g., corrupted
    /// data)
    INVALID_ENDIAN,
    /// \brief An unexpected geometry type value was encountered (e.g., corrupted data or
    /// curved/complex geometry)
    INVALID_GEOMETRY_TYPE
  };

  WKBParser() = default;

  /// \brief Parse a GeoArrowBufferView into out, placing the end of the sequence in
  /// cursor
  Status Parse(struct GeoArrowBufferView data, WKBGeometry* out,
               const uint8_t** cursor = nullptr) {
    return Parse(data.data, static_cast<uint32_t>(data.size_bytes), out, cursor);
  }

  /// \brief Parse the specified bytes into out, placing the end of the sequence in
  /// focursor
  Status Parse(const uint8_t* data, size_t size, WKBGeometry* out,
               const uint8_t** cursor = nullptr) {
    data_ = cursor_ = data;
    remaining_ = size;
    Status status = ParseGeometry(out);
    if (status != OK) {
      return status;
    }

    if (cursor != nullptr) {
      *cursor = cursor_;
    }

    if (static_cast<size_t>(cursor_ - data_) < size) {
      return TOO_MANY_BYTES;
    }

    return OK;
  }

  std::string ErrorToString(Status status) {
    switch (status) {
      case OK:
        return "OK";
      case TOO_FEW_BYTES:
        return "TOO_FEW_BYTES";
      case TOO_MANY_BYTES:
        return "TOO_MANY_BYTES";
      case INVALID_ENDIAN:
        return "INVALID_ENDIAN";
      case INVALID_GEOMETRY_TYPE:
        return "INVALID_GEOMETRY_TYPE";
      default:
        return "UNKNOWN";
    }
  }

 private:
  const uint8_t* data_{};
  const uint8_t* cursor_{};
  size_t remaining_{};
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
    out->Reset();

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
      case 3:
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
    uint32_t bytes_required = 1 * last_coord_stride_ * sizeof(double);
    Status status = CheckRemaining(bytes_required);
    if (status != OK) {
      return status;
    }

    NewSequenceAtCursor(out, 1);
    Advance(bytes_required);
    return OK;
  }

  Status ParseSequence(WKBGeometry* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    uint32_t size = ReadUInt32Unchecked();
    size_t bytes_required = sizeof(double) * size * last_coord_stride_;
    status = CheckRemaining(bytes_required);
    if (status != OK) {
      return status;
    }

    NewSequenceAtCursor(out, size);
    Advance(bytes_required);
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
    seq->length = size;
    seq->data = cursor_;
    seq->dimensions = last_dimensions_;
    seq->stride = last_coord_stride_;
    seq->endianness = last_endian_;
    return size * last_coord_stride_ * sizeof(double);
  }

  Status CheckRemaining(size_t bytes) {
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
        Advance(sizeof(uint8_t));
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
      out = load_uint32_be_(cursor_);
    }

    Advance(sizeof(uint32_t));
    return out;
  }

  void Advance(size_t bytes) {
    cursor_ += bytes;
    remaining_ -= bytes;
  }
};

namespace internal {

template <typename WKBSequence>
class WKBSequenceIterator
    : public array_util::internal::BaseRandomAccessIterator<WKBSequence> {
 public:
  explicit WKBSequenceIterator(const WKBSequence& outer, int64_t i)
      : array_util::internal::BaseRandomAccessIterator<WKBSequence>(outer, i) {}

  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = const WKBGeometry&;

  value_type operator*() {
    WKBParser::Status status = parser_.Parse(this->outer_.blob(this->i_), &stashed_);
    if (status != WKBParser::OK) {
      throw Exception("Failed to parse WKB at index " + std::to_string(this->i_) +
                      " with error " + parser_.ErrorToString(status));
    }

    return stashed_;
  }

 private:
  WKBGeometry stashed_;
  WKBParser parser_;
};

}  // namespace internal

template <typename Offset>
struct WKBBlobSequence : public array_util::BinarySequence<Offset> {
  using value_type = const WKBGeometry&;

  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    for (const auto& item : *this) {
      item.template VisitVertices<CoordDst>(func);
    }
  }

  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    for (const auto& item : *this) {
      item.template VisitEdges<CoordDst>(func);
    }
  }

  using const_iterator = internal::WKBSequenceIterator<WKBBlobSequence>;
  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, this->length); }
};

/// \brief An Array of WKB blobs
template <typename Offset>
struct WKBArray : public array_util::Array<WKBBlobSequence<Offset>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_UNKNOWN;

  /// \brief Return a new array that is a subset of this one
  ///
  /// Caller is responsible for ensuring that offset + length is within the bounds
  /// of this array.
  WKBArray Slice(int64_t offset, int64_t length) {
    return this->template SliceImpl<WKBArray>(*this, offset, length);
  }
};

}  // namespace wkb_util

}  // namespace geoarrow

/// @}

#endif
