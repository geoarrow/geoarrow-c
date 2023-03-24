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




    WkbType(geoarrow.wkb <EPSG:1234>)




```python
ga.linestring().with_dimensions(ga.Dimensions.XYZ)
```




    LinestringType(geoarrow.linestring_z)



You can create arrays from iterables of Python objects using `geoarrow.array()`. String-like values are assumed to be (and are validated as) well-known text; bytes-like values are assumed to be (and are validated as) well-known binary.


```python
ga.array(['POINT (30 10)'], ga.wkt())
```




    VectorArray:WktType(geoarrow.wkt)[1]
    <POINT (30 10)>




If you already have a `pyarrow.Array` or `pyarrow.ChunkedArray`, you can use `<Type>.wrap_array()`:


```python
import pyarrow as pa
existing_array = pa.array(['POINT (30 10)'])
ga.wkt().wrap_array(existing_array)
```




    VectorArray:WktType(geoarrow.wkt)[1]
    <POINT (30 10)>




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
ga.wkb().with_crs(str(df.geometry.crs)).wrap_array(wkb)
```




    VectorArray:WkbType(geoarrow.wkb <EPSG:4326>)[127]
    <POLYGON ((-59.57209469261153 -80.0401787250963, -59.8658493719746...>
    <POLYGON ((-159.2081835601977 -79.49705942170873, -161.12760128481...>
    <POLYGON ((-45.15475765642103 -78.04706960058674, -43.920827806155...>
    <POLYGON ((-121.2115113938571 -73.50099049900605, -119.91885127829...>
    <POLYGON ((-125.5595664068953 -73.48135345473521, -124.03188187726...>
    ...117 values...
    <POLYGON ((51.13618655783139 80.54728017854103, 49.79368452332082 ...>
    <POLYGON ((99.93976000000001 78.88094, 97.75794 78.75620000000001,...>
    <POLYGON ((-87.02 79.66, -85.81435 79.3369, -87.18756 79.0393, -89...>
    <POLYGON ((-68.5 83.10632151676583, -65.82735 83.02801000000014, -...>
    <POLYGON ((-27.10046 83.51966, -20.84539 82.72669, -22.69182 82.34...>



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
