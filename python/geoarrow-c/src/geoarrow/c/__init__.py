"""
The root import for geoarrow. This import is intended for those working
with geoarrow at a low level; most users should use the pyarrow integration
as ``import geoarrow.pyarrow as ga``.

Examples
--------

>>> import geoarrow.c as ga
"""

from geoarrow.c._version import __version__, __version_tuple__  # NOQA: F401
from geoarrow.c.types import arrow_to_type_spec, type_spec_to_arrow

__all__ = ["arrow_to_type_spec", "type_spec_to_arrow"]
