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




Importing `geoarrow.pyarrow` will register the geoarrow extension types with pyarrow such that you can read/write Arrow streams, Arrow files, and Parquet that contains Geoarrow extension types. A number of these files are available from the [geoarrow-data](https://github.com/paleolimbot/geoarrow-data) repository.

## Geopandas

You can convert from geopandas using `geoarrow.array()`:


```python
import geopandas

url = "https://github.com/paleolimbot/geoarrow-data/releases/download/v0.0.1/nshn_basin_line.gpkg"
df = geopandas.read_file(url)
wkb_array = ga.array(df.geometry)
wkb_array

```

    /Library/Frameworks/Python.framework/Versions/3.9/lib/python3.9/site-packages/geopandas/_compat.py:123: UserWarning: The Shapely GEOS version (3.11.1-CAPI-1.17.1) is incompatible with the GEOS version PyGEOS was compiled with (3.10.1-CAPI-1.16.0). Conversions between both will be slow.
      warnings.warn(
    Warning 1: File /vsimem/e0acae235392478fb4cf4bec24c55045 has GPKG application_id, but non conformant file extension





    VectorArray:WkbType(geoarrow.wkb <{"$schema":"https://proj.org/schem...>)[255]
    <MULTILINESTRING Z ((648686.0197000001 5099181.984099999 0, 648626...>
    <MULTILINESTRING Z ((687687.8200000003 5117029.181600001 0, 686766...>
    <MULTILINESTRING Z ((631355.5193999996 5122892.2849 0, 631364.3433...>
    <MULTILINESTRING Z ((665166.0199999996 5138641.9825 0, 665146.0199...>
    <MULTILINESTRING Z ((673606.0199999996 5162961.9823 0, 673606.0199...>
    ...245 values...
    <MULTILINESTRING Z ((681672.6200000001 5078601.5823 0, 681866.0199...>
    <MULTILINESTRING Z ((414867.9170000004 5093040.8807 0, 414793.8169...>
    <MULTILINESTRING Z ((414867.9170000004 5093040.8807 0, 414829.7170...>
    <MULTILINESTRING Z ((414867.9170000004 5093040.8807 0, 414937.2170...>
    <MULTILINESTRING Z ((648686.0197000001 5099181.984099999 0, 648866...>



By default, `geoarrow.array()` performs the fewest transformations required, which in
this case means we get a well-known binary representation in Arrow form. To get a
geoarrow-encoded version, use `.as_geoarrow()`:


```python
geoarrow_array = wkb_array.as_geoarrow(ga.multilinestring().with_dimensions(ga.Dimensions.XYZ))
geoarrow_array
```




    MultiLinestringArray:MultiLinestringType(geoarrow.multilinestring_z <{"$schema":"https://proj.org/schem...>)[255]
    <MULTILINESTRING Z ((648686.0197000001 5099181.984099999 0, 648626...>
    <MULTILINESTRING Z ((687687.8200000003 5117029.181600001 0, 686766...>
    <MULTILINESTRING Z ((631355.5193999996 5122892.2849 0, 631364.3433...>
    <MULTILINESTRING Z ((665166.0199999996 5138641.9825 0, 665146.0199...>
    <MULTILINESTRING Z ((673606.0199999996 5162961.9823 0, 673606.0199...>
    ...245 values...
    <MULTILINESTRING Z ((681672.6200000001 5078601.5823 0, 681866.0199...>
    <MULTILINESTRING Z ((414867.9170000004 5093040.8807 0, 414793.8169...>
    <MULTILINESTRING Z ((414867.9170000004 5093040.8807 0, 414829.7170...>
    <MULTILINESTRING Z ((414867.9170000004 5093040.8807 0, 414937.2170...>
    <MULTILINESTRING Z ((648686.0197000001 5099181.984099999 0, 648866...>



If you'd like to do some of your own processing, you can access buffers as numpy arrays
using `.geobuffers()`:


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
            104, 105, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 117,
            118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
            131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
            144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156,
            157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
            170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
            183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195,
            196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
            209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
            222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
            235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
            248, 249, 250, 251, 252, 253, 254, 255, 256], dtype=int32),
     array([    0,   405,  1095,  1728,  2791,  3242,  3952,  4025,  4709,
             6366,  6368,  6373,  6375,  6377,  6381,  6384,  6386,  6395,
             6397,  6399,  6401,  6403,  6409,  6412,  6414,  6418,  6420,
             6423,  6426,  6428,  6432,  6445,  6448,  6450,  6457,  6462,
             6464,  6466,  6476,  6479,  6482,  6484,  6486,  6493,  6501,
             6503,  6506,  6509,  6511,  6513,  6515,  6517,  6519,  6521,
             6523,  6525,  6527,  6529,  6531,  6534,  6653,  6943,  7047,
             7086,  7131,  7140,  7220,  7230,  7233,  7285,  7420,  7450,
             7460,  7485,  7492,  7494,  7497,  7499,  7501,  7504,  7506,
             7568,  7629,  7633,  7635,  7689,  7778,  7780,  7783,  7786,
             7788,  7792,  7795,  7798,  7800,  7805,  8366,  8368,  8372,
             8376,  8380,  8382,  8385,  8388,  8392,  8395,  8398,  8400,
             8402,  8404,  8406,  8408,  8411,  8413,  8416,  8420,  8435,
             8437,  8440,  8443,  8446,  8453,  8456,  8458,  8460,  8463,
             8465,  8468,  8475,  8478,  8488,  8490,  8493,  8495,  8498,
             8502,  8505,  8508,  8510,  8513,  8515,  8517,  8529,  8531,
             8534,  8539,  8546,  8549,  8555,  8564,  8566,  8569,  8571,
             8855,  9156,  9957, 10118, 10179, 10380, 10561, 10862, 11111,
            11212, 11352, 11765, 11848, 11949, 12043, 12261, 12742, 12943,
            13244, 13285, 13306, 13527, 13548, 13609, 13650, 13771, 13823,
            13994, 14407, 14982, 15631, 16161, 16655, 17779, 17879, 17918,
            18382, 18783, 20028, 21223, 21344, 21492, 21601, 21686, 21952,
            22278, 22756, 23118, 23391, 24186, 24379, 24655, 25164, 25486,
            26279, 26741, 27295, 28306, 28331, 28844, 29429, 30233, 31317,
            31472, 33015, 33061, 33245, 33415, 33715, 33866, 34589, 34834,
            35317, 35474, 35595, 35997, 36210, 36770, 37053, 37074, 37434,
            37874, 38724, 39846, 40443, 41369, 42169, 43122, 43289, 43539,
            44133, 44771, 44781, 44890, 46058, 46534, 46988, 47466, 48053,
            48712, 49007, 49221, 49275, 49494], dtype=int32),
     array([648686.0197, 648626.0187, 648586.0197, ..., 658335.0659,
            658341.5039, 658351.4199]),
     array([5099181.9841, 5099181.9841, 5099161.9831, ..., 5099975.8904,
            5099981.8684, 5099991.9824]),
     array([0., 0., 0., ..., 0., 0., 0.])]




```python
assert geoarrow_array.as_wkt() == wkb_array.as_wkt()
```

## Building

Python bindings for nanoarrow are managed with [setuptools](https://setuptools.pypa.io/en/latest/index.html).
This means you can build the project using:

```shell
git clone https://github.com/geoarrow/geoarrow-cpp.git
cd python
pip install -e .
```

Tests use [pytest](https://docs.pytest.org/):

```shell
# Install dependencies
pip install -e .[test]

# Run tests
pytest -vvx
```
