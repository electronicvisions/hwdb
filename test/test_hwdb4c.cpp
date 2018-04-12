#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include "halco/hicann/v2/coordinates.h"

#include "hwdb4cpp/hwdb4cpp.h"

extern "C" {
#include <arpa/inet.h>
#include "hwdb4cpp/hwdb4c.h"
}

static size_t constexpr testwafer_id = 5;
static size_t constexpr fpgas_per_wafer = halco::hicann::v2::FPGAOnWafer::size;
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

std::string const test_db_string =
"---\n\
wafer: 5\n\
setuptype: bsswafer\n\
macu: 192.168.200.165\n\
fpgas:\n\
  - fpga: 0\n\
    ip: 192.168.5.1\n\
  - fpga: 3\n\
    ip: 192.168.5.4\n\
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
    label: v4-15";
};


TEST_F(HWDB4C_Test, HWDB_Handle)
{

	hwdb4c_database_t* hwdb = NULL;

	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != NULL);
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, test_path.c_str()), HWDB4C_SUCCESS);
	hwdb4c_free_hwdb(hwdb);
	// test if default HWDB can be loaded
	ASSERT_EQ(hwdb4c_alloc_hwdb(&hwdb), HWDB4C_SUCCESS);
	ASSERT_TRUE(hwdb != NULL);
	ASSERT_EQ(hwdb4c_load_hwdb(hwdb, NULL), HWDB4C_SUCCESS);
	hwdb4c_free_hwdb(hwdb);
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
	ASSERT_EQ(hwdb4c_has_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 0, &ret), HWDB4C_SUCCESS);
	EXPECT_TRUE(ret);
	ASSERT_EQ(hwdb4c_has_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 1, &ret), HWDB4C_SUCCESS);
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

	hwdb4c_fpga_entry* fpga = NULL;
	ASSERT_EQ(hwdb4c_get_fpga_entry(hwdb, fpgas_per_wafer * testwafer_id + 3, &fpga), HWDB4C_SUCCESS);
	ASSERT_TRUE(fpga != NULL);
	EXPECT_EQ(fpga->fpgaglobal_id, fpgas_per_wafer * testwafer_id + 3);
	EXPECT_EQ(std::string(inet_ntoa(fpga->ip)), "192.168.5.4");
	EXPECT_EQ(fpga->highspeed, true);
	hwdb4c_free_fpga_entry(fpga);
	fpga = NULL;

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


TEST_F(HWDB4C_Test, coord)
{
	EXPECT_EQ(hwdb4c_HICANNOnWafer_size(), halco::hicann::v2::HICANNOnWafer::size);
	EXPECT_EQ(hwdb4c_FPGAOnWafer_size(), halco::hicann::v2::FPGAOnWafer::size);
	EXPECT_EQ(hwdb4c_master_FPGA_enum(), halco::hicann::v2::FPGAOnWafer::Master.toEnum().value());

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
}
