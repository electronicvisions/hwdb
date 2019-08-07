#!/usr/bin/env python

import unittest
import os
import shutil
import pyhwdb
import uuid
import tempfile

from pyhwdb_generate_slurm_license_file import\
    generate_license_strings, create_license_files
import pyhalco_hicann_v2 as coord


class Test_LicenseFileCreation(unittest.TestCase):
    def test_custom_db(self):
        db = pyhwdb.database()
        wafer = pyhwdb.WaferEntry()
        wafer.setup_type = coord.SetupType.BSSWafer
        wafer_coord = coord.Wafer(10)
        db.add_wafer_entry(wafer_coord, wafer)
        fpga = pyhwdb.FPGAEntry()
        fpga_coord = coord.FPGAGlobal(coord.FPGAOnWafer(0), wafer_coord)
        db.add_fpga_entry(fpga_coord, fpga)

        licenses, tres = generate_license_strings(db)
        self.assertEqual(licenses, "Licenses=W10F0,W10T8")
        self.assertEqual(
            tres, "AccountingStorageTRES=License/W10F0,License/W10T8"
        )

        try:
            tmpdirname = tempfile.mkdtemp()
            lic_tmp_file = os.path.join(tmpdirname, str(uuid.uuid4()))
            tres_tmp_file = os.path.join(tmpdirname, str(uuid.uuid4()))

            create_license_files(db, lic_tmp_file, tres_tmp_file)

            with open(lic_tmp_file, "r") as file:
                content = file.read()
                self.assertNotEqual(content.find("Licenses=W10F0,W10T8"), -1)

            with open(tres_tmp_file, "r") as file:
                content = file.read()
                self.assertNotEqual(content.find(
                    "AccountingStorageTRES=License/W10F0,License/W10T8"
                ), -1)
        finally:
            shutil.rmtree(tmpdirname)

    def test_empty_db(self):
        db = pyhwdb.database()
        licenses, tres = generate_license_strings(db)
        self.assertEqual(licenses, "Licenses=")

    @unittest.skipUnless(os.path.split(os.getcwd())[-1] == "hwdb", "assuming "
                         "test is executed with cwd == hwdb/ as done by waf")
    def test_convert_local_hwdb(self):
        path = os.path.join(os.getcwd(), "db.yaml")
        db = pyhwdb.database()
        self.assertTrue(os.path.exists(path))
        db.load(path)
        generate_license_strings(db)


if __name__ == "__main__":
    unittest.main()
