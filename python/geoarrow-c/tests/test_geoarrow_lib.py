import pytest
from geoarrow.c import lib

np = pytest.importorskip("numpy")
pa = pytest.importorskip("pyarrow")


def test_schema_holder():
    holder = lib.SchemaHolder()
    assert holder.is_valid() is False
    with pytest.raises(ValueError):
        holder.release()

    pa.int32()._export_to_c(holder._addr())
    assert holder.is_valid() is True
    holder.release()


def test_array_holder():
    holder = lib.ArrayHolder()
    assert holder.is_valid() is False
    with pytest.raises(ValueError):
        holder.release()

    pa.array([1, 2, 3], pa.int32())._export_to_c(holder._addr())
    assert holder.is_valid() is True
    holder.release()


def test_array_holder_arrow_c_array_roundtrip():
    holder = lib.ArrayHolder.from_arrow_c_array(pa.array([1, 2, 3]))

    assert pa.array(holder) == pa.array([1, 2, 3])
    assert holder.is_valid() is False


def test_array_stream_holder_arrow_c_stream_roundtrip():
    holder = lib.ArrayStreamHolder.from_arrow_c_stream(
        pa.chunked_array([[1, 2], [3]], type=pa.int64())
    )

    assert pa.array(holder.get_next()) == pa.array([1, 2])
    assert pa.array(holder.get_next()) == pa.array([3])
    assert holder.get_next() is None

    holder = lib.ArrayStreamHolder.from_arrow_c_stream(
        pa.chunked_array([[1, 2], [3]], type=pa.int64())
    )
    assert pa.chunked_array(holder) == pa.chunked_array([[1, 2], [3]], type=pa.int64())
    assert holder.is_valid() is False


def test_array_stream_holder_from_arrays_owns_schema():
    schema = lib.SchemaHolder.from_arrow_c_schema(pa.int32())
    expected = pa.array([1, None, 3], type=pa.int32())
    array = lib.ArrayHolder.from_arrow_c_array(expected)
    stream = lib.ArrayStreamHolder.from_arrays(schema, [array])

    schema.release()
    batch = stream.get_next()
    assert stream.get_next() is None
    stream.release()

    assert pa.array(batch) == expected


def test_kernel_void():
    kernel = lib.CKernel(b"void")

    schema_in = lib.SchemaHolder.from_arrow_c_schema(pa.int32())

    schema_out = kernel.start(schema_in, b"")
    schema_out_pa = pa.DataType._import_from_c_capsule(schema_out.__arrow_c_schema__())
    assert schema_out_pa == pa.null()

    array_in = lib.ArrayHolder.from_arrow_c_array(pa.array([1, 2, 3], pa.int32()))
    array_out = kernel.push_batch(array_in)
    array_out_pa = pa.array(array_out)
    assert array_out_pa == pa.array([None, None, None], pa.null())


def test_kernel_void_agg():
    kernel = lib.CKernel(b"void_agg")

    schema_in = lib.SchemaHolder.from_arrow_c_schema(pa.int32())

    schema_out = kernel.start(schema_in, b"")
    schema_out_pa = pa.DataType._import_from_c_capsule(schema_out.__arrow_c_schema__())
    assert schema_out_pa == pa.null()

    array_in = lib.ArrayHolder.from_arrow_c_array(pa.array([1, 2, 3], pa.int32()))
    assert kernel.push_batch_agg(array_in) is None

    array_out = kernel.finish_agg()
    array_out_pa = pa.array(array_out)
    assert array_out_pa == pa.array([None], pa.null())


def test_kernel_init_error():
    with pytest.raises(lib.GeoArrowCException):
        lib.CKernel(b"not_a_kernel")

    with pytest.raises(TypeError):
        lib.CKernel()

    with pytest.raises(TypeError):
        lib.CKernel(None)
