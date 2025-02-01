
# geoarrow-c for Python

The geoarrow-c Python package provides bindings to the geoarrow-c implementation of the [GeoArrow specification](https://github.com/geoarrow/geoarrow). Its primary purpose is to serve as a dependency to [geoarrow-python](https://github.com/geoarrow/geoarrow-python) where needed.

## Installation

Python bindings for geoarrow are available on PyPI and can be installed with:

```bash
pip install geoarrow-c
```

You can install a development version with:

```bash
python -m pip install "git+https://github.com/geoarrow/geoarrow-c.git#egg=geoarrow-c&subdirectory=python/geoarrow-c"
```

If you can import the namespace, you're good to go!

```python
import geoarrow.c
```

## Example

Most users should use the higher-level
[geoarrow-python](https://github.com/geoarrow/geoarrow-python) bindings.
If you would like to use the compute kernels exposed via `geoarrow-c`
directly, you will have to use the Arrow C Data interface to pass and
retrieve values.

```python
import geoarrow.pyarrow as ga
from geoarrow.c import lib

input_pyarrow = ga.array(["POINT (0 1)"])

type_in = lib.SchemaHolder()
input_pyarrow.type._export_to_c(type_in._addr())
array_in = lib.ArrayHolder()
input_pyarrow._export_to_c(array_in._addr())

kernel = lib.CKernel("box_agg".encode("UTF-8"))
type_out = kernel.start(type_in, bytes())
kernel.push_batch_agg(array_in)
array_out = kernel.finish_agg()

result = pyarrow.Array._import_from_c(array_out._addr(), type_out._addr())
```

## Building

Python bindings for nanoarrow are managed with [setuptools](https://setuptools.pypa.io/en/latest/index.html).
This means you can build the project using:

```shell
git clone https://github.com/geoarrow/geoarrow-c.git
cd python
pip install -e geoarrow-c/
```

Tests use [pytest](https://docs.pytest.org/):

```shell
# Install dependencies
cd python/geoarrow-c
pip install -e ".[test]"

# Run tests
pytest
```
