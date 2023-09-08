
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

The library and bindings are still in draft form pending an initial
release of the geoarrow specification.

## Get started in Python

```python
import geoarrow.pyarrow as ga

ga.point()
# PointType(geoarrow.point)
ga.point().storage_type
# StructType(struct<x: double, y: double>)
ga.as_geoarrow(["POINT (0 1)"])
# PointArray:PointType(geoarrow.point)[1]
# <POINT (0 1)>
```
