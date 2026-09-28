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


def test_kernel_void():
    kernel = lib.CKernel(b"void")

    schema_in = lib.SchemaHolder()
    pa.int32()._export_to_c(schema_in._addr())

    schema_out = kernel.start(schema_in, b"")
    schema_out_pa = pa.DataType._import_from_c(schema_out._addr())
    assert schema_out_pa == pa.null()

    array_in = lib.ArrayHolder()
    pa.array([1, 2, 3], pa.int32())._export_to_c(array_in._addr())
    array_out = kernel.push_batch(array_in)
    array_out_pa = pa.Array._import_from_c(array_out._addr(), schema_out_pa)
    assert array_out_pa == pa.array([None, None, None], pa.null())


def test_kernel_void_agg():
    kernel = lib.CKernel(b"void_agg")

    schema_in = lib.SchemaHolder()
    pa.int32()._export_to_c(schema_in._addr())

    schema_out = kernel.start(schema_in, b"")
    schema_out_pa = pa.DataType._import_from_c(schema_out._addr())
    assert schema_out_pa == pa.null()

    array_in = lib.ArrayHolder()
    pa.array([1, 2, 3], pa.int32())._export_to_c(array_in._addr())
    assert kernel.push_batch_agg(array_in) is None

    array_out = kernel.finish_agg()
    array_out_pa = pa.Array._import_from_c(array_out._addr(), schema_out_pa)
    assert array_out_pa == pa.array([None], pa.null())


def test_kernel_init_error():
    with pytest.raises(lib.GeoArrowCException):
        lib.CKernel(b"not_a_kernel")

    with pytest.raises(TypeError):
        lib.CKernel()

    with pytest.raises(TypeError):
        lib.CKernel(None)
