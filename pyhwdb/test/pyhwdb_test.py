#!/usr/bin/env python

import unittest
import os
import pyhwdb

IS_PYPLUSPLUS = None
try:
    import pyhalco_hicann_v2 as coord
    IS_PYPLUSPLUS = True
except ImportError:
    import pyhalco_common as coord
    IS_PYPLUSPLUS = False

class Test_Pyhwdb(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(Test_Pyhwdb, self).__init__(*args, **kwargs)
        if IS_PYPLUSPLUS:
            self.WAFER_COORD = coord.Wafer(10)
            self.WAFER_SETUP_TYPE = coord.SetupType.BSSWafer
            self.WAFER_MACU_IP = coord.IPv4.from_string("192.168.10.120")
            self.FPGA_COORD = coord.FPGAOnWafer(0)
            self.FPGA_HIGHSPEED = True
        self.FPGA_IP = coord.IPv4.from_string("192.168.10.1")
        if IS_PYPLUSPLUS:
            self.RETICLE_COORD = coord.DNCOnWafer(coord.common.Enum(0))
            self.RETICLE_TO_BE_POWERED = False
            self.HICANN_COORD = coord.HICANNOnWafer(coord.common.Enum(144))
            self.HICANN_VERSION = 4
            self.HICANN_LABEL = "bla"
            self.ADC_COORD = "B123456"
            self.ADC_MODE = "LOAD_CALIBRATION"
            self.ADC_CHAN = coord.ChannelOnADC(0)
            self.ADC_TRIGGER = coord.TriggerOnADC(1)
        self.ADC_IP = coord.IPv4.from_string("192.168.10.120")
        if IS_PYPLUSPLUS:
            self.ADC_PORT = coord.TCPPort(10101)

        self.DLS_SETUP_ID = "07_20"
        self.HXCUBE_ID = 9

    @unittest.skipUnless(os.path.split(os.getcwd())[-1] == "hwdb", "assuming test is executed with cwd == hwdb/ as done by waf")
    def test_hxcube_entries_have_serial(self):
        path = os.path.join(os.getcwd(), "db.yaml")
        db = pyhwdb.database()
        self.assertTrue(os.path.exists(path))
        db.load(path)
        unique_serial_numbers = set()
        all_ids = db.get_hxcube_ids()
        for hxcube_id in all_ids:
            entry = db.get_hxcube_entry(hxcube_id)
            self.assertNotEqual(entry.usb_serial, "", "USB serial number is present for setup {}".format(hxcube_id))
            unique_serial_numbers.add(entry.usb_serial)
        self.assertEqual(len(all_ids), len(unique_serial_numbers), "HX cube setups have unique USB serial")

    @unittest.skipUnless("GERRIT_EVENT_TYPE" in os.environ and os.environ["GERRIT_EVENT_TYPE"]=="change-merged", "for deployment tests only")
    def test_default_path_valid(self):
        db = pyhwdb.database()
        default_path = db.get_default_path()
        self.assertTrue(os.path.exists(default_path))
        db.load(default_path)

    @unittest.skipUnless(IS_PYPLUSPLUS, "Only works for wafer currently")
    def test_add_to_empty_db(self):
        mydb = pyhwdb.database()
        fpga_coord = coord.FPGAGlobal(self.FPGA_COORD, self.WAFER_COORD)
        hicann_coord = coord.HICANNGlobal(self.HICANN_COORD, self.WAFER_COORD)

        with self.assertRaises(IndexError):
            mydb.add_fpga_entry(fpga_coord, pyhwdb.FPGAEntry())
        with self.assertRaises(IndexError):
            mydb.add_hicann_entry(hicann_coord, pyhwdb.HICANNEntry())

        # FIXME: get usable ADC coord
        #adc_coord = coord.ADCGlobal(adc_COORD, self.WAFER_COORD)
        #with self.assertRaises(IndexError):
        #    mydb.add_adc_entry(adc_coord, pyhwdb.ADCEntry())

    def test_database_access(self):
        mydb = pyhwdb.database()

        if IS_PYPLUSPLUS:
            wafer = pyhwdb.WaferEntry()
            wafer_coord = self.WAFER_COORD
            wafer.setup_type = self.WAFER_SETUP_TYPE
            self.assertFalse(mydb.has_wafer_entry(wafer_coord))
            mydb.add_wafer_entry(wafer_coord, wafer)
            self.assertTrue(mydb.has_wafer_entry(wafer_coord))
            self.assertEqual(mydb.get_wafer_entry(wafer_coord).setup_type, wafer.setup_type)
            wafer_coords = mydb.get_wafer_coordinates()
            self.assertEqual(len(wafer_coords), 1)
            mydb.remove_wafer_entry(wafer_coord)
            self.assertFalse(mydb.has_wafer_entry(wafer_coord))

        dls_entry = pyhwdb.DLSSetupEntry()
        dls_setup_id = self.DLS_SETUP_ID
        self.assertFalse(mydb.has_dls_entry(dls_setup_id))
        mydb.add_dls_entry(dls_setup_id, dls_entry)
        self.assertTrue(mydb.has_dls_entry(dls_setup_id))
        mydb.remove_dls_entry(dls_setup_id)
        self.assertFalse(mydb.has_dls_entry(dls_setup_id))

        hxcube_entry = pyhwdb.HXCubeSetupEntry()
        hxcube_id = self.HXCUBE_ID
        hxcube_entry.hxcube_id = hxcube_id
        hxcube_entry.fpga_ips[0] = self.FPGA_IP
        hxcube_entry.fpga_ips[1] = self.ADC_IP  # just a different IP
        hxcube_entry.usb_host = "fantasy"
        hxcube_entry.ldo_version = 8
        hxcube_entry.usb_serial = "ABACD1243"
        hxcube_entry.eeprom_chip_serial = 0x987DE
        hxcube_entry.handwritten_chip_serial = 12
        hxcube_entry.chip_revision = 42
        self.assertFalse(mydb.has_hxcube_entry(hxcube_id))
        mydb.add_hxcube_entry(hxcube_id, hxcube_entry)
        self.assertTrue(mydb.has_hxcube_entry(hxcube_id))
        mydb.remove_hxcube_entry(hxcube_id)
        self.assertFalse(mydb.has_hxcube_entry(hxcube_id))
        self.assertEqual(hxcube_entry.get_unique_identifier(),
                         "hxcube{}chip{}_1".format(
                             hxcube_entry.hxcube_id,
                             hxcube_entry.handwritten_chip_serial))


        if IS_PYPLUSPLUS:
            # require wafer entry to write other entry typed into
            mydb.add_wafer_entry(wafer_coord, wafer)

            fpga = pyhwdb.FPGAEntry()
            fpga_coord = coord.FPGAGlobal(self.FPGA_COORD, self.WAFER_COORD)
            fpga.ip = self.FPGA_IP
            self.assertFalse(mydb.has_fpga_entry(fpga_coord))
            mydb.add_fpga_entry(fpga_coord, fpga)
            self.assertTrue(mydb.has_fpga_entry(fpga_coord))
            self.assertEqual(mydb.get_fpga_entry(fpga_coord).ip, fpga.ip)
            mydb.remove_fpga_entry(fpga_coord)
            self.assertFalse(mydb.has_fpga_entry(fpga_coord))

            reticle = pyhwdb.ReticleEntry()
            reticle_coord = coord.DNCGlobal(self.RETICLE_COORD, self.WAFER_COORD)
            reticle.to_be_powered = self.RETICLE_TO_BE_POWERED
            self.assertFalse(mydb.has_reticle_entry(reticle_coord))
            mydb.add_reticle_entry(reticle_coord, reticle)
            self.assertTrue(mydb.has_reticle_entry(reticle_coord))
            self.assertEqual(mydb.get_reticle_entry(reticle_coord).to_be_powered, reticle.to_be_powered)
            mydb.remove_reticle_entry(reticle_coord)
            self.assertFalse(mydb.has_reticle_entry(reticle_coord))


            # require fpga entry to add hicann entry
            mydb.add_fpga_entry(fpga_coord, fpga)

            hicann = pyhwdb.HICANNEntry()
            hicann_coord = coord.HICANNGlobal(self.HICANN_COORD, self.WAFER_COORD)
            hicann.version = self.HICANN_VERSION
            self.assertFalse(mydb.has_hicann_entry(hicann_coord))
            mydb.add_hicann_entry(hicann_coord, hicann)
            self.assertTrue(mydb.has_hicann_entry(hicann_coord))
            self.assertEqual(mydb.get_hicann_entry(hicann_coord).version, hicann.version)
            mydb.remove_hicann_entry(hicann_coord)
            self.assertFalse(mydb.has_hicann_entry(hicann_coord))

            # test clear()
            mydb.add_hicann_entry(hicann_coord, hicann)
            mydb.clear()
            self.assertFalse(mydb.has_hicann_entry(hicann_coord))
            self.assertFalse(mydb.has_fpga_entry(fpga_coord))
            self.assertFalse(mydb.has_wafer_entry(wafer_coord))

            # FIXME: get usable ADC coord, i.e. proper wrapping
            #adc = pyhwdb.ADCEntry()
            #adc_coord = coord.ADCGlobal(self.ADC_COORD, self.WAFER_COORD)
            #adc.version = self.ADC_CAHNNEL
            #self.assertFalse(mydb.has_adc_entry(adc_coord))
            #mydb.add_adc_entry(adc_coord, adc)
            #self.assertTrue(mydb.has_adc_entry(adc_coord))
            #self.assertEqual(mydb.get_adc_entry(adc_coord).channel, adc.channel)
            #mydb.remove_adc_entry(adc_coord)
            #self.assertFalse(mydb.has_adc_entry(adc_coord))


if __name__ == "__main__":
    unittest.main()

