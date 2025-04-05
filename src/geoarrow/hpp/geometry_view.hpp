
#ifndef GEOARROW_HPP_GEOMETRY_VIEW_INCLUDED
#define GEOARROW_HPP_GEOMETRY_VIEW_INCLUDED

#include <array>
#include <iterator>
#include <limits>
#include <type_traits>

#include "geoarrow/geoarrow_type.h"

/// \defgroup hpp-array-utility Array iteration utilities
///
/// This header provides utilities for iterating over native GeoArrow arrays
/// (i.e., nested lists of coordinates where the coordinates are contiguous).
/// This header only depends on geoarrow_type.h (i.e., it does not use any symbols
/// from the geoarrow-c runtime) and most logic is written such that it the
/// dependency on the types could be removed.
///
/// @{

namespace geoarrow {

namespace array_util {

namespace internal {

// The verbose bits of a random access iterator for simple outer + index-based
// iteration. Requires that an implementation defineds value_type operator[]
// and value_type operator*.
template <typename T>
T SafeLoadAs(const uint8_t* unaligned) {
  T out;
  std::memcpy(&out, unaligned, sizeof(T));
  return out;
}

template <typename Outer>
class BaseRandomAccessIterator {
 public:
  explicit BaseRandomAccessIterator(const Outer& outer, int64_t i)
      : outer_(outer), i_(i) {}
  BaseRandomAccessIterator& operator++() {
    i_++;
    return *this;
  }
  BaseRandomAccessIterator& operator--() {
    i_--;
    return *this;
  }
  BaseRandomAccessIterator& operator+=(int64_t n) {
    i_ += n;
    return *this;
  }
  BaseRandomAccessIterator& operator-=(int64_t n) {
    i_ -= n;
    return *this;
  }
  int64_t operator-(const BaseRandomAccessIterator& other) const { return i_ - other.i_; }
  bool operator<(const BaseRandomAccessIterator& other) const { return i_ < other.i_; }
  bool operator>(const BaseRandomAccessIterator& other) const { return i_ > other.i_; }
  bool operator<=(const BaseRandomAccessIterator& other) const { return i_ <= other.i_; }
  bool operator>=(const BaseRandomAccessIterator& other) const { return i_ >= other.i_; }
  bool operator==(const BaseRandomAccessIterator& other) const { return i_ == other.i_; }
  bool operator!=(const BaseRandomAccessIterator& other) const { return i_ != other.i_; }

 protected:
  const Outer& outer_;
  int64_t i_;
};

/// \brief Iterator implementation for CoordSequence
template <typename CoordSequence>
class CoordSequenceIterator : public BaseRandomAccessIterator<CoordSequence> {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = typename CoordSequence::value_type;
  using reference = value_type&;
  using pointer = value_type*;

  explicit CoordSequenceIterator(const CoordSequence& outer, int64_t i)
      : BaseRandomAccessIterator<CoordSequence>(outer, i) {}
  value_type operator*() const { return this->outer_.coord(this->i_); }
  value_type operator[](int64_t i) const { return this->outer_.coord(this->i_ + i); }
};

// Iterator for dimension begin/end
template <typename T>
class StridedIterator {
 public:
  explicit StridedIterator(const T* ptr, int64_t stride) : ptr_(ptr), stride_(stride) {}
  StridedIterator& operator++() {
    ptr_ += stride_;
    return *this;
  }
  T operator++(int) {
    T retval = *ptr_;
    ptr_ += stride_;
    return retval;
  }
  StridedIterator& operator--() {
    ptr_ -= stride_;
    return *this;
  }
  StridedIterator& operator+=(int64_t n) {
    ptr_ += (n * stride_);
    return *this;
  }
  StridedIterator& operator-=(int64_t n) {
    ptr_ -= (n * stride_);
    return *this;
  }
  int64_t operator-(const StridedIterator& other) const {
    return (ptr_ - other.ptr_) / stride_;
  }
  bool operator<(const StridedIterator& other) const { return ptr_ < other.ptr_; }
  bool operator>(const StridedIterator& other) const { return ptr_ > other.ptr_; }
  bool operator<=(const StridedIterator& other) const { return ptr_ <= other.ptr_; }
  bool operator>=(const StridedIterator& other) const { return ptr_ >= other.ptr_; }
  bool operator==(const StridedIterator& other) const { return ptr_ == other.ptr_; }
  bool operator!=(const StridedIterator& other) const { return ptr_ != other.ptr_; }

  T operator*() const { return *ptr_; }
  T operator[](ptrdiff_t i) const { return ptr_[i]; }

  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = T;
  using reference = T&;
  using pointer = T*;

 protected:
  const T* ptr_;
  ptrdiff_t stride_;
};

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

template <typename T, typename Load>
class UnalignedStridedIterator {
 public:
  explicit UnalignedStridedIterator(const uint8_t* ptr, int64_t stride)
      : ptr_(ptr), stride_(stride) {}
  UnalignedStridedIterator& operator++() {
    ptr_ += stride_;
    return *this;
  }
  T operator++(int) {
    T retval = *ptr_;
    ptr_ += stride_;
    return retval;
  }
  UnalignedStridedIterator& operator--() {
    ptr_ -= stride_;
    return *this;
  }
  UnalignedStridedIterator& operator+=(int64_t n) {
    ptr_ += (n * stride_);
    return *this;
  }
  UnalignedStridedIterator& operator-=(int64_t n) {
    ptr_ -= (n * stride_);
    return *this;
  }
  int64_t operator-(const UnalignedStridedIterator& other) const {
    return (ptr_ - other.ptr_) / stride_;
  }
  bool operator<(const UnalignedStridedIterator& other) const {
    return ptr_ < other.ptr_;
  }
  bool operator>(const UnalignedStridedIterator& other) const {
    return ptr_ > other.ptr_;
  }
  bool operator<=(const UnalignedStridedIterator& other) const {
    return ptr_ <= other.ptr_;
  }
  bool operator>=(const UnalignedStridedIterator& other) const {
    return ptr_ >= other.ptr_;
  }
  bool operator==(const UnalignedStridedIterator& other) const {
    return ptr_ == other.ptr_;
  }
  bool operator!=(const UnalignedStridedIterator& other) const {
    return ptr_ != other.ptr_;
  }

  T operator*() const { return load_(ptr_); }
  T operator[](ptrdiff_t i) const { return load_(ptr_ + (i * stride_)); }

  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = T;
  using reference = T&;
  using pointer = T*;

 protected:
  const uint8_t* ptr_;
  int64_t stride_;
  static constexpr Load load_{};
};

}  // namespace internal

template <typename T>
struct BoxXY;
template <typename T>
struct BoxXYZ;
template <typename T>
struct BoxXYM;
template <typename T>
struct BoxXYZM;
template <typename CoordSrc, typename CoordDst>
CoordDst CoordCast(CoordSrc src);

/// \brief Coord implementation for XY
template <typename T>
struct XY : public std::array<T, 2> {
  using box_type = BoxXY<T>;
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY;

  T x() const { return this->at(0); }
  T y() const { return this->at(1); }
  T z() const { return std::numeric_limits<T>::quiet_NaN(); }
  T m() const { return std::numeric_limits<T>::quiet_NaN(); }

  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    func(CoordCast<XY, CoordDst>(*this));
  }

  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    CoordDst coord = CoordCast<XY, CoordDst>(*this);
    func(coord, coord);
  }

  static XY FromXYZM(T x, T y, T z, T m) {
    GEOARROW_UNUSED(z);
    GEOARROW_UNUSED(m);
    return XY{x, y};
  }
};

/// \brief Coord implementation for XYZ
template <typename T>
struct XYZ : public std::array<T, 3> {
  using box_type = BoxXYZ<T>;
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XYZ;

  T x() const { return this->at(0); }
  T y() const { return this->at(1); }
  T z() const { return this->at(2); }
  T m() const { return std::numeric_limits<T>::quiet_NaN(); }

  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    func(CoordCast<XYZ, CoordDst>(*this));
  }

  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    CoordDst coord = CoordCast<XYZ, CoordDst>(*this);
    func(coord, coord);
  }

  static XYZ FromXYZM(T x, T y, T z, T m) {
    GEOARROW_UNUSED(m);
    return XYZ{x, y, z};
  }
};

/// \brief Coord implementation for XYM
template <typename T>
struct XYM : public std::array<T, 3> {
  using box_type = BoxXYM<T>;
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XYM;

  T x() const { return this->at(0); }
  T y() const { return this->at(1); }
  T z() const { return std::numeric_limits<T>::quiet_NaN(); }
  T m() const { return this->at(2); }

  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    func(CoordCast<XYM, CoordDst>(*this));
  }

  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    CoordDst coord = CoordCast<XYM, CoordDst>(*this);
    func(coord, coord);
  }

  static XYM FromXYZM(T x, T y, T z, T m) {
    GEOARROW_UNUSED(z);
    return XYM{x, y, m};
  }
};

/// \brief Coord implementation for XYZM
template <typename T>
struct XYZM : public std::array<T, 4> {
  using box_type = BoxXYZM<T>;
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XYZM;

  T x() const { return this->at(0); }
  T y() const { return this->at(1); }
  T z() const { return this->at(2); }
  T m() const { return this->at(3); }

  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    func(CoordCast<XYZM, CoordDst>(*this));
  }

  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    CoordDst coord = CoordCast<XYZM, CoordDst>(*this);
    func(coord, coord);
  }

  static XYZM FromXYZM(T x, T y, T z, T m) { return XYZM{x, y, z, m}; }
};

/// \brief Cast a coordinate from one type to another
///
/// When one type of coordinate is requested but another exists, perform
/// the transformation by (1) dropping dimensions or (2) filling dimensions
/// that did not previously exist with NaN. A difference between ordinate type
/// (e.g., double/float) is handled using static_cast<>().
template <typename CoordSrc, typename CoordDst>
CoordDst CoordCast(CoordSrc src) {
  if constexpr (std::is_same<CoordSrc, CoordDst>::value) {
    return src;
  } else if constexpr (std::is_same<CoordDst, XY<typename CoordSrc::value_type>>::value) {
    return XY<typename CoordSrc::value_type>{src.x(), src.y()};
  } else {
    return CoordDst::FromXYZM(static_cast<typename CoordDst::value_type>(src.x()),
                              static_cast<typename CoordDst::value_type>(src.y()),
                              static_cast<typename CoordDst::value_type>(src.z()),
                              static_cast<typename CoordDst::value_type>(src.m()));
  }
}

/// \brief Coord implementation for Box
template <typename T>
struct BoxXY : public std::array<T, 4> {
  using bound_type = XY<T>;
  T xmin() const { return this->at(0); }
  T ymin() const { return this->at(1); }
  T zmin() const { return std::numeric_limits<T>::infinity(); }
  T mmin() const { return std::numeric_limits<T>::infinity(); }
  T xmax() const { return this->at(2); }
  T ymax() const { return this->at(3); }
  T zmax() const { return -std::numeric_limits<T>::infinity(); }
  T mmax() const { return -std::numeric_limits<T>::infinity(); }
  bound_type lower_bound() const { return {xmin(), ymin()}; }
  bound_type upper_bound() const { return {xmax(), ymax()}; }
  static BoxXY Empty() {
    return {std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(),
            -std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity()};
  }
};

/// \brief Coord implementation for BoxZ
template <typename T>
struct BoxXYZ : public std::array<T, 6> {
  using bound_type = XYZ<T>;
  T xmin() const { return this->at(0); }
  T ymin() const { return this->at(1); }
  T zmin() const { return this->at(2); }
  T mmin() const { return std::numeric_limits<T>::infinity(); }
  T xmax() const { return this->at(3); }
  T ymax() const { return this->at(4); }
  T zmax() const { return this->at(5); }
  T mmax() const { return -std::numeric_limits<T>::infinity(); }
  bound_type lower_bound() const { return {xmin(), ymin(), zmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), zmax()}; }
  static BoxXYZ Empty() {
    return {std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(),
            -std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity()};
  }
};

/// \brief Coord implementation for BoxM
template <typename T>
struct BoxXYM : public std::array<T, 6> {
  using bound_type = XYM<T>;
  T xmin() const { return this->at(0); }
  T ymin() const { return this->at(1); }
  T zmin() const { return std::numeric_limits<T>::infinity(); }
  T mmin() const { return this->at(2); }
  T xmax() const { return this->at(3); }
  T ymax() const { return this->at(4); }
  T zmax() const { return -std::numeric_limits<T>::infinity(); }
  T mmax() const { return this->at(5); }
  bound_type lower_bound() const { return {xmin(), ymin(), mmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), mmax()}; }
  static BoxXYM Empty() {
    return {std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity(),
            -std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity()};
  }
};

/// \brief Coord implementation for BoxZM
template <typename T>
struct BoxXYZM : public std::array<T, 8> {
  using bound_type = XYZM<T>;
  T xmin() const { return this->at(0); }
  T ymin() const { return this->at(1); }
  T zmin() const { return this->at(2); }
  T mmin() const { return this->at(3); }
  T xmax() const { return this->at(4); }
  T ymax() const { return this->at(5); }
  T zmax() const { return this->at(6); }
  T mmax() const { return this->at(7); }
  bound_type lower_bound() const { return {xmin(), ymin(), zmin(), mmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), zmax(), mmax()}; }
  static BoxXYZM Empty() {
    return {std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity(),
            std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity(),
            -std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(),
            -std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity()};
  }
};

/// \brief View of an unaligned GeoArrow coordinate sequence
///
/// A view of zero or more coordinates. This data structure can handle either interleaved
/// or separated coordinates. This coordinate sequence type is intended to wrap
/// arbitrary bytes (e.g., WKB).
///
/// Like the CoordSequence, this sequence can handle structures beyond strictly
/// interleaved or separated coordinates. For example, the UnalignedCoordSequence can wrap
/// a sequence of contiguous WKB points (because the memory representing XY[Z[M]] values
/// are equally spaced, although the spacing is not an even multiple of the coordinate
/// size).
template <typename Coord,
          typename Load = internal::LoadIdentity<typename Coord::value_type>>
struct UnalignedCoordSequence {
  /// \brief The C++ Coordinate type
  using value_type = Coord;

  /// \brief The C++ numeric type for ordinate storage
  using ordinate_type = typename value_type::value_type;

  /// \brief The number of values in each coordinate
  static constexpr uint32_t coord_size = Coord().size();

  /// \brief The number of bytes in each coordinate
  static constexpr uint32_t coord_size_bytes = sizeof(Coord);

  /// \brief The offset into values to apply
  int64_t offset{};

  /// \brief The number of coordinates in the sequence
  int64_t length{};

  /// \brief Pointers to the first ordinate values in each dimension
  std::array<const uint8_t*, coord_size> values{};

  /// \brief The distance (in bytes) between sequential coordinates in
  /// each values array.
  int64_t stride_bytes{};

  /// \brief Initialize a dimension pointer for this array
  void InitValue(uint32_t i, const void* value) {
    values[i] = reinterpret_cast<const uint8_t*>(value);
  }

  /// \brief Initialize an interleaved coordinate sequence from a pointer to its start
  int InitInterleaved(int64_t length_elements, const void* data,
                      int64_t stride_elements = coord_size) {
    if (data == nullptr && length_elements != 0) {
      return EINVAL;
    }

    this->stride_bytes = stride_elements * sizeof(ordinate_type);
    this->offset = 0;
    this->length = length_elements;
    if (data != nullptr) {
      for (uint32_t i = 0; i < coord_size; ++i) {
        this->InitValue(
            i, reinterpret_cast<const uint8_t*>(data) + i * sizeof(ordinate_type));
      }
    }

    return 0;
  }

  /// \brief Initialize a separated coordinate sequence from pointers to each
  /// dimension start
  int InitSeparated(int64_t length_elements,
                    std::array<const void*, coord_size> dimensions,
                    int64_t stride_elements = 1) {
    this->offset = 0;
    this->length = length_elements;
    this->stride_bytes = stride_elements * sizeof(ordinate_type);
    for (uint32_t i = 0; i < dimensions.size(); i++) {
      this->InitValue(i, dimensions[i]);
    }

    return 0;
  }

  /// \brief Return a coordinate at the given position
  Coord coord(int64_t i) const {
    Coord out;
    for (size_t j = 0; j < out.size(); j++) {
      out[j] = load_(values[j] + ((offset + i) * stride_bytes));
    }
    return out;
  }

  /// \brief Return the number of coordinates in the sequence
  int64_t size() const { return length; }

  /// \brief Return a new coordinate sequence that is a subset of this one
  ///
  /// Caller is responsible for ensuring that offset + length is within the bounds
  /// of this function.
  UnalignedCoordSequence<Coord> Slice(int64_t offset, int64_t length) const {
    UnalignedCoordSequence<Coord> out = *this;
    out.offset += offset;
    out.length = length;
    return out;
  }

  /// \brief Call func once for each vertex in this sequence
  ///
  /// This function is templated on the desired coordinate output type.
  /// This allows, for example, iteration along all XYZM dimensions of an
  /// arbitrary sequence, even if some of those dimensions don't exist
  /// in the sequence. Similarly, one can iterate over fewer dimensions than
  /// are strictly in the output (discarding dimensions not of interest).
  template <typename CoordDst, typename Func>
  void VisitVertices(Func&& func) const {
    for (const auto vertex : *this) {
      func(CoordCast<Coord, CoordDst>(vertex));
    }
  }

  /// \brief Call func once for each sequential pair of vertices in this sequence
  ///
  /// This function is templated on the desired coordinate output type, performing
  /// the same coordinate conversion as VisitVertices. Note that sequential vertices
  /// may not be meaningful as edges for some types of sequences.
  template <typename CoordDst, typename Func>
  void VisitEdges(Func&& func) const {
    if (this->length < 2) {
      return;
    }

    auto it = begin();
    CoordDst start = CoordCast<Coord, CoordDst>(*it);
    ++it;
    while (it != end()) {
      CoordDst end = CoordCast<Coord, CoordDst>(*it);
      func(start, end);
      start = end;
      ++it;
    }
  }

  using const_iterator = internal::CoordSequenceIterator<UnalignedCoordSequence>;
  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, length); }

  using dimension_iterator =
      internal::UnalignedStridedIterator<typename value_type::value_type, Load>;
  dimension_iterator dbegin(uint32_t j) const {
    return dimension_iterator(values[j] + (offset * stride_bytes), stride_bytes);
  }
  dimension_iterator dend(uint32_t j) const {
    return dimension_iterator(values[j] + ((offset + length) * stride_bytes),
                              stride_bytes);
  }

 private:
  static constexpr Load load_{};
};

}  // namespace array_util

}  // namespace geoarrow

#endif
