
#ifndef GEOARROW_HPP_ITERATION_INCLUDED
#define GEOARROW_HPP_ITERATION_INCLUDED

#include <array>

#include "geoarrow.h"

namespace geoarrow {

namespace array {

template <typename Coord>
struct CoordSequence {
  int64_t offset;
  int64_t length;
  struct GeoArrowCoordView* coord_view;

  class Iterator {
    const CoordSequence& outer_;
    int64_t i_ = 0;

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
    bool operator==(Iterator other) const {
      return outer_.coord_view == other.outer_.coord_view && i_ == other.i_;
    }
    bool operator!=(Iterator other) const { return !(*this == other); }

    Coord operator*() const {
      Coord out;
      for (size_t i = 0; i < out.size(); i++) {
        out[i] = GEOARROW_COORD_VIEW_VALUE(outer_.coord_view, outer_.offset + i_, i);
      }
      return out;
    }
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = int64_t;
    using value_type = Coord;
    using pointer = const Coord*;
    using reference = const Coord&;
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
};

template <typename T>
struct Nullable : public T {
  const uint8_t* validity;
};

}  // namespace array
}  // namespace geoarrow

#endif
