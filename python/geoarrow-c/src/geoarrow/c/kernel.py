"""Higher-level wrappers around GeoArrow C compute kernels.

These wrappers accept and return Arrow PyCapsule protocol providers while
keeping the current ``CKernel`` binding as an implementation detail. Scalar and
aggregate kernels are separate because they have different execution
lifecycles and are expected to use separate C ABIs in the future.
"""

import sys

from geoarrow.c import lib


class _Kernel:
    def __init__(self, name, type_in, options):
        if not isinstance(name, str):
            raise TypeError("Expected `name` to be a str")

        self._name = name
        self._kernel = lib.CKernel(name.encode("UTF-8"))

        self._type_in = lib.SchemaHolder.from_arrow_c_schema(type_in)
        self._type_out_schema = self._kernel.start(
            self._type_in, _pack_options(options)
        )

    @property
    def name(self):
        """The C kernel name."""
        return self._name

    @property
    def type_in(self):
        """The input ``SchemaHolder``."""
        return self._type_in

    @property
    def type_out(self):
        """The output ``SchemaHolder``."""
        return self._type_out_schema

    def _execute_array(self, array):
        array_in = lib.ArrayHolder.from_arrow_c_array(array)
        return self._kernel.push_batch(array_in)


class ScalarKernel(_Kernel):
    """A scalar GeoArrow C kernel.

    Parameters
    ----------
    name : str
        The registered C kernel name.
    type_in : __arrow_c_schema__ provider
        The input array type. The current GeoArrow C kernel ABI accepts one
        input; a future scalar-kernel ABI may support multiple inputs.
    **options
        Kernel options. ``None`` values are omitted.
    """

    def __init__(self, name, type_in, **options):
        _validate_name(name)
        if name.endswith("_agg"):
            raise ValueError("Aggregate kernel names must be used with AggregateKernel")
        super().__init__(name, type_in, options)

    def execute(self, array):
        """Execute the kernel for an Arrow array or array stream."""
        if hasattr(array, "__arrow_c_array__"):
            return self._execute_array(array)
        if hasattr(array, "__arrow_c_stream__"):
            stream_in = lib.ArrayStreamHolder.from_arrow_c_stream(array)
            arrays_out = []
            while True:
                array_in = stream_in.get_next()
                if array_in is None:
                    break
                arrays_out.append(self._execute_array(array_in))

            return lib.ArrayStreamHolder.from_arrays(self._type_out_schema, arrays_out)

        raise TypeError(
            "Expected an __arrow_c_array__ or __arrow_c_stream__ provider, "
            f"got {type(array)}"
        )


class AggregateKernel(_Kernel):
    """An aggregate GeoArrow C kernel with a push/finish lifecycle.

    Parameters
    ----------
    name : str
        The registered C aggregate kernel name.
    type_in : __arrow_c_schema__ provider
        The input array type.
    **options
        Kernel options. ``None`` values are omitted.
    """

    def __init__(self, name, type_in, **options):
        _validate_name(name)
        if not name.endswith("_agg"):
            raise ValueError("Scalar kernel names must be used with ScalarKernel")
        super().__init__(name, type_in, options)

    def push(self, array):
        """Push an Arrow array or all chunks of an array stream."""
        if hasattr(array, "__arrow_c_array__"):
            array_in = lib.ArrayHolder.from_arrow_c_array(array)
            self._kernel.push_batch_agg(array_in)
            return

        if hasattr(array, "__arrow_c_stream__"):
            stream_in = lib.ArrayStreamHolder.from_arrow_c_stream(array)
            while True:
                array_in = stream_in.get_next()
                if array_in is None:
                    break
                self._kernel.push_batch_agg(array_in)
            return

        raise TypeError(
            "Expected an __arrow_c_array__ or __arrow_c_stream__ provider, "
            f"got {type(array)}"
        )

    def finish(self):
        """Finish the aggregation and return its result array."""
        return self._kernel.finish_agg()


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


__all__ = ["AggregateKernel", "ScalarKernel"]
