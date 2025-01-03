
#ifndef GEOARROW_HPP_ITERATION_INCLUDED
#define GEOARROW_HPP_ITERATION_INCLUDED

#include <array>
#include <cmath>
#include <iterator>

#include "geoarrow.h"

namespace geoarrow {

namespace array {

namespace internal {

/// \brief Iterator implementation for CoordSequence
template <typename CoordSequence>
class CoordSequenceIterator {
  const CoordSequence& outer_;
  uint32_t i_;

 public:
  explicit CoordSequenceIterator(const CoordSequence& outer, uint32_t i = 0)
      : outer_(outer), i_(i) {}
  CoordSequenceIterator& operator++() {
    i_++;
    return *this;
  }
  CoordSequenceIterator operator++(int) {
    CoordSequenceIterator retval = *this;
    ++(*this);
    return retval;
  }
  bool operator==(const CoordSequenceIterator& other) const { return i_ == other.i_; }
  bool operator!=(const CoordSequenceIterator& other) const { return i_ != other.i_; }

  typename CoordSequence::coord_type operator*() const { return outer_[i_]; }
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = typename CoordSequence::coord_type;
};

/// \brief Iterator implementation for ListSequence
template <typename ListSequence>
class ListSequenceIterator {
  const ListSequence& outer_;
  int64_t i_;
  typename ListSequence::child_type stashed_;

 public:
  explicit ListSequenceIterator(const ListSequence& outer, int64_t i = 0)
      : outer_(outer), i_(i), stashed_(outer_.child) {}
  ListSequenceIterator& operator++() {
    i_++;
    return *this;
  }
  ListSequenceIterator operator++(int) {
    ListSequenceIterator retval = *this;
    ++(*this);
    return retval;
  }
  bool operator==(const ListSequenceIterator& other) const { return i_ == other.i_; }
  bool operator!=(const ListSequenceIterator& other) const { return i_ != other.i_; }

  typename ListSequence::child_type& operator*() {
    outer_.UpdateChild(&stashed_, i_);
    return stashed_;
  }
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = typename ListSequence::child_type&;
};
}  // namespace internal

/// \brief Coord implementation for XY
struct XY : public std::array<double, 2> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return NAN; }
  double m() const { return NAN; }
};

/// \brief Coord implementation for XYZ
struct XYZ : public std::array<double, 3> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return at(2); }
  double m() const { return NAN; }
};

/// \brief Coord implementation for XYM
struct XYM : public std::array<double, 3> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return NAN; }
  double m() const { return at(2); }
};

/// \brief Coord implementation for XYZM
struct XYZM : public std::array<double, 4> {
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
  double zmin() const { return INFINITY; }
  double mmin() const { return INFINITY; }
  double xmax() const { return at(2); }
  double ymax() const { return at(3); }
  double zmax() const { return -INFINITY; }
  double mmax() const { return -INFINITY; }
  bound_type lower_bound() const { return {xmin(), ymin()}; }
  bound_type upper_bound() const { return {xmax(), ymax()}; }
};

/// \brief Coord implementation for BoxZ
struct BoxXYZ : public std::array<double, 6> {
  using bound_type = XYZ;
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return at(2); }
  double mmin() const { return INFINITY; }
  double xmax() const { return at(3); }
  double ymax() const { return at(4); }
  double zmax() const { return at(5); }
  double mmax() const { return -INFINITY; }
  bound_type lower_bound() const { return {xmin(), ymin(), zmin()}; }
  bound_type upper_bound() const { return {xmax(), ymax(), zmax()}; }
};

/// \brief Coord implementation for BoxM
struct BoxXYM : public std::array<double, 6> {
  using bound_type = XYM;
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return INFINITY; }
  double mmin() const { return at(2); }
  double xmax() const { return at(3); }
  double ymax() const { return at(4); }
  double zmax() const { return -INFINITY; }
  double mmax() const { return at(5); }
  bound_type lower_bound() const { return {mmin(), ymin(), mmin()}; }
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
  double zmax() const { return at(7); }
  double mmax() const { return at(8); }
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
  using coord_type = Coord;

  /// \brief Trait to indicate that this is a sequence (and not a list)
  static constexpr bool is_sequence = true;

  /// \brief The number of values in each coordinate
  static constexpr uint32_t coord_size = Coord().size();

  /// \brief The offset into values to apply
  uint32_t offset;

  /// \brief The number of coordinates in the sequence
  uint32_t length;

  /// \brief Pointers to the first ordinate values in each dimension
  ///
  /// This structure can accomodate either interleaved or separated
  /// coordinates. For interleaved coordinates, these pointers will be
  /// contiguous; for separated coordinates these pointers will point
  /// to separate arrays. Each ordinate value is accessed by using the
  /// expression `values[dimension_id][(offset + coord_id) * stride]`.
  std::array<const double*, coord_size> values;

  /// \brief The distance (in elements) between sequential coordinates in
  /// each values array.
  ///
  /// For interleaved coordinates this is the number of dimensions in the
  /// input; for separated coordinates this is 1. This does not need to be
  /// equal to coord_size (e.g., when providing a CoordSequence<XY> view
  /// of an interleaved sequence of XYZM coordinates).
  uint32_t stride;

  /// \brief Return a coordinate at the given position
  Coord operator[](uint32_t i) const {
    Coord out;
    for (size_t j = 0; j < out.size(); j++) {
      out[j] = values[j][(offset + i) * stride];
    }
    return out;
  }

  /// \brief Return the number of coordinates in the sequence
  uint32_t size() const { return length; }

  using Iterator = internal::CoordSequenceIterator<CoordSequence>;
  Iterator begin() const { return Iterator(*this); }
  Iterator end() const { return Iterator(*this, length); }
};

/// \brief View of a sequence of lists
template <typename T>
struct ListSequence {
  /// \brief The child view type (either a ListSequence or a CoordSequence)
  using child_type = T;

  /// \brief Trait to indicate this is not a CoordSequence
  static constexpr bool is_sequence = false;

  /// \brief The logical offset into the sequence
  uint32_t offset;

  /// \brief The number of lists in the sequence
  uint32_t length;

  /// \brief The pointer to the first offset
  ///
  /// These offsets are sequential such that the offset of the ith element in the
  /// sequence begins at offsets[i]. This means there must be (offset + length + 1)
  /// accessible elements in offsets. This is exactly equal to the definition of the
  /// offsets in the Apache Arrow list type.
  const int32_t* offsets;

  /// \brief The item from which slices are to be taken according to each offset pair
  ///
  /// Note that the child may have its own non-zero offset which must also be applied.
  T child;

  /// \brief Initialize a child whose offset and length are unset.
  void InitChild(T* child_p) const {
    *child_p = child;
  }

  /// \brief Update a child initialized with InitChild such that it represents the
  /// ith element of the array.
  void UpdateChild(T* child_p, uint32_t i) const {
    int32_t child_offset = offsets[offset + i];
    child_p->offset = child.offset + child_offset;
    child_p->length = offsets[offset + i + 1] - child_offset;
  }

  using Iterator = internal::ListSequenceIterator<ListSequence>;
  Iterator begin() const { return Iterator(*this); }
  Iterator end() const { return Iterator(*this, length); }
};

namespace internal {

// Helpers to populate the above views from geoarrow-c structures. These structures
// are intentionally omitted from the above definitions to keep the definitions
// self-contained.

template <typename T>
GeoArrowErrorCode InitFromCoordView(T* value, const struct GeoArrowCoordView* view) {
  if (view->n_values != T::coord_size) {
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
void InitFromArrayView(T* value, const struct GeoArrowArrayView* view, int level) {
  if constexpr (T::is_sequence) {
    InitFromCoordView(value, &view->coords);
  } else {
    value->offsets = view->offsets[level];
    InitFromArrayView(&value->child, view, level + 1);
  }

  value->offset = view->offset[level];
  value->length = view->length[level];
};

}  // namespace internal

template <typename T>
struct Array {
  using value_type = T;
  value_type value;
  const uint8_t* validity;

  void Init(const struct GeoArrowArrayView* view) {
    internal::InitFromArrayView(&value, view, 0);
    validity = view->validity_bitmap;
  }
};

template <typename Coord>
struct PointArray : public Array<CoordSequence<Coord>> {
  static constexpr enum GeoArrowGeometryType geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
};

template <typename Coord>
struct LinestringArray : public Array<ListSequence<CoordSequence<Coord>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_LINESTRING;
};

template <typename Coord>
struct PolygonArray : public Array<ListSequence<ListSequence<CoordSequence<Coord>>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_POLYGON;
};

template <typename Coord>
struct MultipointArray : public Array<CoordSequence<Coord>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_MULTIPOINT;
};

template <typename Coord>
struct MultiLinestringArray
    : public Array<ListSequence<ListSequence<CoordSequence<Coord>>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_MULTILINESTRING;
};

template <typename Coord>
struct MultiPolygonArray
    : public Array<ListSequence<ListSequence<CoordSequence<Coord>>>> {
  static constexpr enum GeoArrowGeometryType geometry_type =
      GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON;
};

}  // namespace array
}  // namespace geoarrow

#endif
