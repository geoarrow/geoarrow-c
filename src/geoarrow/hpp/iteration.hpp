
#ifndef GEOARROW_HPP_ITERATION_INCLUDED
#define GEOARROW_HPP_ITERATION_INCLUDED

#include <array>
#include <iterator>
#include "geoarrow.h"

namespace geoarrow {

namespace array {

struct XY : public std::array<double, 2> {
  double x() const { return at(0); }
  double y() const { return at(1); }
};

struct XYZ : public XY {
  double z() const { return at(2); }
};

struct XYM : public XY {
  double m() const { return at(2); }
};

struct XYZM : public XYZ {
  double m() const { return at(3); }
};

template <typename Coord>
struct CoordSequence {
  int64_t offset;
  int64_t length;
  struct GeoArrowCoordView* coord_view;

  Coord operator[](int64_t i) const {
    Coord out;
    for (size_t j = 0; j < out.size(); j++) {
      out[j] = GEOARROW_COORD_VIEW_VALUE(coord_view, offset + i, j);
    }
    return out;
  }

  int64_t size() const { return length; }

  class Iterator {
    const CoordSequence& outer_;
    int64_t i_;

   public:
    explicit Iterator(const CoordSequence& outer, int64_t i = 0) : outer_(outer), i_(i) {}
    Iterator& operator++() {
      i_++;
      return *this;
    }
    Iterator operator++(int) {
      Iterator retval = *this;
      ++(*this);
      return retval;
    }
    bool operator==(Iterator other) const { return i_ == other.i_; }
    bool operator!=(Iterator other) const { return i_ != other.i_; }

    Coord operator*() const { return outer_[i_]; }
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = int64_t;
    using value_type = Coord;
  };

  Iterator begin() const { return Iterator(*this); }
  Iterator end() const { return Iterator(*this, length); }
};

template <typename T>
struct Nested {
  int64_t offset;
  int64_t length;
  const int32_t* offsets;
  T child;

  class Iterator {
    const Nested& outer_;
    int64_t i_;
    T stashed_;

   public:
    explicit Iterator(const Nested& outer, int64_t i = 0)
        : outer_(outer), i_(i), stashed_(outer_.child) {}
    Iterator& operator++() {
      i_++;
      return *this;
    }
    Iterator operator++(int) {
      Iterator retval = *this;
      ++(*this);
      return retval;
    }
    bool operator==(Iterator other) const { return i_ == other.i_; }
    bool operator!=(Iterator other) const { return i_ != other.i_; }

    const T& operator*() {
      stashed_.offset = outer_.offsets[outer_.offset + i_];
      stashed_.length = outer_.offsets[outer_.offset + i_ + 1] - stashed_.offset;
      return stashed_;
    }
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = int64_t;
    using value_type = const T&;
  };

  Iterator begin() const { return Iterator(*this); }
  Iterator end() const { return Iterator(*this, length); }
};

}  // namespace array
}  // namespace geoarrow

#endif
