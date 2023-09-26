
# geoarrow-c for Python

The geoarrow-c Python package provides bindings to the geoarrow-c implementation of the [GeoArrow specification](https://github.com/geoarrow/geoarrow). Its primary purpose is to serve as a dependency to [geoarrow-python](https://github.com/geoarrow/geoarrow-python) where needed.

## Installation

Python bindings for geoarrow are available on PyPI and can be installed with:

```bash
pip install geoarrow-c
```

You can install a developement version with:

```bash
python -m pip install "git+https://github.com/geoarrow/geoarrow-c.git#egg=geoarrow-c&subdirectory=python/geoarrow-c"
```

If you can import the namespace, you're good to go!

```python
import geoarrow.c
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
