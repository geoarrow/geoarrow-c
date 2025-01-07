
#ifndef GEOARROW_HPP_ITERATION_INCLUDED
#define GEOARROW_HPP_ITERATION_INCLUDED

#include <array>
#include <iterator>
#include <limits>

#include "geoarrow_type.h"

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
template <typename Outer>
class BaseRandomAccessIterator {
 public:
  explicit BaseRandomAccessIterator(const Outer& outer, uint32_t i)
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
  uint32_t i_;
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

  explicit CoordSequenceIterator(const CoordSequence& outer, uint32_t i)
      : BaseRandomAccessIterator<CoordSequence>(outer, i) {}
  value_type operator*() const { return this->outer_.coord(this->i_); }
  value_type operator[](int64_t i) const { return this->outer_.coord(this->i_ + i); }
};

/// \brief Iterator implementation for ListSequence
template <typename ListSequence>
class ListSequenceIterator : public BaseRandomAccessIterator<ListSequence> {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = typename ListSequence::value_type;

  explicit ListSequenceIterator(const ListSequence& outer, uint32_t i)
      : BaseRandomAccessIterator<ListSequence>(outer, i), stashed_(outer.child) {}

  typename ListSequence::child_type& operator*() {
    this->outer_.UpdateChild(&stashed_, this->i_);
    return stashed_;
  }

  // Not quite a random access iterator because it doesn't implement []
  // (which would necessitate a copy, which we don't really want to do)
 private:
  typename ListSequence::child_type stashed_;
};

// Iterator for dimension begin/end
template <typename T>
class StridedIterator {
 public:
  explicit StridedIterator(const T* ptr, ptrdiff_t stride) : ptr_(ptr), stride_(stride) {}
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
  StridedIterator& operator+=(ptrdiff_t n) {
    ptr_ += (n * stride_);
    return *this;
  }
  StridedIterator& operator-=(ptrdiff_t n) {
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

}  // namespace internal

struct BoxXY;
struct BoxXYZ;
struct BoxXYM;
struct BoxXYZM;

/// \brief Coord implementation for XY
struct XY : public std::array<double, 2> {
  using box_type = BoxXY;
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return std::numeric_limits<double>::quiet_NaN(); }
  double m() const { return std::numeric_limits<double>::quiet_NaN(); }
};

/// \brief Coord implementation for XYZ
struct XYZ : public std::array<double, 3> {
  using box_type = BoxXYZ;
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return at(2); }
  double m() const { return std::numeric_limits<double>::quiet_NaN(); }
};

/// \brief Coord implementation for XYM
struct XYM : public std::array<double, 3> {
  using box_type = BoxXYM;
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return std::numeric_limits<double>::quiet_NaN(); }
  double m() const { return at(2); }
};

/// \brief Coord implementation for XYZM
struct XYZM : public std::array<double, 4> {
  using box_type = BoxXYZM;
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return at(2); }
  double m() const { return at(3); }
};

/// \brief Coord implementation for Box
struct BoxXY : public std::array<double, 4> {
  using bound_type = XY;
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return std::numeric_limits<double>::infinity(); }
  double mmin() const { return std::numeric_limits<double>::infinity(); }
  double xmax() const { return at(2); }
  double ymax() const { return at(3); }
  double zmax() const { return -std::numeric_limits<double>::infinity(); }
  double mmax() const { return -std::numeric_limits<double>::infinity(); }
  bound_type lower_bound() const { return {xmin(), ymin()}; }
  bound_type upper_bound() const { return {xmax(), ymax()}; }
};

/// \brief Coord implementation for BoxZ
struct BoxXYZ : public std::array<double, 6> {
  using bound_type = XYZ;
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return at(2); }
  double mmin() const { return std::numeric_limits<double>::infinity(); }
  double xmax() const { return at(3); }
  double ymax() const { return at(4); }
  double zmax() const { return at(5); }
  double mmax() const { return -std::numeric_limits<double>::infinity(); }
  bound_type lower_bound() const { return {xmin(), ymin(), zmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), zmax()}; }
};

/// \brief Coord implementation for BoxM
struct BoxXYM : public std::array<double, 6> {
  using bound_type = XYM;
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return std::numeric_limits<double>::infinity(); }
  double mmin() const { return at(2); }
  double xmax() const { return at(3); }
  double ymax() const { return at(4); }
  double zmax() const { return -std::numeric_limits<double>::infinity(); }
  double mmax() const { return at(5); }
  bound_type lower_bound() const { return {xmin(), ymin(), mmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), mmax()}; }
};

/// \brief Coord implementation for BoxZM
struct BoxXYZM : public std::array<double, 8> {
  using bound_type = XYZM;
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return at(2); }
  double mmin() const { return at(3); }
  double xmax() const { return at(4); }
  double ymax() const { return at(5); }
  double zmax() const { return at(6); }
  double mmax() const { return at(7); }
  bound_type lower_bound() const { return {xmin(), ymin(), zmin(), mmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), zmax(), mmax()}; }
};

/// \brief View of a GeoArrow coordinate sequence
///
/// A view of zero or more coordinates. This data structure can handle either interleaved
/// or separated coordinates.
template <typename Coord>
struct CoordSequence {
  /// \brief The C++ Coordinate type. This type must implement size() and
  /// assignment via [].
  using value_type = Coord;

  /// \brief Trait to indicate that this is a sequence (and not a list)
  static constexpr bool is_sequence = true;

  /// \brief The number of values in each coordinate
  static constexpr uint32_t coord_size = Coord().size();

  /// \brief The offset into values to apply
  uint32_t offset{};

  /// \brief The number of coordinates in the sequence
  uint32_t length{};

  /// \brief Pointers to the first ordinate values in each dimension
  ///
  /// This structure can accommodate either interleaved or separated
  /// coordinates. For interleaved coordinates, these pointers will be
  /// contiguous; for separated coordinates these pointers will point
  /// to separate arrays. Each ordinate value is accessed by using the
  /// expression `values[dimension_id][(offset + coord_id) * stride]`.
  std::array<const double*, coord_size> values{};

  /// \brief The distance (in elements) between sequential coordinates in
  /// each values array.
  ///
  /// For interleaved coordinates this is the number of dimensions in the
  /// input; for separated coordinates this is 1. This does not need to be
  /// equal to coord_size (e.g., when providing a CoordSequence<XY> view
  /// of an interleaved sequence of XYZM coordinates).
  uint32_t stride{};

  /// \brief Return a coordinate at the given position
  Coord coord(uint32_t i) const {
    Coord out;
    for (size_t j = 0; j < out.size(); j++) {
      out[j] = values[j][(offset + i) * stride];
    }
    return out;
  }

  /// \brief Return the number of coordinates in the sequence
  uint32_t size() const { return length; }

  CoordSequence<Coord> Slice(uint32_t offset, uint32_t length) const {
    CoordSequence<Coord> out = *this;
    out.offset += offset;
    out.length = length;
    return out;
  }

  using const_iterator = internal::CoordSequenceIterator<CoordSequence>;
  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, length); }

  using dimension_iterator = internal::StridedIterator<double>;
  dimension_iterator dbegin(uint32_t j) const {
    return dimension_iterator(values[j] + (offset * stride), stride);
  }
  dimension_iterator dend(uint32_t j) const {
    return dimension_iterator(values[j] + ((offset + length) * stride), stride);
  }
};

/// \brief View of a sequence of lists
template <typename T>
struct ListSequence {
  /// \brief The child view type (either a ListSequence or a CoordSequence)
  using child_type = T;

  /// \brief For the purposes of iteration, the value type is a const reference
  /// to the child type (stashed in the iterator).
  using value_type = const T&;

  /// \brief Trait to indicate this is not a CoordSequence
  static constexpr bool is_sequence = false;

  /// \brief The logical offset into the sequence
  uint32_t offset{};

  /// \brief The number of lists in the sequence
  uint32_t length{};

  /// \brief The pointer to the first offset
  ///
  /// These offsets are sequential such that the offset of the ith element in the
  /// sequence begins at offsets[i]. This means there must be (offset + length + 1)
  /// accessible elements in offsets. This is exactly equal to the definition of the
  /// offsets in the Apache Arrow list type.
  const int32_t* offsets{};

  /// \brief The item from which slices are to be taken according to each offset pair
  ///
  /// Note that the child may have its own non-zero offset which must also be applied.
  T child{};

  T ValidChildElements() const {
    if (length == 0) {
      return child.Slice(0, 0);
    } else {
      uint32_t first_offset = offsets[offset];
      uint32_t last_offset = offsets[offset + length];
      return child.Slice(first_offset, last_offset - first_offset);
    }
  }

  /// \brief Initialize a child whose offset and length are unset.
  void InitChild(T* child_p) const { *child_p = child; }

  /// \brief Update a child initialized with InitChild such that it represents the
  /// ith element of the array.
  void UpdateChild(T* child_p, uint32_t i) const {
    int32_t child_offset = offsets[offset + i];
    child_p->offset = child.offset + child_offset;
    child_p->length = offsets[offset + i + 1] - child_offset;
  }

  ListSequence<T> Slice(uint32_t offset, uint32_t length) const {
    ListSequence<T> out = *this;
    out.offset += offset;
    out.length = length;
    return out;
  }

  using const_iterator = internal::ListSequenceIterator<ListSequence>;
  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, length); }
};

namespace internal {

// Helpers to populate the above views from geoarrow-c structures. These structures
// are intentionally omitted from the above definitions to keep the definitions
// self-contained.

template <typename T>
GeoArrowErrorCode InitFromCoordView(T* value, const struct GeoArrowCoordView* view) {
  if (view->n_values < T::coord_size) {
    return EINVAL;
  }

  value->offset = 0;
  value->length = view->n_coords;
  value->stride = view->coords_stride;
  for (uint32_t i = 0; i < T::coord_size; i++) {
    value->values[i] = view->values[i];
  }
  return GEOARROW_OK;
}

template <typename T>
GeoArrowErrorCode InitFromArrayView(T* value, const struct GeoArrowArrayView* view,
                                    int level) {
  if constexpr (T::is_sequence) {
    if (level != view->n_offsets) {
      return EINVAL;
    }

    GEOARROW_RETURN_NOT_OK(InitFromCoordView(value, &view->coords));
  } else {
    if (level > (view->n_offsets - 1)) {
      return EINVAL;
    }

    value->offsets = view->offsets[level];
    GEOARROW_RETURN_NOT_OK(InitFromArrayView(&value->child, view, level + 1));
  }

  value->offset = view->offset[level];
  value->length = view->length[level];

  return GEOARROW_OK;
};

template <typename Coord>
struct CoordTraits;

template <>
struct CoordTraits<XY> {
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY;
};

template <>
struct CoordTraits<XYZ> {
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XYZ;
};

template <>
struct CoordTraits<XYM> {
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XYM;
};

template <>
struct CoordTraits<XYZM> {
  static constexpr enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XYZM;
};

}  // namespace internal

/// \brief A nullable sequence (either a ListSequence or a CoordSequence)
///
/// Unlike the ListSequence and CoordSequence types, elements may be nullable.
template <typename T>
struct Array {
  /// \brief The sequence type (either a ListSequence or a CoordSequence)
  using sequence_type = T;

  /// \brief An instance of the ListSequence or CoordSequence
  T value{};

  /// \brief A validity bitmap where a set bit indicates a non-null value
  /// and an unset bit indicates a null value.
  ///
  /// The pointer itself may be (C++) nullptr, which indicates that all values
  /// in the array are non-null. Bits use least-significant bit ordering such that
  /// the validity of the ith element in the array is calculated with the expression
  /// `validity[i / 8] & (1 << (i % 8))`. This is exactly equal to the definition of
  /// the validity bitmap in the Apache Arrow specification.
  ///
  /// Note that the offset of the underlying sequence must be applied.
  const uint8_t* validity{};

  /// \brief Return the validity of a given element
  ///
  /// Note that this is not an efficient mechanism to check for nullability in a loop.
  bool is_valid(uint32_t i) const {
    i += value.offset;
    return validity == nullptr || validity[i / 8] & (1 << (i % 8));
  }

  /// \brief Return the nullness of a given element
  ///
  /// Note that this is not an efficient mechanism to check for nullability in a loop.
  bool is_null(uint32_t i) const {
    i += value.offset;
    return validity != nullptr && !(validity[i / 8] & (1 << (i % 8)));
  }

  /// \brief Initialize an Array from a GeoArrowArrayView
  ///
  /// Returns EINVAL if the nesting levels and/or coordinate size
  /// is incompatible with the values in the view.
  GeoArrowErrorCode Init(const struct GeoArrowArrayView* view) {
    GEOARROW_RETURN_NOT_OK(internal::InitFromArrayView(&value, view, 0));
    validity = view->validity_bitmap;
    return GEOARROW_OK;
  }

 protected:
  template <typename Impl>
  Impl SliceImpl(Impl self, uint32_t offset, uint32_t length) {
    self.value.offset += offset;
    self.value.length = length;
    return self;
  }
};

/// \brief An Array of points
template <typename Coord>
struct PointArray : public Array<CoordSequence<Coord>> {
  static constexpr enum GeoArrowGeometryType geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  /// \brief Return a view of all coordinates in this array
  ///
  /// Note that in the presence of null values, some of the coordinates values
  /// are not present in the array (e.g., for the purposes of calculating aggregate
  /// statistics).
  CoordSequence<Coord> Coords() const { return this->value; }

  PointArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<PointArray>(*this, offset, length);
  }
};

/// \brief An Array of boxes
template <typename Coord>
struct BoxArray : public Array<CoordSequence<typename Coord::box_type>> {
  static constexpr enum GeoArrowGeometryType geometry_type = GEOARROW_GEOMETRY_TYPE_BOX;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  PointArray<Coord> LowerBound() {
    PointArray<Coord> out;
    out.validity = this->validity;
    out.value.stride = this->value.stride;
    out.value.offset = this->value.offset;
    out.value.length = this->value.length;
    for (size_t i = 0; i < Coord().size(); i++) {
      out.value.values[i] = this->value.values[i];
    }

    return out;
  }

  PointArray<Coord> UpperBound() {
    PointArray<Coord> out;
    out.validity = this->validity;
    out.value.stride = this->value.stride;
    out.value.offset = this->value.offset;
    out.value.length = this->value.length;
    for (size_t i = 0; i < Coord().size(); i++) {
      out.value.values[i] = this->value.values[Coord().size() + i];
    }

    return out;
  }

  BoxArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<BoxArray>(*this, offset, length);
  }
};

/// \brief An Array of linestrings
template <typename Coord>
struct LinestringArray : public Array<ListSequence<CoordSequence<Coord>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_LINESTRING;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  /// \brief Return a view of all coordinates in this array
  ///
  /// Note that in the presence of null values, some of the coordinates values
  /// are not present in the array (e.g., for the purposes of calculating aggregate
  /// statistics).
  CoordSequence<Coord> Coords() const { return this->value.ValidChildElements(); }

  LinestringArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<LinestringArray>(*this, offset, length);
  }
};

/// \brief An Array of polygons
template <typename Coord>
struct PolygonArray : public Array<ListSequence<ListSequence<CoordSequence<Coord>>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_POLYGON;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  /// \brief Return a view of all coordinates in this array
  ///
  /// Note that in the presence of null values, some of the coordinates values
  /// are not present in the array (e.g., for the purposes of calculating aggregate
  /// statistics).
  CoordSequence<Coord> Coords() const {
    return this->value.ValidChildElements().ValidChildElements();
  }

  PolygonArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<PolygonArray>(*this, offset, length);
  }
};

/// \brief An Array of multipoints
template <typename Coord>
struct MultipointArray : public Array<CoordSequence<Coord>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_MULTIPOINT;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  // \brief Return a view of all coordinates in this array
  ///
  /// Note that in the presence of null values, some of the coordinates values
  /// are not present in the array (e.g., for the purposes of calculating aggregate
  /// statistics).
  CoordSequence<Coord> Coords() const { return this->value.ValidChildElements(); }

  MultipointArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<MultipointArray>(*this, offset, length);
  }
};

/// \brief An Array of multilinestrings
template <typename Coord>
struct MultiLinestringArray
    : public Array<ListSequence<ListSequence<CoordSequence<Coord>>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_MULTILINESTRING;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  // \brief Return a view of all coordinates in this array
  ///
  /// Note that in the presence of null values, some of the coordinates values
  /// are not present in the array (e.g., for the purposes of calculating aggregate
  /// statistics).
  CoordSequence<Coord> Coords() const {
    return this->value.ValidChildElements().ValidChildElements();
  }

  MultiLinestringArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<MultiLinestringArray>(*this, offset, length);
  }
};

/// \brief An Array of multipolygons
template <typename Coord>
struct MultiPolygonArray
    : public Array<ListSequence<ListSequence<ListSequence<CoordSequence<Coord>>>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON;
  static constexpr enum GeoArrowDimensions dimensions =
      internal::CoordTraits<Coord>::dimensions;

  // \brief Return a view of all coordinates in this array
  ///
  /// Note that in the presence of null values, some of the coordinates values
  /// are not present in the array (e.g., for the purposes of calculating aggregate
  /// statistics).
  CoordSequence<Coord> Coords() const {
    return this->value.ValidChildElements().ValidChildElements().ValidChildElements();
  }

  MultiPolygonArray Slice(uint32_t offset, uint32_t length) {
    return this->template SliceImpl<MultiPolygonArray>(*this, offset, length);
  }
};

}  // namespace array_util
}  // namespace geoarrow

/// @}

#endif
