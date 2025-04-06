
#ifndef GEOARROW_HPP_GEOMETRY_INCLUDED
#define GEOARROW_HPP_GEOMETRY_INCLUDED

#include <stdint.h>
#include <limits>
#include <string>
#include <vector>

#include "geoarrow/geoarrow_type.h"
#include "geoarrow/hpp/array_util.hpp"

namespace geoarrow {

namespace geometry {

namespace internal {

template <typename T>
T SafeLoadAs(const uint8_t* unaligned) {
  T out;
  std::memcpy(&out, unaligned, sizeof(T));
  return out;
}

template <typename T>
struct LoadIdentity {
  T operator()(const uint8_t* unaligned) const {
    return internal::SafeLoadAs<T>(unaligned);
  };
};

template <typename T>
struct LoadSwapped {
  T operator()(const uint8_t* unaligned) const {
    uint8_t swapped[sizeof(T)];
    for (size_t i = 0; i < sizeof(T); i++) {
      swapped[sizeof(T) - i - 1] = unaligned[i];
    }
    return internal::SafeLoadAs<T>(swapped);
  };
};

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
  using LoadUInt32 = LoadIdentity<uint32_t>;
  using LoadDouble = LoadIdentity<double>;
};

template <>
struct Endian<kSwappedEndian> {
  using LoadUInt32 = LoadSwapped<uint32_t>;
  using LoadDouble = LoadSwapped<double>;
};

static double kNaN = std::numeric_limits<double>::quiet_NaN();
static const uint8_t* kNaNBytes = reinterpret_cast<uint8_t*>(&internal::kNaN);

}  // namespace internal

struct GeometryNode {
  const uint8_t* data[4]{internal::kNaNBytes, internal::kNaNBytes, internal::kNaNBytes,
                         internal::kNaNBytes};
  uint32_t stride[4]{};
  uint32_t size{};
  uint8_t geometry_type{};
  uint8_t dimensions{};
  uint8_t endian{internal::kNativeEndian};
  uint8_t flags{};
  struct GeometryNode* next{};

  template <typename Visit>
  void VisitVertices(Visit&& fun) {
    switch (geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_LINESTRING: {
        array_util::XY<double> c;
        const uint8_t* ptrs[4];
        memcpy(ptrs, data, sizeof(data));
        auto load = array_util::internal::LoadIdentity<double>();

        for (uint32_t i = 0; i < size; i++) {
          for (uint32_t j = 0; j < c.size(); j++) {
            c[j] = load(ptrs[j]);
            ptrs[j] += stride[j];
          }

          fun(c);
        }
      } break;
      default:
        break;
    }

    if (next != nullptr) {
      next->VisitVertices(fun);
    }
  }
};

class Geometry {
 public:
  GeometryNode* data() { return &root_; }

  void clear() { nodes_.clear(); }

  GeometryNode* AppendNode() { return &nodes_.emplace_back(); }

  void Finish() {
    if (nodes_.empty()) {
      return;
    }

    root_.next = nodes_.data();
    for (auto& node : nodes_) {
      node.next = &node + 1;
    }
    nodes_.back().next = nullptr;
  }

 private:
  GeometryNode root_;
  std::vector<GeometryNode> nodes_;
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
  Status Parse(struct GeoArrowBufferView data, Geometry* out,
               const uint8_t** cursor = nullptr) {
    return Parse(data.data, static_cast<uint32_t>(data.size_bytes), out, cursor);
  }

  /// \brief Parse the specified bytes into out, placing the end of the sequence in
  /// focursor
  Status Parse(const uint8_t* data, size_t size, Geometry* root,
               const uint8_t** cursor = nullptr) {
    data_ = cursor_ = data;
    remaining_ = size;
    root->clear();
    Status status = ParseGeometry(root, root->data());
    if (status != OK) {
      return status;
    }

    if (cursor != nullptr) {
      *cursor = cursor_;
    }

    if (static_cast<size_t>(cursor_ - data_) < size) {
      return TOO_MANY_BYTES;
    }

    root->Finish();
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
  uint8_t last_dimensions_;
  uint32_t last_coord_stride_;
  internal::Endian<internal::kBigEndian>::LoadUInt32 load_uint32_be_;
  internal::Endian<internal::kLittleEndian>::LoadUInt32 load_uint32_le_;

  static constexpr uint32_t kEWKBZ = 0x80000000;
  static constexpr uint32_t kEWKBM = 0x40000000;
  static constexpr uint32_t kEWKBSrid = 0x20000000;
  static constexpr uint32_t kEWKBMask = 0x00FFFFFF;

  Status ParseGeometry(Geometry* root, GeometryNode* out) {
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

      ReadUInt32Unchecked();
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
      last_dimensions_ = out->dimensions = static_cast<uint8_t>(GEOARROW_DIMENSIONS_XYZM);
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
        out->geometry_type = static_cast<uint8_t>(geometry_type);
        return ParsePoint(out);
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
        out->geometry_type = static_cast<uint8_t>(geometry_type);
        return ParseSequence(out);
      case GEOARROW_GEOMETRY_TYPE_POLYGON:
        out->geometry_type = static_cast<uint8_t>(geometry_type);
        return ParsePolygon(root, out);
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION:
        out->geometry_type = static_cast<uint8_t>(geometry_type);
        return ParseCollection(root, out);
      default:
        return INVALID_GEOMETRY_TYPE;
    }
  }

  Status ParsePoint(GeometryNode* out) {
    uint32_t bytes_required = 1 * last_coord_stride_ * sizeof(double);
    Status status = CheckRemaining(bytes_required);
    if (status != OK) {
      return status;
    }

    NewSequenceAtCursor(out, 1);
    Advance(bytes_required);
    return OK;
  }

  Status ParseSequence(GeometryNode* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    out->size = ReadUInt32Unchecked();
    size_t bytes_required = sizeof(double) * out->size * last_coord_stride_;
    status = CheckRemaining(bytes_required);
    if (status != OK) {
      return status;
    }

    NewSequenceAtCursor(out, out->size);
    Advance(bytes_required);
    return OK;
  }

  Status ParsePolygon(Geometry* root, GeometryNode* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    out->size = ReadUInt32Unchecked();
    for (uint32_t i = 0; i < out->size; ++i) {
      status = ParseSequence(root->AppendNode());
      if (status != OK) {
        return status;
      }
    }

    return OK;
  }

  Status ParseCollection(Geometry* root, GeometryNode* out) {
    Status status = CheckRemaining(sizeof(uint32_t));
    if (status != OK) {
      return status;
    }

    out->size = ReadUInt32Unchecked();
    for (uint32_t i = 0; i < out->size; ++i) {
      status = ParseGeometry(root, root->AppendNode());
      if (status != OK) {
        return status;
      }
    }

    return OK;
  }

  uint32_t NewSequenceAtCursor(GeometryNode* out, uint32_t size) {
    out->size = size;
    for (uint32_t i = 0; i < last_coord_stride_; i++) {
      out->data[i] = cursor_ + (i * sizeof(double));
      out->stride[i] = last_coord_stride_ * sizeof(double);
    }
    out->dimensions = last_dimensions_;
    out->endian = last_endian_;
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

}  // namespace geometry

}  // namespace geoarrow

#endif
