
# geoarrow-c

[![Codecov test coverage](https://codecov.io/gh/geoarrow/geoarrow-c/branch/main/graph/badge.svg)](https://app.codecov.io/gh/geoarrow/geoarrow-c?branch=main)
[![Documentation](https://img.shields.io/badge/Documentation-dev-yellow)](https://geoarrow.github.io/geoarrow-c/dev)
[![geoarrow on GitHub](https://img.shields.io/badge/GitHub-geoarrow%2Fgeoarrow--c-blue)](https://github.com/geoarrow/geoarrow-c)

The geoarrow C library is a geospatial type system and generic
coordinate-shuffling library written in C with bindings in C++,
R, and Python. The library supports well-known binary (WKB),
well-known text (ISO) and [geoarrow](https://github.com/geoarrow/geoarrow)
encodings as Arrow extension types with all possible mutual
conversions including support for Z, M, and ZM geometries.

The library currently implements version 0.2.0 of the GeoArrow
specification. The easiest way to get started with GeoArrow/C is to use
the [Python bindings](https://github.com/geoarrow/geoarrow-python)
or [R bindings](https://github.com/geoarrow/geoarrow-r)
which currently use geoarrow-c under the hood for most operations.

The C library supports CMake with FetchContent:

```cmake
FetchContent_Declare(
  geoarrow
  URL https://github.com/geoarrow/geoarrow-c/archive/refs/heads/main.zip)

FetchContent_MakeAvailable(nanoarrow)
FetchContent_MakeAvailable(geoarrow)
```

The most straightforward way to get started producing and consuming is the C++
interface:

```cpp
#include <vector>
#include "geoarrow/geoarrow.hpp"

void Example() {
  // Build the buffers using C++ objects
  std::vector<uint8_t> wkb = MakeLinestringWKB(coords);
  std::vector<int32_t> offsets = {0, static_cast<int32_t>(wkb.size())};

  // Use SetBufferWrapped() to move these into an ArrowArray. This works
  // for anything movable with .data() and .size().
  geoarrow::ArrayBuilder builder(GEOARROW_TYPE_WKB);
  builder.SetBufferWrapped(1, offsets);
  builder.SetBufferWrapped(2, wkb);

  // Export to an ArrowArray
  struct ArrowArray array;
  builder.Finish(&array);

  // Read the ArrowArray using the visitor interface or the GeoArrowGeometry
  // interface.
  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array);

  reader.Visit(...)
}
```
