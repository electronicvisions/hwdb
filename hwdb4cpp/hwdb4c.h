/* This is plain C :) */
#pragma once
#include "hate/visibility.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HWDB4C_SUCCESS 0
#define HWDB4C_FAILURE -1

typedef struct in_addr ip_addr_t; // in network byte order
typedef uint16_t udp_port_t;
struct SYMBOL_VISIBLE hwdb4c_database_t;

struct SYMBOL_VISIBLE hwdb4c_fpga_entry {
	//key
	size_t fpgaglobal_id;

	//values
	ip_addr_t ip;
	bool highspeed;
};

struct SYMBOL_VISIBLE hwdb4c_reticle_entry {
	//key
	size_t reticleglobal_id;

	//values
	bool to_be_powered;
};

struct SYMBOL_VISIBLE hwdb4c_ananas_entry {
	//key
	size_t ananasglobal_id;

	//values
	ip_addr_t ip;
	udp_port_t baseport_data;
	udp_port_t baseport_reset;
};

struct SYMBOL_VISIBLE hwdb4c_hicann_entry {
	//key
	size_t hicannglobal_id;

	//values
	size_t version;
	char* label;
};

struct SYMBOL_VISIBLE hwdb4c_adc_entry {
	//composite key
	size_t fpgaglobal_id;
	size_t analogout;

	//values
	enum calibration_mode_t {
		LOAD_CALIBRATION,
		ESS_CALIBRATION,
		DEFAULT_CALIBRATION
	} calibration_mode;
	char* coord;
	size_t channel;
	size_t trigger;
	ip_addr_t remote_ip;
	size_t remote_port;
};

struct SYMBOL_VISIBLE hwdb4c_wafer_entry {
	//key
	size_t wafer_id;

	//values
	enum setup_type_t {
		CubeSetup,
		BSSWafer
	} setup_type;
	struct hwdb4c_fpga_entry** fpgas;
	size_t num_fpga_entries;
	struct hwdb4c_reticle_entry** reticles;
	size_t num_reticle_entries;
	struct hwdb4c_ananas_entry** ananas;
	size_t num_ananas_entries;
	struct hwdb4c_hicann_entry** hicanns;
	size_t num_hicann_entries;
	struct hwdb4c_adc_entry** adcs;
	size_t num_adc_entries;
	ip_addr_t macu_ip;
	size_t macu_version;
};

struct SYMBOL_VISIBLE hwdb4c_dls_setup_entry {
	//key
	char* dls_setup;

	//values
	char* fpga_name;
	char* board_name;
	size_t board_version;
	size_t chip_id;
	size_t chip_version;
	char* ntpwr_ip;
	size_t ntpwr_slot;
};

struct SYMBOL_VISIBLE hwdb4c_hxcube_wing_entry
{
	size_t ldo_version;
	size_t handwritten_chip_serial;
	size_t chip_revision;
	uint32_t eeprom_chip_serial;
	uint16_t synram_timing_pcconf[2][2];
	uint16_t synram_timing_wconf[2][2];
};

struct SYMBOL_VISIBLE hwdb4c_hxcube_fpga_entry
{
	size_t fpga_id;
	struct in_addr ip;
	struct hwdb4c_hxcube_wing_entry* wing;
	uint64_t fuse_dna;
	uint16_t extoll_node_id;
	bool ci_test_node;

	uint64_t dna_port; // cpp get_dna_port member function
};

struct SYMBOL_VISIBLE hwdb4c_hxcube_setup_entry
{
	//key
	size_t hxcube_id;

	//values
	struct hwdb4c_hxcube_fpga_entry** fpgas;
	size_t num_fpgas;
	char* usb_host;
	char* usb_serial;
	char* xilinx_hw_server;
};


// functions to allocate cpp hwdb object
int hwdb4c_alloc_hwdb(struct hwdb4c_database_t** ret) SYMBOL_VISIBLE;
// load database either form path or if path is NULL load from default hwdb path
int hwdb4c_load_hwdb(struct hwdb4c_database_t* handle, char const* hwdb_path) SYMBOL_VISIBLE;
// store database to path
int hwdb4c_store_hwdb(struct hwdb4c_database_t* handle, char const* hwdb_path) SYMBOL_VISIBLE;
// clear the loaded content
void hwdb4c_clear_hwdb(struct hwdb4c_database_t* handle) SYMBOL_VISIBLE;
// free HWDB handle
void hwdb4c_free_hwdb(struct hwdb4c_database_t* ret) SYMBOL_VISIBLE;

// return matching yaml entries for query
char* hwdb4c_get_yaml_entries(char const* hwdb_path, char const* node, char const* query)
	SYMBOL_VISIBLE;

// check if entry in in hwdb, return HWDB4C_SUCCESS on success, on error returns HWDB4C_FAILURE
int hwdb4c_has_fpga_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_reticle_entry(struct hwdb4c_database_t* handle, size_t reticleglobal_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_ananas_entry(struct hwdb4c_database_t* handle, size_t ananasglobal_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_hicann_entry(struct hwdb4c_database_t* handle, size_t hicannglobal_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_adc_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, size_t analogonhicann, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_wafer_entry(struct hwdb4c_database_t* handle, size_t wafer_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_dls_entry(struct hwdb4c_database_t* handle, char* dls_setup, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_hxcube_setup_entry(struct hwdb4c_database_t* handle, size_t hxcube_id, bool* ret)
	SYMBOL_VISIBLE;

// get entry from hwdb, if entry not in hwdb or invalid coord returns HWDB4C_FAILURE
// onwership of entries lies with user, use corresponding hwdb4c_free_xxx_entry function to free memory
int hwdb4c_get_fpga_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_fpga_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_reticle_entry(struct hwdb4c_database_t* handle, size_t reticleglobal_id, struct hwdb4c_reticle_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_ananas_entry(struct hwdb4c_database_t* handle, size_t ananasglobal_id, struct hwdb4c_ananas_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_hicann_entry(struct hwdb4c_database_t* handle, size_t hicannglobal_id, struct hwdb4c_hicann_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_adc_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, size_t analogonhicann, struct hwdb4c_adc_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_wafer_entry(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_wafer_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_dls_entry(struct hwdb4c_database_t* handle, char* dls_setup, struct hwdb4c_dls_setup_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_hxcube_setup_entry(
	struct hwdb4c_database_t* handle,
	size_t hxcube_id,
	struct hwdb4c_hxcube_setup_entry** ret) SYMBOL_VISIBLE;

// returns all Wafer IDs in database as size_t array of size num_wafer, ownership of array lies with user
int hwdb4c_get_wafer_coordinates(struct hwdb4c_database_t* handle, size_t** wafer, size_t* num_wafer) SYMBOL_VISIBLE;

// returns all DLS setup IDs in database as char* array of size num_dls_setups
int hwdb4c_get_dls_setup_ids(struct hwdb4c_database_t* handle, char*** dls_setups, size_t* num_dls_setups) SYMBOL_VISIBLE;

// get array of entries, size of array given with num_xxx, if num_xxx ist zero than pointer is NULL
// return HWDB4C_SUCCESS on success, if coord invalid returns HWDB4C_FAILURE
// onwership of entries lies with user, use corresponding hwdb4c_free_xxx_entry function to free memory
int hwdb4c_get_adc_entries_of_Wafer(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_adc_entry*** adcs, size_t* num_adcs) SYMBOL_VISIBLE;
int hwdb4c_get_adc_entries_of_FPGAGlobal(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_adc_entry*** adcs, size_t* num_adcs) SYMBOL_VISIBLE;
int hwdb4c_get_hicann_entries_of_Wafer(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_hicann_entry*** hicanns, size_t* num_hicanns) SYMBOL_VISIBLE;
int hwdb4c_get_hicann_entries_of_FPGAGlobal(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_hicann_entry*** hicanns, size_t* num_hicanns) SYMBOL_VISIBLE;

// free memory of an entry
void hwdb4c_free_fpga_entry(struct hwdb4c_fpga_entry* fpga) SYMBOL_VISIBLE;
void hwdb4c_free_reticle_entry(struct hwdb4c_reticle_entry* reticle) SYMBOL_VISIBLE;
void hwdb4c_free_ananas_entry(struct hwdb4c_ananas_entry* ananas) SYMBOL_VISIBLE;
void hwdb4c_free_hicann_entry(struct hwdb4c_hicann_entry* hicann) SYMBOL_VISIBLE;
void hwdb4c_free_adc_entry(struct hwdb4c_adc_entry* adc) SYMBOL_VISIBLE;
void hwdb4c_free_wafer_entry(struct hwdb4c_wafer_entry* wafer) SYMBOL_VISIBLE;
void hwdb4c_free_dls_setup_entry(struct hwdb4c_dls_setup_entry* dls_setup) SYMBOL_VISIBLE;
void hwdb4c_free_hicann_entries(struct hwdb4c_hicann_entry** hicanns, size_t num_hicanns) SYMBOL_VISIBLE;
void hwdb4c_free_adc_entries(struct hwdb4c_adc_entry** adcs, size_t num_adcs) SYMBOL_VISIBLE;
void hwdb4c_free_hxcube_setup_entry(struct hwdb4c_hxcube_setup_entry* setup) SYMBOL_VISIBLE;
void hwdb4c_free_hxcube_fpga_entry(struct hwdb4c_hxcube_fpga_entry* fpga) SYMBOL_VISIBLE;

//convert functions for HALbe coordinates
//FIXME should be its own API
size_t hwdb4c_FPGAOnWafer_size() SYMBOL_VISIBLE;
size_t hwdb4c_DNCOnWafer_size() SYMBOL_VISIBLE;
size_t hwdb4c_AnanasOnWafer_size() SYMBOL_VISIBLE;
size_t hwdb4c_HICANNOnWafer_size() SYMBOL_VISIBLE;
size_t hwdb4c_master_FPGA_enum() SYMBOL_VISIBLE;
int hwdb4c_ReticleOnWafer_toFPGAOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_FPGAOnWafer_toReticleOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_FPGAOnWafer_toTriggerOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_toReticleOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_toFPGAOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_TriggerOnWafer_toAnanasOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_east(size_t hicann_id, size_t* ret_east_id) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_south(size_t hicann_id, size_t* ret_south_id) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_west(size_t hicann_id, size_t* ret_west_id) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_north(size_t hicann_id, size_t* ret_north_id) SYMBOL_VISIBLE;

// Converts coordinate to slurm license string. ret needs to be freed
int hwdb4c_AnanasGlobal_slurm_license(size_t ananas_id, char** ret) SYMBOL_VISIBLE;
int hwdb4c_FPGAGlobal_slurm_license(size_t fpga_id, char** ret) SYMBOL_VISIBLE;
int hwdb4c_HICANNGlobal_slurm_license(size_t hicann_id, char** ret) SYMBOL_VISIBLE;
int hwdb4c_TriggerGlobal_slurm_license(size_t trigger_id, char** ret) SYMBOL_VISIBLE;

#ifdef __cplusplus
} // extern "C"
#endif
