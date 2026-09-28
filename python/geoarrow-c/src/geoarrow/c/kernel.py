"""High-level wrappers around GeoArrow C compute kernels."""

import sys

from geoarrow.c import lib


class ScalarFunction:
    """A callable scalar GeoArrow C function.

    Each call creates a new kernel using the input array or stream's schema.

    Parameters
    ----------
    name : str
        The registered C kernel name.
    **options
        Kernel options. ``None`` values are omitted.
    """

    def __init__(self, name, **options):
        _validate_name(name)
        if name.endswith("_agg"):
            raise ValueError(
                "Aggregate kernel names must be used with AggregateFunction"
            )
        self._name = name
        self._options = _pack_options(options)

    @property
    def name(self):
        """The C kernel name."""
        return self._name

    def __call__(self, array):
        """Return an Arrow array or stream matching the input's batch structure."""
        array_in = _import_input(array)
        kernel = lib.CKernel(self._name.encode("UTF-8"))
        type_out = kernel.start(array_in.get_schema(), self._options)
        if isinstance(array_in, lib.ArrayHolder):
            return kernel.push_batch(array_in)

        arrays_out = []
        while True:
            batch = array_in.get_next()
            if batch is None:
                break
            arrays_out.append(kernel.push_batch(batch))

        return lib.ArrayStreamHolder.from_arrays(type_out, arrays_out)


class AggregateFunction:
    """A callable aggregate GeoArrow C function.

    Each call creates a new kernel using the input array or stream's schema,
    accumulates all input batches, and returns the aggregate result array.

    Parameters
    ----------
    name : str
        The registered C aggregate kernel name.
    **options
        Kernel options. ``None`` values are omitted.
    """

    def __init__(self, name, **options):
        _validate_name(name)
        if not name.endswith("_agg"):
            raise ValueError("Scalar kernel names must be used with ScalarFunction")
        self._name = name
        self._options = _pack_options(options)

    @property
    def name(self):
        """The C kernel name."""
        return self._name

    def __call__(self, array):
        """Aggregate an Arrow array or stream and return its result array."""
        array_in = _import_input(array)
        kernel = lib.CKernel(self._name.encode("UTF-8"))
        kernel.start(array_in.get_schema(), self._options)
        if isinstance(array_in, lib.ArrayHolder):
            kernel.push_batch_agg(array_in)
        else:
            while True:
                batch = array_in.get_next()
                if batch is None:
                    break
                kernel.push_batch_agg(batch)

        return kernel.finish_agg()


def _import_input(array):
    if hasattr(array, "__arrow_c_array__"):
        return lib.ArrayHolder.from_arrow_c_array(array)
    if hasattr(array, "__arrow_c_stream__"):
        return lib.ArrayStreamHolder.from_arrow_c_stream(array)

    raise TypeError(
        "Expected an __arrow_c_array__ or __arrow_c_stream__ provider, "
        f"got {type(array)}"
    )


def _validate_name(name):
    if not isinstance(name, str):
        raise TypeError("Expected `name` to be a str")


def _pack_options(options):
    options = {key: value for key, value in options.items() if value is not None}
    if not options:
        return b""

    packed = len(options).to_bytes(4, sys.byteorder, signed=True)
    for key, value in options.items():
        key_bytes = str(key).encode("UTF-8")
        packed += len(key_bytes).to_bytes(4, sys.byteorder, signed=True)
        packed += key_bytes

        value_bytes = str(value).encode("UTF-8")
        packed += len(value_bytes).to_bytes(4, sys.byteorder, signed=True)
        packed += value_bytes

    return packed


__all__ = ["AggregateFunction", "ScalarFunction"]
