from . import _lib
from ._lib import (
    ArrayHolder,
    CKernel,
    GeoArrowCException,
    SchemaHolder,
)

_ENCODING_TO_C_TYPE = {
    1: _lib.GEOARROW_TYPE_WKB,
    2: _lib.GEOARROW_TYPE_LARGE_WKB,
    3: _lib.GEOARROW_TYPE_WKT,
    4: _lib.GEOARROW_TYPE_LARGE_WKT,
    5: _lib.GEOARROW_TYPE_WKB_VIEW,
    6: _lib.GEOARROW_TYPE_WKT_VIEW,
}

_C_TYPE_TO_ENCODING = {value: key for key, value in _ENCODING_TO_C_TYPE.items()}


def from_type_spec(spec):
    """Create an Arrow schema capsule from a ``geoarrow.types.TypeSpec``."""
    import geoarrow.types as gt

    spec = gt.type_spec(spec).with_defaults().canonicalize()
    metadata = spec.extension_metadata().encode("UTF-8")

    if spec.encoding == gt.Encoding.GEOARROW:
        c_type = _lib.CGeometryDataType.Make(
            spec.geometry_type.value,
            spec.dimensions.value,
            spec.coord_type.value,
            metadata,
        )
    else:
        try:
            type_id = _ENCODING_TO_C_TYPE[spec.encoding.value]
        except KeyError:
            raise ValueError(
                f"Unsupported GeoArrow encoding: {spec.encoding}"
            ) from None

        c_type = _lib.CGeometryDataType.MakeType(type_id, metadata)

    return c_type.to_schema_capsule()


def to_type_spec(arrow_pycapsule_schema):
    """Create a ``geoarrow.types.TypeSpec`` from an Arrow schema capsule."""
    import geoarrow.types as gt

    c_type = _lib.CGeometryDataType.FromExtensionCapsule(arrow_pycapsule_schema)
    metadata_spec = gt.TypeSpec.from_extension_metadata(
        c_type.extension_metadata.decode("UTF-8")
    )

    encoding_value = _C_TYPE_TO_ENCODING.get(c_type.id)
    if encoding_value is None:
        encoding = gt.Encoding.GEOARROW
        geometry_type = gt.GeometryType(c_type.geometry_type)
        dimensions = gt.Dimensions(c_type.dimensions)
        coord_type = gt.CoordType(c_type.coord_type)
    else:
        encoding = gt.Encoding(encoding_value)
        geometry_type = gt.GeometryType.GEOMETRY
        dimensions = gt.Dimensions.UNKNOWN
        coord_type = gt.CoordType.UNSPECIFIED

    return gt.TypeSpec(
        encoding,
        geometry_type,
        dimensions,
        coord_type,
        metadata_spec.edge_type,
        metadata_spec.crs,
    )


__all__ = [
    "ArrayHolder",
    "CKernel",
    "GeoArrowCException",
    "SchemaHolder",
    "from_type_spec",
    "to_type_spec",
]
