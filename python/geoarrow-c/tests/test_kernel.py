import pytest
from geoarrow.c import Accumulator, ScalarKernel, lib

pa = pytest.importorskip("pyarrow")


class ArrayProvider:
    def __init__(self, array):
        self.array = array

    def __arrow_c_array__(self, requested_schema=None):
        return self.array.__arrow_c_array__(requested_schema)


class StreamProvider:
    def __init__(self, array):
        self.array = array

    def __arrow_c_stream__(self, requested_schema=None):
        return self.array.__arrow_c_stream__(requested_schema)


def test_scalar_kernel_execute_array():
    kernel = ScalarKernel("void", pa.int32())

    assert kernel.name == "void"
    assert (
        pa.DataType._import_from_c_capsule(kernel.type_in.__arrow_c_schema__())
        == pa.int32()
    )
    assert (
        pa.DataType._import_from_c_capsule(kernel.type_out.__arrow_c_schema__())
        == pa.null()
    )
    result = kernel.execute(ArrayProvider(pa.array([1, 2, 3])))
    assert isinstance(result, lib.ArrayHolder)
    assert pa.array(result) == pa.nulls(3)
    assert pa.array(kernel.execute(pa.array([4, 5]))) == pa.nulls(2)


def test_scalar_kernel_execute_chunked_array():
    kernel = ScalarKernel("void", pa.int32())
    array = pa.chunked_array([[1, 2], [3]], type=pa.int32())

    result_holder = kernel.execute(StreamProvider(array))
    result = pa.chunked_array(result_holder)

    assert isinstance(result_holder, lib.ArrayStreamHolder)
    assert isinstance(result, pa.ChunkedArray)
    assert result.type == pa.null()
    assert [len(chunk) for chunk in result.chunks] == [2, 1]


def test_aggregate_kernel_push_and_finish():
    kernel = Accumulator("void_agg", pa.int32())
    array = pa.chunked_array([[1, 2], [3]], type=pa.int32())

    assert kernel.push(StreamProvider(array)) is None
    assert pa.array(kernel.finish()) == pa.nulls(1)
    assert pa.array(kernel.finish()) == pa.nulls(1)
    assert kernel.push(pa.array([4])) is None
    assert pa.array(kernel.finish()) == pa.nulls(1)


def test_kernel_kind_is_explicit():
    with pytest.raises(ValueError, match="Aggregate kernel"):
        ScalarKernel("void_agg", pa.int32())
    with pytest.raises(ValueError, match="Scalar kernel"):
        Accumulator("void", pa.int32())


@pytest.mark.parametrize("kernel_cls", [ScalarKernel, Accumulator])
def test_kernel_requires_string_name(kernel_cls):
    with pytest.raises(TypeError, match="name.*str"):
        kernel_cls(b"void", pa.int32())


@pytest.mark.parametrize("kernel_cls", [ScalarKernel, Accumulator])
def test_kernel_requires_schema_provider(kernel_cls):
    name = "void" if kernel_cls is ScalarKernel else "void_agg"
    with pytest.raises(ValueError):
        kernel_cls(name, "int32")


def test_scalar_kernel_requires_array():
    kernel = ScalarKernel("void", pa.int32())
    with pytest.raises(TypeError, match="__arrow_c_array__"):
        kernel.execute([1, 2, 3])


def test_kernel_options_omit_none():
    kernel = ScalarKernel("void", pa.int32(), unused=None)
    assert pa.array(kernel.execute(pa.array([1]))) == pa.nulls(1)
