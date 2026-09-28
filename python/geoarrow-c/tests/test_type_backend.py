import pytest
from geoarrow.c import lib, types

gt = pytest.importorskip("geoarrow.types")
pa = pytest.importorskip("pyarrow")


@pytest.mark.parametrize(
    "spec",
    [
        gt.wkb(),
        gt.large_wkb(edge_type=gt.EdgeType.SPHERICAL, crs="EPSG:4326"),
        gt.wkt(),
        gt.large_wkt(),
        gt.wkb_view(),
        gt.wkt_view(),
        gt.point(),
        gt.linestring(
            dimensions=gt.Dimensions.XYZ,
            coord_type=gt.CoordType.INTERLEAVED,
        ),
        gt.box(dimensions=gt.Dimensions.XYM),
        gt.multipolygon(dimensions=gt.Dimensions.XYZM),
    ],
)
def test_type_spec_roundtrip(spec):
    schema = types.type_spec_to_arrow(spec)
    assert isinstance(schema, lib.SchemaHolder)
    assert types.arrow_to_type_spec(schema) == spec.with_defaults().canonicalize()


def test_to_type_spec_does_not_consume_holder():
    schema = types.type_spec_to_arrow(gt.point())

    assert types.arrow_to_type_spec(schema) == types.arrow_to_type_spec(schema)


def test_to_type_spec_short_circuits_type_spec():
    spec = gt.point()

    assert types.arrow_to_type_spec(spec) is spec


def test_from_type_spec_capsule_is_arrow_compatible():
    schema = types.type_spec_to_arrow(gt.point())
    schema_capsule = schema.__arrow_c_schema__()
    assert pa.DataType._import_from_c_capsule(schema_capsule) == pa.struct(
        [
            pa.field("x", pa.float64(), nullable=False),
            pa.field("y", pa.float64(), nullable=False),
        ]
    )


def test_to_type_spec_accepts_arrow_schema_provider():
    spec = gt.linestring(
        dimensions=gt.Dimensions.XYZ,
        coord_type=gt.CoordType.INTERLEAVED,
        edge_type=gt.EdgeType.SPHERICAL,
        crs="EPSG:4326",
    )
    arrow_type = spec.to_pyarrow()

    assert types.arrow_to_type_spec(arrow_type) == spec.with_defaults()


def test_to_type_spec_accepts_capsule():
    schema_capsule = gt.point().to_pyarrow().__arrow_c_schema__()

    assert types.arrow_to_type_spec(schema_capsule) == gt.point().with_defaults()


def test_lib_reexports_type_backend():
    assert lib.type_spec_to_arrow is types.type_spec_to_arrow
    assert lib.arrow_to_type_spec is types.arrow_to_type_spec
