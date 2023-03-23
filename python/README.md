# geoarrow for Python

Python bindings for geoarrow-cpp. These are in a preliminary state: see open issues
and tests/test_geoarrow.py for usage.

## Installation

Python bindings for nanoarrow are not yet available on PyPI. You can install via
URL (requires a C compiler):

```bash
python -m pip install "https://github.com/geoarrow/geoarrow-cpp/archive/refs/heads/main.zip#egg=geoarrow&subdirectory=python"
```

If you can import the namespace, you're good to go! The only reasonable interface to geoarrow currently depends on `pyarrow`, which you can import with:


```python
import geoarrow.pyarrow as ga
```

## Examples

You can create geoarrow types with `geoarrow.wkt()`, `geoarrow.wkb()`, and friends. Use the `.with_*()` modifiers to assign differing dimensions, geometry types, edge types, or coordinate reference system values; use `geoarrow.vector_type()` to specify everything at once.


```python
ga.wkb().with_crs('EPSG:1234')
```




    WkbType(DataType(binary))




```python
ga.linestring().with_dimensions(ga.Dimensions.XYZ)
```




    LinestringType(ListType(list<vertices: struct<x: double, y: double, z: double>>))



You can create arrays from iterables of Python objects using `geoarrow.array()`. String-like values are assumed to be (and are validated as) well-known text; bytes-like values are assumed to be (and are validated as) well-known binary.


```python
ga.array(['POINT (30 10)'], ga.wkt())
```




    <geoarrow.pyarrow.VectorArray object at 0x1231677c0>
    [
      "POINT (30 10)"
    ]



If you already have a `pyarrow.Array` or `pyarrow.ChunkedArray`, you can use `<Type>.wrap_array()`:


```python
import pyarrow as pa
existing_array = pa.array(['POINT (30 10)'])
ga.wkt().wrap_array(existing_array)
```




    <geoarrow.pyarrow.VectorArray object at 0x123167820>
    [
      "POINT (30 10)"
    ]



## Geopandas

You can convert from geopandas by going through well-known binary:


```python
import geopandas

url = "http://d2ad6b4ur7yvpq.cloudfront.net/naturalearth-3.3.0/ne_110m_land.geojson"
df = geopandas.read_file(url)
wkb = pa.array(df.geometry.to_wkb())

```

    /Library/Frameworks/Python.framework/Versions/3.9/lib/python3.9/site-packages/geopandas/_compat.py:123: UserWarning: The Shapely GEOS version (3.11.1-CAPI-1.17.1) is incompatible with the GEOS version PyGEOS was compiled with (3.10.1-CAPI-1.16.0). Conversions between both will be slow.
      warnings.warn(



```python
geoarrow_wkb = ga.wkb().with_crs(str(df.geometry.crs)).wrap_array(wkb)
```

## Building

Python bindings for nanoarrow are managed with setuptools[setuptools]. This means you
can build the project using:

```shell
git clone https://github.com/geoarrow/geoarrow-cpp.git
cd python
pip install -e .
```

Tests use [pytest][pytest]:

```shell
# Install dependencies
pip install -e .[test]

# Run tests
pytest -vvx
```

[pytest]: https://docs.pytest.org/
[setuptools]: https://setuptools.pypa.io/en/latest/index.html
