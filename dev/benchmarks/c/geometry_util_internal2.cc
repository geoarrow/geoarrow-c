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

#include "geometry_util_internal2.h"

namespace parquet2::geometry {

class WKBBuffer {
 public:
  WKBBuffer() : data_(nullptr), size_(0) {}
  WKBBuffer(const uint8_t* data, int64_t size) : data_(data), size_(size) {}

  void Init(const uint8_t* data, int64_t size) {
    data_ = data;
    size_ = size;
  }

  ::arrow2::Result<uint8_t> ReadUInt8() { return ReadChecked<uint8_t>(); }

  ::arrow2::Result<uint32_t> ReadUInt32(bool swap) {
    ARROW2_ASSIGN_OR_RAISE(uint32_t value, ReadChecked<uint32_t>());
    if (ARROW2_PREDICT_FALSE(swap)) {
      return ByteSwap(value);
    } else {
      return value;
    }
  }

  template <typename Coord, typename Visit>
  ::arrow2::Status ReadDoubles(uint32_t n_coords, bool swap, Visit&& visit) {
    size_t total_bytes = n_coords * sizeof(Coord);
    if (size_ < total_bytes) {
      return ::arrow2::Status::SerializationError(
          "Can't coordinate sequence of ", total_bytes, " bytes from WKBBuffer with ",
          size_, " remaining");
    }

    if (ARROW2_PREDICT_FALSE(swap)) {
      Coord coord;
      for (uint32_t i = 0; i < n_coords; i++) {
        coord = ReadUnchecked<Coord>();
        for (uint32_t j = 0; j < coord.size(); j++) {
          coord[j] = ByteSwap(coord[j]);
        }

        visit(coord);
      }
    } else {
      for (uint32_t i = 0; i < n_coords; i++) {
        visit(ReadUnchecked<Coord>());
      }
    }

    return ::arrow2::Status::OK();
  }

  size_t size() { return size_; }

 private:
  const uint8_t* data_;
  size_t size_;

  template <typename T>
  ::arrow2::Result<T> ReadChecked() {
    if (size_ < sizeof(T)) {
      return ::arrow2::Status::SerializationError(
          "Can't read ", sizeof(T), " bytes from WKBBuffer with ", size_, " remaining");
    }

    return ReadUnchecked<T>();
  }

  template <typename T>
  T ReadUnchecked() {
    T out = ::arrow2::util::SafeLoadAs<T>(data_);
    data_ += sizeof(T);
    size_ -= sizeof(T);
    return out;
  }

  template <typename T>
  T ByteSwap(T value) {
    return ::arrow2::bit_util::ByteSwap(value);
  }
};

using GeometryTypeAndDimensions = std::pair<GeometryType, Dimensions>;

namespace {

::arrow2::Result<GeometryTypeAndDimensions> ParseGeometryType(
    uint32_t wkb_geometry_type) {
  // The number 1000 can be used because WKB geometry types are constructed
  // on purpose such that this relationship is true (e.g., LINESTRING ZM maps
  // to 3002).
  uint32_t geometry_type_component = wkb_geometry_type % 1000;
  uint32_t dimensions_component = wkb_geometry_type / 1000;

  auto min_geometry_type_value = static_cast<uint32_t>(GeometryType::MIN);
  auto max_geometry_type_value = static_cast<uint32_t>(GeometryType::MAX);
  auto min_dimension_value = static_cast<uint32_t>(Dimensions::MIN);
  auto max_dimension_value = static_cast<uint32_t>(Dimensions::MAX);

  if (geometry_type_component < min_geometry_type_value ||
      geometry_type_component > max_geometry_type_value ||
      dimensions_component < min_dimension_value ||
      dimensions_component > max_dimension_value) {
    return ::arrow2::Status::SerializationError("Invalid WKB geometry type: ",
                                                wkb_geometry_type);
  }

  GeometryTypeAndDimensions out{static_cast<GeometryType>(geometry_type_component),
                                static_cast<Dimensions>(dimensions_component)};
  return out;
}

}  // namespace

::arrow2::Status WKBGeometryBounder::ReadGeometry(const uint8_t* data, int64_t size) {
  WKBBuffer src{data, size};
  ARROW2_RETURN_NOT_OK(ReadGeometryInternal(&src, /*record_wkb_type=*/true));
  if (src.size() != 0) {
    return ::arrow2::Status::SerializationError(
        "Exepcted zero bytes after consuming WKB but got ", src.size());
  }

  return ::arrow2::Status::OK();
}

::arrow2::Status WKBGeometryBounder::ReadGeometryInternal(WKBBuffer* src,
                                                          bool record_wkb_type) {
  ARROW2_ASSIGN_OR_RAISE(uint8_t endian, src->ReadUInt8());
#if defined(ARROW2_LITTLE_ENDIAN)
  bool swap = endian != 0x01;
#else
  bool swap = endian != 0x00;
#endif

  ARROW2_ASSIGN_OR_RAISE(uint32_t wkb_geometry_type, src->ReadUInt32(swap));
  ARROW2_ASSIGN_OR_RAISE(GeometryTypeAndDimensions geometry_type_and_dimensions,
                         ParseGeometryType(wkb_geometry_type));

  // Keep track of geometry types encountered if at the top level
  if (record_wkb_type) {
    geospatial_types_.insert(static_cast<int32_t>(wkb_geometry_type));
  }

  switch (geometry_type_and_dimensions.first) {
    case GeometryType::POINT:
      ARROW2_RETURN_NOT_OK(
          ReadSequence(src, geometry_type_and_dimensions.second, 1, swap));
      break;

    case GeometryType::LINESTRING: {
      ARROW2_ASSIGN_OR_RAISE(uint32_t n_coords, src->ReadUInt32(swap));
      ARROW2_RETURN_NOT_OK(
          ReadSequence(src, geometry_type_and_dimensions.second, n_coords, swap));
      break;
    }
    case GeometryType::POLYGON: {
      ARROW2_ASSIGN_OR_RAISE(uint32_t n_parts, src->ReadUInt32(swap));
      for (uint32_t i = 0; i < n_parts; i++) {
        ARROW2_ASSIGN_OR_RAISE(uint32_t n_coords, src->ReadUInt32(swap));
        ARROW2_RETURN_NOT_OK(
            ReadSequence(src, geometry_type_and_dimensions.second, n_coords, swap));
      }
      break;
    }

    // These are all encoded the same in WKB, even though this encoding would
    // allow for parts to be of a different geometry type or different dimensions.
    // For the purposes of bounding, this does not cause us problems.
    case GeometryType::MULTIPOINT:
    case GeometryType::MULTILINESTRING:
    case GeometryType::MULTIPOLYGON:
    case GeometryType::GEOMETRYCOLLECTION: {
      ARROW2_ASSIGN_OR_RAISE(uint32_t n_parts, src->ReadUInt32(swap));
      for (uint32_t i = 0; i < n_parts; i++) {
        ARROW2_RETURN_NOT_OK(ReadGeometryInternal(src, /*record_wkb_type*/ false));
      }
      break;
    }
  }

  return ::arrow2::Status::OK();
}

::arrow2::Status WKBGeometryBounder::ReadSequence(WKBBuffer* src, Dimensions dimensions,
                                                  uint32_t n_coords, bool swap) {
  switch (dimensions) {
    case Dimensions::XY:
      return src->ReadDoubles<BoundingBox::XY>(
          n_coords, swap, [&](BoundingBox::XY coord) { box_.UpdateXY(coord); });
    case Dimensions::XYZ:
      return src->ReadDoubles<BoundingBox::XYZ>(
          n_coords, swap, [&](BoundingBox::XYZ coord) { box_.UpdateXYZ(coord); });
    case Dimensions::XYM:
      return src->ReadDoubles<BoundingBox::XYM>(
          n_coords, swap, [&](BoundingBox::XYM coord) { box_.UpdateXYM(coord); });
    case Dimensions::XYZM:
      return src->ReadDoubles<BoundingBox::XYZM>(
          n_coords, swap, [&](BoundingBox::XYZM coord) { box_.UpdateXYZM(coord); });
    default:
      return ::arrow2::Status::SerializationError("Unknown dimensions");
  }
}

}  // namespace parquet2::geometry
