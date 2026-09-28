import pytest

from geoarrow.c import lib


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
    schema_capsule = lib.from_type_spec(spec)
    assert lib.to_type_spec(schema_capsule) == spec.with_defaults().canonicalize()


def test_to_type_spec_does_not_consume_capsule():
    schema_capsule = lib.from_type_spec(gt.point())

    assert lib.to_type_spec(schema_capsule) == lib.to_type_spec(schema_capsule)


def test_from_type_spec_capsule_is_arrow_compatible():
    schema_capsule = lib.from_type_spec(gt.point())
    assert pa.DataType._import_from_c_capsule(schema_capsule) == pa.struct(
        [
            pa.field("x", pa.float64(), nullable=False),
            pa.field("y", pa.float64(), nullable=False),
        ]
    )


def test_to_type_spec_accepts_arrow_compatible_capsule():
    spec = gt.linestring(
        dimensions=gt.Dimensions.XYZ,
        coord_type=gt.CoordType.INTERLEAVED,
        edge_type=gt.EdgeType.SPHERICAL,
        crs="EPSG:4326",
    )
    schema_capsule = spec.to_pyarrow().__arrow_c_schema__()

    assert lib.to_type_spec(schema_capsule) == spec.with_defaults()
