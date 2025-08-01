#include "halco/hicann/v2/coordinates.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <gtest/gtest.h>

#include "hwdb4cpp/hwdb4cpp.h"

extern "C" {
#include <arpa/inet.h>
#include <string.h>
#include "hwdb4cpp/hwdb4c.h"
}

static size_t constexpr testwafer_id = 5;
static char testdls_id0[] = "07_20";
static char testdls_id1[] = "B123456_42";
static char testdls_id_false[] = "07_21";
static size_t constexpr testhxcube_id = 6;
static size_t constexpr testjboa_id = 7;
static size_t constexpr fpgas_per_wafer = halco::hicann::v2::FPGAOnWafer::size;
static size_t constexpr reticles_per_wafer = halco::hicann::v2::DNCOnWafer::size;
static size_t constexpr ananas_per_wafer = halco::hicann::v2::AnanasOnWafer::size;
static size_t constexpr hicanns_per_wafer = halco::hicann::v2::HICANNOnWafer::size;

class HWDB4C_Test : public ::testing::Test
{
public:
	HWDB4C_Test() :
		fd(0)
	{
		char tmp_path[] = {"/tmp/abcdXXXXXX"};
		fd = mkstemp(tmp_path);
		if (!fd)
			std::cout << "ERROR open" << std::endl;
		test_path = std::string(tmp_path);
		write(fd, test_db_string.c_str(), test_db_string.size());
		if (close(fd)!= 0)
			std::cout << "ERROR close" << std::endl;
	}

	~HWDB4C_Test() {
		remove(test_path.c_str());
	}

protected:
	std::string test_path;
	int fd;

	std::string const test_db_string = "---\n\
wafer: 5\n\
setuptype: bsswafer\n\
macu: 192.168.200.165\n\
macuversion: 1\n\
fpgas:\n\
  - fpga: 0\n\
    ip: 192.168.5.1\n\
  - fpga: 3\n\
    ip: 192.168.5.4\n\
reticles:\n\
  - reticle: 0\n\
    to_be_powered: true\n\
  - reticle: 1\n\
    to_be_powered: false\n\
ananas:\n\
  - ananas: 0\n\
    ip: 192.168.5.190\n\
    baseport_data: 0xafe0\n\
    baseport_reset: 0x2570\n\
adcs:\n\
  - fpga: 0\n\
    analog: 0\n\
    adc: B201331\n\
    channel: 0\n\
    trigger: 1\n\
  - fpga: 0\n\
    analog: 1\n\
    adc: B201331\n\
    channel: 1\n\
    trigger: 1\n\
  - fpga: 3\n\
    analog: 0\n\
    adc: B201259\n\
    channel: 0\n\
    trigger: 1\n\
    remote_ip: 192.168.200.44\n\
    remote_port: 44489\n\
hicanns:\n\
  - hicann: 88\n\
    version: 4\n\
    label: v4-26\n\
  - hicann: 116\n\
    version: 4\n\
    label: v4-26\n\
  - hicann: 144\n\
    version: 4\n\
    label: v4-15\n\
---\n\
dls_setup: '07_20'\n\
fpga_name: '07'\n\
board_name: 'Gaston'\n\
board_version: 2\n\
chip_id: '20'\n\
chip_version: 2\n\
ntpwr_ip: '192.168.200.54'\n\
ntpwr_slot: 1\n\
---\n\
dls_setup: 'B123456_42'\n\
fpga_name: 'B123456'\n\
board_name: 'Herbert'\n\
board_version: 5\n\
chip_id: '42'\n\
chip_version: 4\n\
ntpwr_ip: '192.168.200.108'\n\
ntpwr_slot: 3\n\
---\n\
hxcube_id: 6\n\
fpgas:\n\
  - fpga: 0\n\
    ip: 192.168.66.1\n\
    ci_test_node: true\n\
    extoll_node_id: 2\n\
    handwritten_chip_serial: 12\n\
    chip_revision: 42\n\
    eeprom_chip_serial: 0x1234ABCD\n\
    synram_timing_pcconf:\n\
      - [1, 2]\n\
      - [1, 2]\n\
    synram_timing_wconf:\n\
      - [3, 4]\n\
      - [3, 4]\n\
    fuse_dna: 0x3A0E92C402882A33\n\
  - fpga: 3\n\
    ip: 192.168.66.4\n\
    handwritten_chip_serial: 69\n\
    chip_revision: 1\n\
  - fpga: 7\n\
    ip: 192.168.66.8\n\
usb_host: 'AMTHost11'\n\
usb_serial: 'AFEABC1230456789'\n\
xilinx_hw_server: 'abc.de:1234'\n\
---\n\
jboa_id: 7\n\
fpgas:\n\
  - fpga: 12\n\
    ip: 192.168.87.33\n\
    handwritten_chip_serial: 13\n\
    chip_revision: 43\n\
  - fpga: 13\n\
    ip: 192.168.87.34\n\
    fuse_dna: 0x123456789\n\
aggregators:\n\
  - aggregator: 0\n\
    ip: 192.168.87.13\n\
    ci_test_node: true\n\
  - aggregator: 1\n\
    ip: 192.168.87.45\n\
xilinx_hw_server: 'abc.yz:4321'\n\
";
};


TEST_F(HWDB4C_Test, HWDB_Handle)
{

	hwdb4c_database_t* hwdb = NULL;

	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != NULL);
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);
	hwdb4c_free_hwdb(hwdb);
}

TEST_F(HWDB4C_Test, HWDB_default_path)
{
	// only run this test at deployment time
	if (std::getenv("GERRIT_EVENT_TYPE") != 0 && strcmp(std::getenv("GERRIT_EVENT_TYPE"), "change-merged") == 0)
	{
		hwdb4c_database_t* hwdb = NULL;

		ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
		ASSERT_TRUE(hwdb != NULL);
		ASSERT_EQ(hwdb4c_load_hwdb(hwdb, NULL), HWDB4C_SUCCESS);
		hwdb4c_free_hwdb(hwdb);
	}
}

TEST_F(HWDB4C_Test, has_entry)
{
	hwdb4c_database_t* hwdb = NULL;
	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != NULL);
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);

	bool ret = false;
	ASSERT_EQ(hwdb4c_has_wafer_entry(hwdb, testwafer_id, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_wafer_entry(hwdb, testwafer_id + 1, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_dls_entry(hwdb, testdls_id0, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_dls_entry(hwdb, testdls_id_false, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_hxcube_setup_entry(hwdb, testhxcube_id, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_hxcube_setup_entry(hwdb, testhxcube_id + 1, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 0, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 1, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);
	ASSERT_EQ(hwdb4c_has_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_reticle_entry(hwdb, reticles_per_wafer * testwafer_id + 0, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_reticle_entry(hwdb, reticles_per_wafer * testwafer_id + 1, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_reticle_entry(hwdb, reticles_per_wafer * testwafer_id + 2, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_ananas_entry(hwdb, ananas_per_wafer * testwafer_id + 0, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_ananas_entry(hwdb, ananas_per_wafer * testwafer_id + 1, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_hicann_entry(hwdb, hicanns_per_wafer * testwafer_id + 144, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_hicann_entry(hwdb, hicanns_per_wafer * testwafer_id + 20, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);

	ret = false;
	ASSERT_EQ(hwdb4c_has_adc_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, 0,&ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_adc_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, 1, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);
	ASSERT_EQ(hwdb4c_has_adc_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, 6, &ret), HWDB4C_FAILURE);

	hwdb4c_free_hwdb(hwdb);
}

namespace {

void get_entry_test_impl(hwdb4c_database_t* hwdb)
{
	size_t* wafer_list = NULL;
	size_t num_wafer = 0;
	ASSERT_EQ(hwdb4c_get_wafer_coordinates(hwdb, &wafer_list, &num_wafer), HWDB4C_SUCCESS);
	ASSERT_TRUE(wafer_list != NULL);
	ASSERT_EQ(num_wafer, 1);
	ASSERT_EQ(wafer_list[0], 5);
	free(wafer_list);
	wafer_list = NULL;

	char** dls_setup_list = NULL;
	size_t num_dls_setups = 0;
	ASSERT_EQ(hwdb4c_get_dls_setup_ids(hwdb, &dls_setup_list, &num_dls_setups), HWDB4C_SUCCESS);
	ASSERT_TRUE(dls_setup_list[0] != NULL);
	ASSERT_EQ(num_dls_setups, 2);
	ASSERT_EQ(strcmp(dls_setup_list[0], testdls_id0), 0);
	ASSERT_EQ(strcmp(dls_setup_list[1], testdls_id1), 0);
	for (size_t i = 0; i < num_dls_setups; i++) {
		free(dls_setup_list[i]);
	}
	free(dls_setup_list);
	dls_setup_list = NULL;

	hwdb4c_fpga_entry* fpga = NULL;
	ASSERT_EQ(hwdb4c_get_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, &fpga), HWDB4C_SUCCESS);
	ASSERT_TRUE(fpga != NULL);
	EXPECT_EQ(fpga->fpgaglobal_id, fpgas_per_wafer * testwafer_id + 3);
	EXPECT_EQ(std::string(inet_ntoa(fpga->ip)), "192.168.5.4");
	EXPECT_EQ(fpga->highspeed, true);
	hwdb4c_free_fpga_entry(fpga);
	fpga = NULL;

	hwdb4c_reticle_entry* reticle = NULL;
	ASSERT_EQ(hwdb4c_get_reticle_entry(hwdb, reticles_per_wafer * testwafer_id + 0, &reticle), HWDB4C_SUCCESS);
	ASSERT_TRUE(reticle != NULL);
	EXPECT_EQ(reticle->reticleglobal_id, reticles_per_wafer * testwafer_id + 0);
	EXPECT_EQ(reticle->to_be_powered, true);
	hwdb4c_free_reticle_entry(reticle);
	reticle = NULL;

	ASSERT_EQ(hwdb4c_get_reticle_entry(hwdb, reticles_per_wafer * testwafer_id + 1, &reticle), HWDB4C_SUCCESS);
	ASSERT_TRUE(reticle != NULL);
	EXPECT_EQ(reticle->reticleglobal_id, reticles_per_wafer * testwafer_id + 1);
	EXPECT_EQ(reticle->to_be_powered, false);
	hwdb4c_free_reticle_entry(reticle);
	reticle = NULL;

	hwdb4c_ananas_entry* ananas = NULL;
	ASSERT_EQ(hwdb4c_get_ananas_entry(hwdb, ananas_per_wafer * testwafer_id, &ananas), HWDB4C_SUCCESS);
	ASSERT_TRUE(ananas != NULL);
	EXPECT_EQ(ananas->ananasglobal_id, ananas_per_wafer * testwafer_id);
	EXPECT_EQ(std::string(inet_ntoa(ananas->ip)), "192.168.5.190");
	EXPECT_EQ(ananas->baseport_data, 0xafe0);
	EXPECT_EQ(ananas->baseport_reset, 0x2570);
	hwdb4c_free_ananas_entry(ananas);
	ananas = NULL;

	hwdb4c_hicann_entry* hicann = NULL;
	ASSERT_EQ(hwdb4c_get_hicann_entry(hwdb, hicanns_per_wafer * testwafer_id + 144, &hicann), HWDB4C_SUCCESS);
	ASSERT_TRUE(hicann != NULL);
	EXPECT_EQ(hicann->hicannglobal_id, hicanns_per_wafer * testwafer_id + 144);
	EXPECT_EQ(hicann->version, 4);
	EXPECT_EQ(std::string(hicann->label), std::string("v4-15"));
	hwdb4c_free_hicann_entry(hicann);
	hicann = NULL;

	hwdb4c_adc_entry* adc = NULL;
	ASSERT_EQ(hwdb4c_get_adc_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, 0, &adc), HWDB4C_SUCCESS);
	ASSERT_TRUE(adc != NULL);
	EXPECT_EQ(adc->fpgaglobal_id, fpgas_per_wafer * testwafer_id + 3);
	EXPECT_EQ(adc->analogout, 0);
	EXPECT_EQ(std::string(adc->coord), "B201259");
	EXPECT_EQ(adc->channel, 0);
	EXPECT_EQ(adc->trigger, 1);
	EXPECT_EQ(std::string(inet_ntoa(adc->remote_ip)), "192.168.200.44");
	EXPECT_EQ(adc->remote_port, 44489);
	hwdb4c_free_adc_entry(adc);
	adc = NULL;

	hwdb4c_hxcube_setup_entry* hxcube = NULL;
	ASSERT_EQ(hwdb4c_get_hxcube_setup_entry(hwdb, testhxcube_id, &hxcube), HWDB4C_SUCCESS);
	ASSERT_TRUE(hxcube != NULL);
	EXPECT_EQ(hxcube->hxcube_id, testhxcube_id);
	EXPECT_EQ(std::string(hxcube->usb_serial), "AFEABC1230456789");
	EXPECT_EQ(std::string(hxcube->usb_host), "AMTHost11");
	EXPECT_EQ(hxcube->num_fpgas, 3);
	EXPECT_EQ(std::string(hxcube->xilinx_hw_server), "abc.de:1234");

	EXPECT_EQ(std::string(inet_ntoa(hxcube->fpgas[0]->ip)), "192.168.66.1");
	EXPECT_EQ(hxcube->fpgas[0]->ci_test_node, true);
	EXPECT_EQ(hxcube->fpgas[0]->fpga_id, 0);
	EXPECT_EQ(hxcube->fpgas[0]->fuse_dna, 0x3A0E92C402882A33);
	EXPECT_EQ(hxcube->fpgas[0]->extoll_node_id, 2);
	EXPECT_EQ(hxcube->fpgas[0]->dna_port, 0x5411402349705C);
	EXPECT_EQ(hxcube->fpgas[0]->wing->handwritten_chip_serial, 12);
	EXPECT_EQ(hxcube->fpgas[0]->wing->chip_revision, 42);
	EXPECT_EQ(hxcube->fpgas[0]->wing->eeprom_chip_serial, 0x1234ABCD);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_pcconf[0][0], 1);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_pcconf[0][1], 2);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_pcconf[1][0], 1);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_pcconf[1][1], 2);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_wconf[0][0], 3);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_wconf[0][1], 4);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_wconf[1][0], 3);
	EXPECT_EQ(hxcube->fpgas[0]->wing->synram_timing_wconf[1][1], 4);

	EXPECT_EQ(std::string(inet_ntoa(hxcube->fpgas[1]->ip)), "192.168.66.4");
	EXPECT_EQ(hxcube->fpgas[1]->ci_test_node, false);
	EXPECT_EQ(hxcube->fpgas[1]->fpga_id, 3);
	EXPECT_TRUE(hxcube->fpgas[1]->fuse_dna == 0); // optional value
	EXPECT_TRUE(hxcube->fpgas[1]->extoll_node_id == 0); // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->handwritten_chip_serial, 69);
	EXPECT_EQ(hxcube->fpgas[1]->wing->chip_revision, 1);
	EXPECT_EQ(hxcube->fpgas[1]->wing->eeprom_chip_serial, 0);         // default value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_pcconf[0][0], 0); // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_pcconf[0][1], 0); // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_pcconf[1][0], 0); // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_pcconf[1][1], 0); // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_wconf[0][0], 0);  // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_wconf[0][1], 0);  // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_wconf[1][0], 0);  // optional value
	EXPECT_EQ(hxcube->fpgas[1]->wing->synram_timing_wconf[1][1], 0);  // optional value

	EXPECT_EQ(std::string(inet_ntoa(hxcube->fpgas[2]->ip)), "192.168.66.8");
	EXPECT_EQ(hxcube->fpgas[2]->ci_test_node, false);
	EXPECT_EQ(hxcube->fpgas[2]->fpga_id, 7);
	EXPECT_TRUE(hxcube->fpgas[2]->fuse_dna == 0); // optional value
	EXPECT_TRUE(hxcube->fpgas[1]->extoll_node_id == 0); // optional value
	EXPECT_TRUE(hxcube->fpgas[2]->wing == NULL); // optional value

	hwdb4c_free_hxcube_setup_entry(hxcube);
	hxcube = NULL;

	hwdb4c_jboa_setup_entry* jboa = NULL;
	ASSERT_EQ(hwdb4c_get_jboa_setup_entry(hwdb, testjboa_id, &jboa), HWDB4C_SUCCESS);
	ASSERT_TRUE(jboa != NULL);
	EXPECT_EQ(jboa->jboa_id, testjboa_id);
	EXPECT_EQ(jboa->num_fpgas, 2);
	EXPECT_EQ(std::string(jboa->xilinx_hw_server), "abc.yz:4321");

	EXPECT_EQ(std::string(inet_ntoa(jboa->fpgas[0]->ip)), "192.168.87.33");
	EXPECT_EQ(jboa->fpgas[0]->ci_test_node, false);
	EXPECT_EQ(jboa->fpgas[0]->fpga_id, 12);
	EXPECT_EQ(jboa->fpgas[0]->fuse_dna, 0);
	EXPECT_EQ(jboa->fpgas[0]->wing->handwritten_chip_serial, 13);
	EXPECT_EQ(jboa->fpgas[0]->wing->chip_revision, 43);

	EXPECT_EQ(std::string(inet_ntoa(jboa->fpgas[1]->ip)), "192.168.87.34");
	EXPECT_EQ(jboa->fpgas[1]->fpga_id, 13);
	EXPECT_EQ(jboa->fpgas[1]->fuse_dna, 0x123456789);

	EXPECT_EQ(std::string(inet_ntoa(jboa->aggregators[0]->ip)), "192.168.87.13");
	EXPECT_EQ(jboa->aggregators[0]->aggregator_id, 0);
	EXPECT_EQ(jboa->aggregators[0]->ci_test_node, true);

	EXPECT_EQ(std::string(inet_ntoa(jboa->aggregators[1]->ip)), "192.168.87.45");
	EXPECT_EQ(jboa->aggregators[1]->aggregator_id, 1);
	EXPECT_EQ(jboa->aggregators[1]->ci_test_node, false);

	hwdb4c_free_jboa_setup_entry(jboa);
	jboa = NULL;

	//just check if num of entries is correct from now, not all entry elements
	hwdb4c_hicann_entry** hicanns = NULL;
	size_t num_hicanns = 0;
	ASSERT_EQ(hwdb4c_get_hicann_entries_of_Wafer(hwdb, testwafer_id, &hicanns, &num_hicanns), HWDB4C_SUCCESS);
	ASSERT_TRUE(hicanns != NULL);
	EXPECT_EQ(num_hicanns, 3);
	hwdb4c_free_hicann_entries(hicanns, num_hicanns);
	hicanns = NULL;
	num_hicanns = 0;
	ASSERT_EQ(hwdb4c_get_hicann_entries_of_FPGAGlobal(hwdb, fpgas_per_wafer * testwafer_id + 0, &hicanns, &num_hicanns), HWDB4C_SUCCESS);
	ASSERT_TRUE(hicanns != NULL);
	EXPECT_EQ(num_hicanns, 1);
	hwdb4c_free_hicann_entries(hicanns, num_hicanns);
	hicanns = NULL;

	hwdb4c_adc_entry** adcs = NULL;
	size_t num_adcs = 0;
	ASSERT_EQ(hwdb4c_get_adc_entries_of_Wafer(hwdb, testwafer_id, &adcs, &num_adcs), HWDB4C_SUCCESS);
	ASSERT_TRUE(adcs != NULL);
	EXPECT_EQ(num_adcs, 3);
	hwdb4c_free_adc_entries(adcs, num_adcs);
	adcs = NULL;
	num_adcs = 0;

	ASSERT_EQ(hwdb4c_get_adc_entries_of_FPGAGlobal(hwdb, fpgas_per_wafer * testwafer_id + 3, &adcs, &num_adcs), HWDB4C_SUCCESS);
	ASSERT_TRUE(adcs != NULL);
	EXPECT_EQ(num_adcs, 1);
	hwdb4c_free_adc_entries(adcs, num_adcs);
	adcs = NULL;

	hwdb4c_wafer_entry* wafer = NULL;
	ASSERT_EQ(hwdb4c_get_wafer_entry(hwdb, testwafer_id, &wafer), HWDB4C_SUCCESS);
	EXPECT_EQ(wafer->num_fpga_entries, 2);
	EXPECT_EQ(wafer->num_hicann_entries, 3);
	EXPECT_EQ(wafer->num_adc_entries, 3);
	EXPECT_EQ(std::string(inet_ntoa(wafer->macu_ip)), "192.168.200.165");
	EXPECT_EQ(wafer->macu_version, 1);
	hwdb4c_free_wafer_entry(wafer);
	wafer = NULL;

	bool ret = false;
	ASSERT_EQ(
		hwdb4c_has_hicann_entry(hwdb, hicanns_per_wafer * testwafer_id + 144, &ret),
		HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(
		hwdb4c_has_hicann_entry(hwdb, hicanns_per_wafer * testwafer_id + 20, &ret), HWDB4C_SUCCESS);
	EXPECT_FALSE(ret);
}
} // end namespace

TEST_F(HWDB4C_Test, get_entry)
{
	hwdb4c_database_t* hwdb = NULL;
	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != NULL);
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);
	get_entry_test_impl(hwdb);
	hwdb4c_free_hwdb(hwdb);
}

TEST_F(HWDB4C_Test, get_entry_after_store_and_load)
{
	hwdb4c_database_t* hwdb = NULL;
	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != NULL);

	// Dump the database into a temporary file and read it again.
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);
	ASSERT_EQ(hwdb4c_store_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);
	hwdb4c_clear_hwdb(hwdb);
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);

	get_entry_test_impl(hwdb);
	hwdb4c_free_hwdb(hwdb);
}

TEST_F(HWDB4C_Test, get_yaml_entries)
{
	hwdb4c_database_t* hwdb = nullptr;
	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != nullptr);

	std::string yaml_string;
	char* ret = hwdb4c_get_yaml_entries(test_path.c_str(), "dls_setup", "07_20");
	yaml_string = std::string(ret);
	std::string test_string = "dls_setup: 07_20\n"
	                          "fpga_name: 07\n"
	                          "board_name: Gaston\n"
	                          "board_version: 2\n"
	                          "chip_id: 20\n"
	                          "chip_version: 2\n"
	                          "ntpwr_ip: 192.168.200.54\n"
	                          "ntpwr_slot: 1";
	ASSERT_EQ(yaml_string, test_string);
	free(ret);

	ret = hwdb4c_get_yaml_entries(test_path.c_str(), "hxcube_id", "6");
	yaml_string = std::string(ret);
	test_string = "hxcube_id: 6\n"
	              "fpgas:\n"
	              "  - fpga: 0\n"
	              "    ip: 192.168.66.1\n"
	              "    ci_test_node: true\n"
	              "    extoll_node_id: 2\n"
	              "    handwritten_chip_serial: 12\n"
	              "    chip_revision: 42\n"
	              "    eeprom_chip_serial: 0x1234ABCD\n"
	              "    synram_timing_pcconf:\n"
	              "      - [1, 2]\n"
	              "      - [1, 2]\n"
	              "    synram_timing_wconf:\n"
	              "      - [3, 4]\n"
	              "      - [3, 4]\n"
	              "    fuse_dna: 0x3A0E92C402882A33\n"
	              "  - fpga: 3\n"
	              "    ip: 192.168.66.4\n"
	              "    handwritten_chip_serial: 69\n"
	              "    chip_revision: 1\n"
	              "  - fpga: 7\n"
	              "    ip: 192.168.66.8\n"
	              "usb_host: AMTHost11\n"
	              "usb_serial: AFEABC1230456789\n"
	              "xilinx_hw_server: abc.de:1234";

	ASSERT_EQ(yaml_string, test_string);
	free(ret);
	ret = hwdb4c_get_yaml_entries(test_path.c_str(), "hxcube_id", "");
	yaml_string = std::string(ret);
	ASSERT_EQ(yaml_string, "");
	free(ret);
	ret = hwdb4c_get_yaml_entries(test_path.c_str(), "hxcube_id", "5623");
	yaml_string = std::string(ret);
	ASSERT_EQ(yaml_string, "");
	free(ret);
	hwdb4c_free_hwdb(hwdb);
}


TEST_F(HWDB4C_Test, coord)
{
	using namespace halco::hicann::v2;
	using namespace halco::common;

	EXPECT_EQ(hwdb4c_HICANNOnWafer_size(), HICANNOnWafer::size);
	EXPECT_EQ(hwdb4c_FPGAOnWafer_size(), FPGAOnWafer::size);
	EXPECT_EQ(hwdb4c_master_FPGA_enum(), FPGAOnWafer::Master.toEnum().value());
	EXPECT_EQ(hwdb4c_AnanasOnWafer_size(), AnanasOnWafer::size);

	size_t ret = 0;
	EXPECT_EQ(hwdb4c_ReticleOnWafer_toFPGAOnWafer(0, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 12);
	EXPECT_EQ(hwdb4c_ReticleOnWafer_toFPGAOnWafer(108, &ret), HWDB4C_FAILURE);

	EXPECT_EQ(hwdb4c_FPGAOnWafer_toReticleOnWafer(0, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 21);
	EXPECT_EQ(hwdb4c_FPGAOnWafer_toReticleOnWafer(108, &ret), HWDB4C_FAILURE);

	EXPECT_EQ(hwdb4c_HICANNOnWafer_toReticleOnWafer(4, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_toReticleOnWafer(1008, &ret), HWDB4C_FAILURE);

	EXPECT_EQ(hwdb4c_HICANNOnWafer_toFPGAOnWafer(0, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 12);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_toFPGAOnWafer(1008, &ret), HWDB4C_FAILURE);

	EXPECT_EQ(hwdb4c_FPGAOnWafer_toTriggerOnWafer(12, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 5);
	EXPECT_EQ(hwdb4c_FPGAOnWafer_toTriggerOnWafer(1008, &ret), HWDB4C_FAILURE);

	EXPECT_EQ(hwdb4c_HICANNOnWafer_north(13, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_east(13, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 14);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_south(13, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 29);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_west(13, &ret), HWDB4C_SUCCESS);
	EXPECT_EQ(ret, 12);

	EXPECT_EQ(hwdb4c_HICANNOnWafer_north(HICANNOnWafer::enum_type::min, &ret), HWDB4C_FAILURE);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_west(HICANNOnWafer::enum_type::min, &ret), HWDB4C_FAILURE);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_east(HICANNOnWafer::enum_type::max, &ret), HWDB4C_FAILURE);
	EXPECT_EQ(hwdb4c_HICANNOnWafer_south(HICANNOnWafer::enum_type::max, &ret), HWDB4C_FAILURE);

	char* ret_string = NULL;
	ASSERT_EQ(
	    hwdb4c_FPGAGlobal_slurm_license(FPGAGlobal(Enum(49)).toEnum(), &ret_string),
	    HWDB4C_SUCCESS);
	// ret_string needs to be 0
	ASSERT_EQ(
	    hwdb4c_FPGAGlobal_slurm_license(FPGAGlobal(Enum(49)).toEnum(), &ret_string),
	    HWDB4C_FAILURE);
	EXPECT_EQ(std::string(ret_string), "W1F1");
	free(ret_string);
	ret_string = NULL;
	ASSERT_EQ(
	    hwdb4c_HICANNGlobal_slurm_license(HICANNGlobal(Enum(123)).toEnum(), &ret_string),
	    HWDB4C_SUCCESS);
	EXPECT_EQ(std::string(ret_string), "W0H123");
	free(ret_string);
	ret_string = NULL;
	ASSERT_EQ(
	    hwdb4c_AnanasGlobal_slurm_license(AnanasGlobal(Enum(6)).toEnum(), &ret_string),
	    HWDB4C_SUCCESS);
	EXPECT_EQ(std::string(ret_string), "W3A0");
	free(ret_string);
	ret_string = NULL;
	ASSERT_EQ(
	    hwdb4c_TriggerGlobal_slurm_license(TriggerGlobal(Enum(5)).toEnum(), &ret_string),
	    HWDB4C_SUCCESS);
	EXPECT_EQ(std::string(ret_string), "W0T5");
	free(ret_string);
}
