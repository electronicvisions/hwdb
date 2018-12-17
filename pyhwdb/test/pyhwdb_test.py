#!/usr/bin/env python

import unittest
import os
import pyhwdb
import pyhalco_hicann_v2 as coord

class Test_Pyhwdb(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(Test_Pyhwdb, self).__init__(*args, **kwargs)
        self.WAFER_COORD = coord.Wafer(10)
        self.WAFER_SETUP_TYPE = coord.SetupType.BSSWafer
        self.WAFER_MACU_IP = coord.IPv4.from_string("192.168.10.120")
        self.FPGA_COORD = coord.FPGAOnWafer(0)
        self.FPGA_HIGHSPEED = True
        self.FPGA_IP = coord.IPv4.from_string("192.168.10.1")
        self.HICANN_COORD = coord.HICANNOnWafer(coord.common.Enum(144))
        self.HICANN_VERSION = 4
        self.HICANN_LABEL = "bla"
        self.ADC_COORD = "B123456"
        self.ADC_MODE = "LOAD_CALIBRATION"
        self.ADC_CHAN = coord.ChannelOnADC(0)
        self.ADC_TRIGGER = coord.TriggerOnADC(1)
        self.ADC_IP = coord.IPv4.from_string("192.168.10.120")
        self.ADC_PORT = coord.TCPPort(10101)

        self.DLS_SETUP_ID = "07_20"

    @unittest.skipUnless("GERRIT_EVENT_TYPE" in os.environ and os.environ["GERRIT_EVENT_TYPE"]=="change-merged", "for deployment tests only")
    def test_default_path_valid(self):
        db = pyhwdb.database()
        default_path = db.get_default_path()
        self.assertTrue(os.path.exists(default_path))
        db.load(default_path)

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

        wafer = pyhwdb.WaferEntry()
        wafer_coord = self.WAFER_COORD
        wafer.setup_type = self.WAFER_SETUP_TYPE
        self.assertFalse(mydb.has_wafer_entry(wafer_coord))
        mydb.add_wafer_entry(wafer_coord, wafer)
        self.assertTrue(mydb.has_wafer_entry(wafer_coord))
        self.assertEqual(mydb.get_wafer_entry(wafer_coord).setup_type, wafer.setup_type)
        mydb.remove_wafer_entry(wafer_coord)
        self.assertFalse(mydb.has_wafer_entry(wafer_coord))

        dls_entry = pyhwdb.DLSSetupEntry()
        dls_setup_id = self.DLS_SETUP_ID
        self.assertFalse(mydb.has_dls_entry(dls_setup_id))
        mydb.add_dls_entry(dls_setup_id, dls_entry)
        self.assertTrue(mydb.has_dls_entry(dls_setup_id))
        mydb.remove_dls_entry(dls_setup_id)
        self.assertFalse(mydb.has_dls_entry(dls_setup_id))

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

