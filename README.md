
# geoarrow-c

[![Codecov test coverage](https://codecov.io/gh/geoarrow/geoarrow-c/branch/main/graph/badge.svg)](https://app.codecov.io/gh/geoarrow/geoarrow-c?branch=main)
[![Documentation](https://img.shields.io/badge/Documentation-dev-yellow)](https://geoarrow.github.io/geoarrow-c/dev)
[![nanoarrow on GitHub](https://img.shields.io/badge/GitHub-apache%2Farrow--nanoarrow-blue)](https://github.com/geoarrow/geoarrow-c)

The geoarrow C library is a geospatial type system and generic
coordinate-shuffling library written in C with bindings in C++,
R, and Python. The library supports well-known binary (WKB),
well-known text (ISO) and [geoarrow](https://github.com/geoarrow/geoarrow)
encodings as Arrow extension types with all possible mutual
conversions including support for Z, M, and ZM geometries.

The library currently implements version 0.1.0 of the GeoArrow
specification. The easiest way to get started with GeoArrow is to use
the [Python bindings](https://github.com/geoarrow/geoarrow-python),
which currently use geoarrow-c under the hood for most operations.
