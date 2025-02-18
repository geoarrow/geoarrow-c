// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_set>

namespace arrow2::util {
template <typename T>
T SafeLoadAs(const uint8_t* unaligned) {
  T out;
  std::memcpy(&out, unaligned, sizeof(T));
  return out;
}
}  // namespace arrow2::util

namespace arrow2::bit_util {
static inline uint32_t ByteSwap(uint32_t value) {
  throw std::runtime_error("we don't benchmark byte swapping here");
}
static inline double ByteSwap(double value) {
  throw std::runtime_error("we don't benchmark byte swapping here");
}
}  // namespace arrow2::bit_util

namespace arrow2 {
class Status {
 public:
  Status(bool ok) : ok_(ok) {}
  // Status(const Status&) = default;

  bool ok() const { return ok_; }

  static Status OK() { return Status(true); }

  template <typename... Args>
  static Status SerializationError(Args&&... args) {
    throw std::runtime_error("should not happen");
    return Status(false);
  }

 private:
  bool ok_{};
};

template <typename T>
class Result {
 public:
  Result(Status status) : status_(status) {}
  Result(T value) : status_(true), value_(value) {}

  bool ok() const { return status_.ok(); }

  Status status() { return status_; }

  const T& value() const { return value_; }

 private:
  Status status_;
  T value_{};
};

#define ARROW2_RETURN_NOT_OK(expr) \
  do {                            \
    ::arrow2::Status s = (expr);   \
    if (!s.ok()) return s;        \
  } while (false)

#define ARROW2_ASSIGN_OR_RAISE(lhs, expr)            \
  lhs;                                              \
  do {                                              \
    auto maybe_lhs = (expr);                        \
    if (!maybe_lhs.ok()) return maybe_lhs.status(); \
    lhs = maybe_lhs.value();                        \
  } while (false)

}  // namespace arrow

#define ARROW2_LITTLE_ENDIAN
#define ARROW2_PREDICT_FALSE(x) (__builtin_expect(!!(x), 0))
#define ARROW2_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))

namespace parquet2::geometry {

/// \brief Infinity, used to define bounds of empty bounding boxes
constexpr double kInf = std::numeric_limits<double>::infinity();

/// \brief Valid combinations of dimensions allowed by ISO well-known binary
enum class Dimensions { XY = 0, XYZ = 1, XYM = 2, XYZM = 3, MIN = 0, MAX = 3 };

/// \brief The supported set of geometry types allowed by ISO well-known binary
enum class GeometryType {
  POINT = 1,
  LINESTRING = 2,
  POLYGON = 3,
  MULTIPOINT = 4,
  MULTILINESTRING = 5,
  MULTIPOLYGON = 6,
  GEOMETRYCOLLECTION = 7,
  MIN = 1,
  MAX = 7
};

/// \brief A collection of intervals representing the encountered ranges of values
/// in each dimension.
struct BoundingBox {
  using XY = std::array<double, 2>;
  using XYZ = std::array<double, 3>;
  using XYM = std::array<double, 3>;
  using XYZM = std::array<double, 4>;

  BoundingBox(const XYZM& mins, const XYZM& maxes) : min(mins), max(maxes) {}
  BoundingBox() : min{kInf, kInf, kInf, kInf}, max{-kInf, -kInf, -kInf, -kInf} {}

  BoundingBox(const BoundingBox& other) = default;
  BoundingBox& operator=(const BoundingBox&) = default;

  void UpdateXY(const XY& coord) { UpdateInternal(coord); }

  void UpdateXYZ(const XYZ& coord) { UpdateInternal(coord); }

  void UpdateXYM(const XYM& coord) {
    min[0] = std::min(min[0], coord[0]);
    min[1] = std::min(min[1], coord[1]);
    min[3] = std::min(min[3], coord[2]);
    max[0] = std::max(max[0], coord[0]);
    max[1] = std::max(max[1], coord[1]);
    max[3] = std::max(max[3], coord[2]);
  }

  void UpdateXYZM(const XYZM& coord) { UpdateInternal(coord); }

  void Reset() {
    for (int i = 0; i < 4; i++) {
      min[i] = kInf;
      max[i] = -kInf;
    }
  }

  void Merge(const BoundingBox& other) {
    for (int i = 0; i < 4; i++) {
      min[i] = std::min(min[i], other.min[i]);
      max[i] = std::max(max[i], other.max[i]);
    }
  }

  std::string ToString() const {
    std::stringstream ss;
    ss << "BoundingBox [" << min[0] << " => " << max[0];
    for (int i = 1; i < 4; i++) {
      ss << ", " << min[i] << " => " << max[i];
    }

    ss << "]";

    return ss.str();
  }

  XYZM min;
  XYZM max;

 private:
  // This works for XY, XYZ, and XYZM
  template <typename Coord>
  void UpdateInternal(Coord coord) {
    static_assert(coord.size() <= 4);

    for (size_t i = 0; i < coord.size(); i++) {
      min[i] = std::min(min[i], coord[i]);
      max[i] = std::max(max[i], coord[i]);
    }
  }
};

inline bool operator==(const BoundingBox& lhs, const BoundingBox& rhs) {
  return lhs.min == rhs.min && lhs.max == rhs.max;
}

class WKBBuffer;

/// \brief TODO: document class
class WKBGeometryBounder {
 public:
  WKBGeometryBounder() = default;
  WKBGeometryBounder(const WKBGeometryBounder&) = default;

  ::arrow2::Status ReadGeometry(const uint8_t* data, int64_t size);

  void ReadBox(const BoundingBox& box) { box_.Merge(box); }

  void ReadGeometryTypes(const std::vector<int32_t>& geospatial_types) {
    geospatial_types_.insert(geospatial_types.begin(), geospatial_types.end());
  }

  const BoundingBox& Bounds() const { return box_; }

  std::vector<int32_t> GeometryTypes() const {
    std::vector<int32_t> out(geospatial_types_.begin(), geospatial_types_.end());
    std::sort(out.begin(), out.end());
    return out;
  }

  void Reset() {
    box_.Reset();
    geospatial_types_.clear();
  }

 private:
  BoundingBox box_;
  std::unordered_set<int32_t> geospatial_types_;

  ::arrow2::Status ReadGeometryInternal(WKBBuffer* src, bool record_wkb_type);

  ::arrow2::Status ReadSequence(WKBBuffer* src, Dimensions dimensions, uint32_t n_coords,
                               bool swap);
};

}  // namespace parquet::geometry
