"""
The root import for geoarrow. This import is intended for those working
with geoarrow at a low level; most users should use the pyarrow integration
as ``import geoarrow.c.pyarrow as ga``.

Examples
--------

>>> import geoarrow as ga
"""


from .lib import GeometryType, Dimensions, CoordType, EdgeType, CrsType
