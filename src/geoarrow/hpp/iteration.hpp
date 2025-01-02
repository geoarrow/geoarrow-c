
#ifndef GEOARROW_HPP_ITERATION_INCLUDED
#define GEOARROW_HPP_ITERATION_INCLUDED

#include <array>
#include <cmath>
#include <iterator>

#include "geoarrow.h"

namespace geoarrow {

namespace array {

struct XY : public std::array<double, 2> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return NAN; }
  double m() const { return NAN; }
};

struct XYZ : public std::array<double, 3> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return at(2); }
  double m() const { return NAN; }
};

struct XYM : public std::array<double, 3> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return NAN; }
  double m() const { return at(2); }
};

struct XYZM : public std::array<double, 4> {
  double x() const { return at(0); }
  double y() const { return at(1); }
  double z() const { return at(2); }
  double m() const { return at(3); }
};

struct BoxXY : public std::array<double, 4> {
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return INFINITY; }
  double mmin() const { return INFINITY; }
  double xmax() const { return at(2); }
  double ymax() const { return at(3); }
  double zmax() const { return -INFINITY; }
  double mmax() const { return -INFINITY; }
};

struct BoxXYZ : public std::array<double, 6> {
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return at(2); }
  double mmin() const { return INFINITY; }
  double xmax() const { return at(3); }
  double ymax() const { return at(4); }
  double zmax() const { return at(5); }
  double mmax() const { return -INFINITY; }
};

struct BoxXYM : public std::array<double, 6> {
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return INFINITY; }
  double mmin() const { return at(2); }
  double xmax() const { return at(3); }
  double ymax() const { return at(4); }
  double zmax() const { return -INFINITY; }
  double mmax() const { return at(5); }
};

struct BoxXYZM : public std::array<double, 8> {
  double xmin() const { return at(0); }
  double ymin() const { return at(1); }
  double zmin() const { return at(2); }
  double mmin() const { return at(3); }
  double xmax() const { return at(4); }
  double ymax() const { return at(5); }
  double zmax() const { return at(7); }
  double mmax() const { return at(8); }
};

template <typename CoordSequence>
class CoordSequenceIterator {
  const CoordSequence& outer_;
  int64_t i_;

 public:
  explicit CoordSequenceIterator(const CoordSequence& outer, int64_t i = 0)
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

template <typename Nested>
class NestedIterator {
  const Nested& outer_;
  int64_t i_;
  typename Nested::child_type stashed_;

 public:
  explicit NestedIterator(const Nested& outer, int64_t i = 0)
      : outer_(outer), i_(i), stashed_(outer_.child) {}
  NestedIterator& operator++() {
    i_++;
    return *this;
  }
  NestedIterator operator++(int) {
    NestedIterator retval = *this;
    ++(*this);
    return retval;
  }
  bool operator==(const NestedIterator& other) const { return i_ == other.i_; }
  bool operator!=(const NestedIterator& other) const { return i_ != other.i_; }

  typename Nested::child_type& operator*() {
    outer_.UpdateChild(&stashed_, i_);
    return stashed_;
  }
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = int64_t;
  using value_type = typename Nested::child_type&;
};

template <typename Coord>
struct CoordSequence {
  using coord_type = Coord;
  static constexpr uint32_t coord_size = Coord().size();
  uint32_t offset;
  uint32_t length;
  struct GeoArrowCoordView* coord_view;
  double* values[coord_size];
  uint32_t stride;

  Coord operator[](int64_t i) const {
    Coord out;
    for (size_t j = 0; j < out.size(); j++) {
      out[j] = GEOARROW_COORD_VIEW_VALUE(coord_view, offset + i, j);
    }
    return out;
  }

  int64_t size() const { return length; }

  CoordSequenceIterator<CoordSequence> begin() const {
    return CoordSequenceIterator<CoordSequence>(*this);
  }
  CoordSequenceIterator<CoordSequence> end() const {
    return CoordSequenceIterator<CoordSequence>(*this, length);
  }
};

template <typename T>
struct Nested {
  using child_type = T;

  int64_t offset;
  int64_t length;
  const int32_t* offsets;
  T child;

  void InitChild(T* child_p) const {
    *child_p = child;
    UpdateChild(child_p, 0);
  }

  void UpdateChild(T* child_p, int64_t i) const {
    int32_t child_offset = offsets[offset + i];
    child_p->offset = child.offset + child_offset;
    child_p->length = offsets[offset + i + 1] - child_offset;
  }

  NestedIterator<Nested> begin() const { return NestedIterator<Nested>(*this); }
  NestedIterator<Nested> end() const { return NestedIterator<Nested>(*this, length); }
};

}  // namespace array
}  // namespace geoarrow

#endif
