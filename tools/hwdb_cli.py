#!/usr/bin/env python
"""
hwdb command line interface

Usage: hwdb_cli.py W33D0 to_be_powered
"""

import inspect
import argparse

import pyhalco_hicann_v2 as C
import pyhwdb

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='hwdb command line interface')
    parser.add_argument("coordinate_short_format", help="short format of Wafer, FPGAGlobal or HICANNGlobal; eg. W33, W5F2, W12H100")
    parser.add_argument("entry_property", help="query given property of the hwdb entry of coordinate_short_format")
    parser.add_argument("--hwdb", default=pyhwdb.database.get_default_path(), help="path to hwdb")

    args = parser.parse_args()

    coord = C.from_string(args.coordinate_short_format)

    db = pyhwdb.database()
    db.load(args.hwdb)

    if isinstance(coord, C.Wafer):
        hwdb_entry = db.get_wafer_entry(coord)
    elif isinstance(coord, C.FPGAGlobal):
        hwdb_entry = db.get_fpga_entry(coord)
    elif isinstance(coord, C.HICANNGlobal):
        hwdb_entry = db.get_hicann_entry(coord)
    elif isinstance(coord, C.DNCGlobal):
        hwdb_entry = db.get_reticle_entry(coord)
    else:
        raise RuntimeError("No hwdb entry assigned to {}. ")

    try:
        print getattr(hwdb_entry, args.entry_property)
    except AttributeError as e:
        print "{}, available properties are: {}".format(e, ', '.join(i for i in dir(hwdb_entry) if not i.startswith('__')))
        exit(1)
