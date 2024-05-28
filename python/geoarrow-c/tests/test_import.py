import geoarrow.c as ga

# This is mostly a naive sanity check that the native extension can be loaded
# on platforms where there is no pyarrow.


def test_enums():
    assert ga.CoordType.UNKNOWN == 0
    assert ga.CrsType.UNKNOWN == 1
    assert ga.Dimensions.UNKNOWN == 0
    assert ga.EdgeType.PLANAR == 0
    assert ga.GeometryType.GEOMETRY == 0
