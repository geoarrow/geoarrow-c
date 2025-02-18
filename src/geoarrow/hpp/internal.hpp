
#ifndef GEOARROW_HPP_INTERNAL_INCLUDED
#define GEOARROW_HPP_INTERNAL_INCLUDED

#include "geoarrow/geoarrow.h"

namespace geoarrow {

namespace internal {
struct SchemaHolder {
  struct ArrowSchema schema {};
  ~SchemaHolder() {
    if (schema.release != nullptr) {
      schema.release(&schema);
    }
  }
};

struct ArrayHolder {
  struct ArrowArray array {};
  ~ArrayHolder() {
    if (array.release != nullptr) {
      array.release(&array);
    }
  }
};

template <typename T>
static inline void FreeWrappedBuffer(uint8_t* ptr, int64_t size, void* private_data) {
  GEOARROW_UNUSED(ptr);
  GEOARROW_UNUSED(size);
  auto obj = reinterpret_cast<T*>(private_data);
  delete obj;
}

template <typename T>
static inline struct GeoArrowBufferView BufferView(const T& v) {
  if (v.size() == 0) {
    return {nullptr, 0};
  } else {
    return {reinterpret_cast<const uint8_t*>(v.data()),
            static_cast<int64_t>(v.size() * sizeof(typename T::value_type))};
  }
}

}  // namespace internal

}  // namespace geoarrow

#endif
