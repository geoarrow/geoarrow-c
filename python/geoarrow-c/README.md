
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

## Type specification integration

The `arrow_to_type_spec()` and `type_spec_to_arrow()` functions bridge
`geoarrow.types.TypeSpec` objects and the Arrow C Data Interface. This
integration requires the `geoarrow-types` package but does not require a
particular Arrow implementation.

`type_spec_to_arrow()` returns a `SchemaHolder` implementing
`__arrow_c_schema__`:

```python
import geoarrow.types as gt
from geoarrow.c import type_spec_to_arrow

type_spec = gt.linestring(
    dimensions=gt.Dimensions.XYZ,
    coord_type=gt.CoordType.INTERLEAVED,
    crs="EPSG:4326",
)
schema = type_spec_to_arrow(type_spec)
schema_capsule = schema.__arrow_c_schema__()
```

`arrow_to_type_spec()` accepts a `SchemaHolder` or any other
`__arrow_c_schema__` provider and reconstructs the corresponding `TypeSpec`:

```python
from geoarrow.c import arrow_to_type_spec

roundtripped = arrow_to_type_spec(schema)
assert roundtripped == type_spec.with_defaults().canonicalize()
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
