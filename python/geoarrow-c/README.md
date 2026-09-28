
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
The Python package also provides thin Arrow PyCapsule protocol wrappers around
the compute kernels exposed by `geoarrow-c`:

```python
import geoarrow.pyarrow as ga
from geoarrow.c import AggregateKernel

input_pyarrow = ga.array(["POINT (0 1)"])
kernel = AggregateKernel("box_agg", input_pyarrow.type)
kernel.push(input_pyarrow)
result = kernel.finish()
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
