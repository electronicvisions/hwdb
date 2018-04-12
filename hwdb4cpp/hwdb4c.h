/* This is plain C :) */
#pragma once
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* clearly define what should be visible in the shared object...
 * visibility is set to hidden in the wscript
 */
#define SYMBOL_VISIBLE __attribute__ ((visibility ("default")))
#define SYMBOL_HIDDEN __attribute__ ((visibility ("hidden")))

#define HWDB4C_SUCCESS 0
#define HWDB4C_FAILURE -1

typedef struct in_addr ip_addr_t; // in network byte order
struct SYMBOL_VISIBLE hwdb4c_database_t;

struct SYMBOL_VISIBLE hwdb4c_fpga_entry {
	//key
	size_t fpgaglobal_id;

	//values
	ip_addr_t ip;
	bool highspeed;
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
	struct hwdb4c_hicann_entry** hicanns;
	size_t num_hicann_entries;
	struct hwdb4c_adc_entry** adcs;
	size_t num_adc_entries;
	ip_addr_t macu_ip;
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

// check if entry in in hwdb, return HWDB4C_SUCCESS on success, on error returns HWDB4C_FAILURE
int hwdb4c_has_fpga_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_hicann_entry(struct hwdb4c_database_t* handle, size_t hicannglobal_id, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_adc_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, size_t analogonhicann, bool* ret) SYMBOL_VISIBLE;
int hwdb4c_has_wafer_entry(struct hwdb4c_database_t* handle, size_t wafer_id, bool* ret) SYMBOL_VISIBLE;

// get entry from hwdb, if entry not in hwdb or invalid coord returns HWDB4C_FAILURE
// onwership of entries lies with user, use corresponding hwdb4c_free_xxx_entry function to free memory
int hwdb4c_get_fpga_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_fpga_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_hicann_entry(struct hwdb4c_database_t* handle, size_t hicannglobal_id, struct hwdb4c_hicann_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_adc_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, size_t analogonhicann, struct hwdb4c_adc_entry** ret) SYMBOL_VISIBLE;
int hwdb4c_get_wafer_entry(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_wafer_entry** ret) SYMBOL_VISIBLE;

// returns all Wafer IDs in database as size_t array of size num_wafer, ownership of array lies with user
int hwdb4c_get_wafer_coordinates(struct hwdb4c_database_t* handle, size_t** wafer, size_t* num_wafer) SYMBOL_VISIBLE;

// get array of entries, size of array given with num_xxx, if num_xxx ist zero than pointer is NULL
// return HWDB4C_SUCCESS on success, if coord invalid returns HWDB4C_FAILURE
// onwership of entries lies with user, use corresponding hwdb4c_free_xxx_entry function to free memory
int hwdb4c_get_adc_entries_of_Wafer(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_adc_entry*** adcs, size_t* num_adcs) SYMBOL_VISIBLE;
int hwdb4c_get_adc_entries_of_FPGAGlobal(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_adc_entry*** adcs, size_t* num_adcs) SYMBOL_VISIBLE;
int hwdb4c_get_hicann_entries_of_Wafer(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_hicann_entry*** hicanns, size_t* num_hicanns) SYMBOL_VISIBLE;
int hwdb4c_get_hicann_entries_of_FPGAGlobal(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_hicann_entry*** hicanns, size_t* num_hicanns) SYMBOL_VISIBLE;

// free memory of an entry
void hwdb4c_free_fpga_entry(struct hwdb4c_fpga_entry* fpga) SYMBOL_VISIBLE;
void hwdb4c_free_hicann_entry(struct hwdb4c_hicann_entry* hicann) SYMBOL_VISIBLE;
void hwdb4c_free_adc_entry(struct hwdb4c_adc_entry* adc) SYMBOL_VISIBLE;
void hwdb4c_free_wafer_entry(struct hwdb4c_wafer_entry* wafer) SYMBOL_VISIBLE;
void hwdb4c_free_hicann_entries(struct hwdb4c_hicann_entry** hicanns, size_t num_hicanns) SYMBOL_VISIBLE;
void hwdb4c_free_adc_entries(struct hwdb4c_adc_entry** adcs, size_t num_adcs) SYMBOL_VISIBLE;

//convert functions for HALbe coordinates
//FIXME should be its own API
size_t hwdb4c_FPGAOnWafer_size() SYMBOL_VISIBLE;
size_t hwdb4c_HICANNOnWafer_size() SYMBOL_VISIBLE;
size_t hwdb4c_master_FPGA_enum() SYMBOL_VISIBLE;
int hwdb4c_ReticleOnWafer_toFPGAOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_FPGAOnWafer_toReticleOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_FPGAOnWafer_toTriggerOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_toReticleOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;
int hwdb4c_HICANNOnWafer_toFPGAOnWafer(size_t id, size_t* ret) SYMBOL_VISIBLE;

#ifdef __cplusplus
} // extern "C"
#endif
