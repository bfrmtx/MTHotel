"""ats package: read/write Metronix ats binary headers and the atss json header.

The binary ats header is a fixed 1024 byte little endian, packed structure.
If numslices is not zero, that many 32 byte slice headers follow the header.

This package uses only the standard library (struct) - no pycstruct dependency.
"""

from . import ats_base
from . import json_header

from .ats_base import (
    ATS_HEADER_FIELDS,
    ATS_SLICE_FIELDS,
    ATS_HEADER_SIZE,
    ATS_SLICE_SIZE,
    create_atsheader,
    create_ats_slice_header,
    read_atsheader,
    write_atsheader,
    binary_ats_samples,
)

__all__ = [
    "ats_base",
    "json_header",
    "ATS_HEADER_FIELDS",
    "ATS_SLICE_FIELDS",
    "ATS_HEADER_SIZE",
    "ATS_SLICE_SIZE",
    "create_atsheader",
    "create_ats_slice_header",
    "read_atsheader",
    "write_atsheader",
    "binary_ats_samples",
]
