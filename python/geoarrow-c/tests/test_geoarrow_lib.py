import pytest

import geoarrow.c as ga
import geoarrow.c.lib as lib

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


def test_c_vector_type_empty():
    empty = lib.CGeometryDataType()
    assert "Uninitialized CGeometryDataType" in repr(empty)

    with pytest.raises(ValueError):
        empty.id

    with pytest.raises(ValueError):
        empty.geometry_type

    with pytest.raises(ValueError):
        empty.dimensions

    with pytest.raises(ValueError):
        empty.coord_type

    with pytest.raises(ValueError):
        empty.extension_name

    with pytest.raises(ValueError):
        empty.extension_metadata

    with pytest.raises(ValueError):
        empty.edge_type

    with pytest.raises(ValueError):
        empty.crs_type

    with pytest.raises(ValueError):
        empty.crs

    with pytest.raises(ValueError):
        empty.with_geometry_type(0)

    with pytest.raises(ValueError):
        empty.with_dimensions(0)

    with pytest.raises(ValueError):
        empty.with_coord_type(0)

    with pytest.raises(ValueError):
        empty.with_edge_type(0)

    with pytest.raises(ValueError):
        empty.with_crs(bytes(), 0)

    with pytest.raises(ValueError):
        empty.to_schema()

    with pytest.raises(ValueError):
        empty.to_storage_schema()


def test_c_vector_type():
    type_obj = lib.CGeometryDataType.Make(
        ga.GeometryType.POINT, ga.Dimensions.XY, ga.CoordType.SEPARATE
    )

    assert type_obj.geometry_type == ga.GeometryType.POINT
    assert type_obj.dimensions == ga.Dimensions.XY
    assert type_obj.coord_type == ga.CoordType.SEPARATE
    assert type_obj.id == ga._lib.GeoArrowType.GEOARROW_TYPE_POINT

    schema = type_obj.to_schema()
    type_obj2 = lib.CGeometryDataType.FromExtension(schema)
    assert type_obj2 == type_obj

    pa_type = pa.DataType._import_from_c(schema._addr())
    pa_type_expected = pa.struct(
        [
            pa.field("x", pa.float64(), nullable=False),
            pa.field("y", pa.float64(), nullable=False),
        ]
    )

    # Depending on how the tests are run, the extension type might be
    # registered here.
    if isinstance(pa_type, pa.ExtensionType):
        assert pa_type.storage_type == pa_type_expected
    else:
        assert pa_type == pa_type_expected

    # Schema is now released, so we get an error
    with pytest.raises(ValueError):
        lib.CGeometryDataType.FromExtension(schema)

    schema_storage = lib.SchemaHolder()
    pa_type_expected._export_to_c(schema_storage._addr())
    type_obj3 = lib.CGeometryDataType.FromStorage(
        schema_storage, b"geoarrow.point", b""
    )

    assert type_obj3.geometry_type == ga.GeometryType.POINT
    assert type_obj3.dimensions == ga.Dimensions.XY
    assert type_obj3.coord_type == ga.CoordType.SEPARATE


def test_c_vector_type_with():
    type_obj = lib.CGeometryDataType.Make(
        lib.GeometryType.POINT, ga.Dimensions.XY, ga.CoordType.SEPARATE
    )

    type_linestring = type_obj.with_geometry_type(ga.GeometryType.LINESTRING)
    assert type_linestring.geometry_type == ga.GeometryType.LINESTRING

    type_xyz = type_obj.with_dimensions(ga.Dimensions.XYZ)
    assert type_xyz.dimensions == ga.Dimensions.XYZ

    type_interleaved = type_obj.with_coord_type(ga.CoordType.INTERLEAVED)
    assert type_interleaved.coord_type == ga.CoordType.INTERLEAVED

    type_spherical = type_obj.with_edge_type(ga.EdgeType.SPHERICAL)
    assert type_spherical.edge_type == ga.EdgeType.SPHERICAL

    type_crs = type_obj.with_crs(b"EPSG:1234", ga.CrsType.UNKNOWN)
    assert type_crs.crs_type == ga.CrsType.UNKNOWN
    assert type_crs.crs == b"EPSG:1234"


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


def test_builder():
    type_obj = lib.CGeometryDataType.Make(
        lib.GeometryType.LINESTRING, ga.Dimensions.XY, ga.CoordType.SEPARATE
    )
    schema = type_obj.to_schema()

    builder = lib.CBuilder(schema)
    builder.set_buffer_uint8(0, b"\xff")
    builder.set_buffer_int32(1, np.array([0, 3], dtype=np.int32))
    builder.set_buffer_double(2, np.array([0.0, 1.0, 2.0]))
    builder.set_buffer_double(3, np.array([3.0, 4.0, 5.0]))
    array = builder.finish()

    storage_schema = type_obj.to_storage_schema()
    storage = pa.Array._import_from_c(array._addr(), storage_schema._addr())
    storage.validate()
