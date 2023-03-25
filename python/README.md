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
wkb_array = ga.array(df.geometry)
wkb_array

```

    /Library/Frameworks/Python.framework/Versions/3.9/lib/python3.9/site-packages/geopandas/_compat.py:123: UserWarning: The Shapely GEOS version (3.11.1-CAPI-1.17.1) is incompatible with the GEOS version PyGEOS was compiled with (3.10.1-CAPI-1.16.0). Conversions between both will be slow.
      warnings.warn(





    VectorArray:WkbType(geoarrow.wkb <{"$schema":"https://proj.org/schem...>)[127]
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




```python
geoarrow_array = wkb_array.as_geoarrow(ga.polygon())
geoarrow_array
```




    PolygonArray:PolygonType(geoarrow.polygon <{"$schema":"https://proj.org/schem...>)[127]
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




```python
geoarrow_array.geobuffers()
```




    [None,
     array([  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,
             13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
             26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
             39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,
             52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
             65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,
             78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
             91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103,
            104, 105, 106, 107, 108, 109, 110, 111, 112, 114, 115, 116, 117,
            118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128], dtype=int32),
     array([   0,   13,   25,   47,   57,   65,   79,  105,  664,  688,  698,
             707,  724,  754,  790,  803,  812,  820,  825,  830,  837,  886,
            1110, 1116, 1123, 1131, 1138, 1153, 1163, 1172, 1181, 1188, 1211,
            1218, 1229, 1251, 1257, 1266, 1280, 1347, 1392, 1405, 1440, 1486,
            1513, 1523, 1531, 1543, 1552, 1561, 1575, 1581, 1614, 1623, 1634,
            1670, 1680, 1697, 1706, 1711, 1720, 1727, 1769, 1777, 1786, 1792,
            1799, 1811, 1823, 1834, 1845, 1855, 1892, 1899, 1915, 1924, 1932,
            1948, 1981, 1995, 2017, 2029, 2035, 2044, 2088, 2095, 2103, 2110,
            2122, 2142, 2162, 2171, 2195, 2206, 2211, 2219, 3151, 3196, 3202,
            3213, 3286, 3301, 3309, 3320, 3337, 3344, 3352, 3361, 3374, 3396,
            3422, 3450, 3462, 4761, 4813, 4820, 4827, 4835, 4842, 4851, 4862,
            4869, 4887, 4899, 4914, 4925, 4946, 5011, 5143], dtype=int32),
     array([-59.57209469, -59.86584937, -60.15965573, ..., -38.62214   ,
            -35.08787   , -27.10046   ]),
     array([-80.04017873, -80.54965667, -81.00032684, ...,  83.54905   ,
             83.64513   ,  83.51966   ])]



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
