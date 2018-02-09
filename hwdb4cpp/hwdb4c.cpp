#include "hwdb4c.h"
#include "halco/common/iter_all.h"
#include "hwdb4cpp.h"

#include <fstream>
#include <iostream>

#define HWDB4C_MAX_STRING_LENGTH 200
#define HWDB4C_DEFAULT_WAFER_ID 20

using namespace halco::common;
using namespace halco::hicann::v2;

extern "C" {

struct hwdb4c_database_t {
	hwdb4cpp::database database;
};

// converts hwdb4cpp::FPGAEntry to hwdb4c_fpga_entry
int _convert_fpga_entry(hwdb4cpp::FPGAEntry fpga_entry_cpp, FPGAGlobal fpgacoord, struct hwdb4c_fpga_entry** ret) {
	struct hwdb4c_fpga_entry* fpga_entry_c = (hwdb4c_fpga_entry*) malloc(sizeof(struct hwdb4c_fpga_entry));
	if (!fpga_entry_c)
		return HWDB4C_FAILURE;
	fpga_entry_c->fpgaglobal_id = fpgacoord.toEnum();
	inet_aton(fpga_entry_cpp.ip.to_string().c_str(), &(fpga_entry_c->ip));
	fpga_entry_c->highspeed = fpga_entry_cpp.highspeed;
	*ret = fpga_entry_c;
	return HWDB4C_SUCCESS;
}

// converts hwdb4cpp::HICANNEntry to hwdb4c_hicann_entry
int _convert_hicann_entry(hwdb4cpp::HICANNEntry hicann_entry_cpp, HICANNGlobal hicanncoord, struct hwdb4c_hicann_entry** ret) {
	struct hwdb4c_hicann_entry* hicann_entry_c = (hwdb4c_hicann_entry*) malloc(sizeof(struct hwdb4c_hicann_entry));
	if (!hicann_entry_c)
		return HWDB4C_FAILURE;
	hicann_entry_c->hicannglobal_id = hicanncoord.toEnum();
	hicann_entry_c->version = hicann_entry_cpp.version;
	size_t max_length = (hicann_entry_cpp.label.size() + 1 > HWDB4C_MAX_STRING_LENGTH) ? HWDB4C_MAX_STRING_LENGTH : hicann_entry_cpp.label.size() + 1;
	hicann_entry_c->label = (char*) malloc(hicann_entry_cpp.label.size() + 1); // + 1 for \0
	if (!hicann_entry_c->label)
		return HWDB4C_FAILURE;
	strncpy(hicann_entry_c->label, hicann_entry_cpp.label.c_str(), max_length);
	*ret = hicann_entry_c;
	return HWDB4C_SUCCESS;
}

// converts hwdb4cpp::ADCEntry to hwdb4c_adc_entry
int _convert_adc_entry(hwdb4cpp::ADCEntry adc_entry_cpp, hwdb4cpp::GlobalAnalog_t key, struct hwdb4c_adc_entry** ret) {
	struct hwdb4c_adc_entry* adc_entry_c = (hwdb4c_adc_entry*) malloc(sizeof(struct hwdb4c_adc_entry));
	if (!adc_entry_c)
		return HWDB4C_FAILURE;
	adc_entry_c->fpgaglobal_id = key.first.toEnum();
	adc_entry_c->analogout = key.second.toEnum();
	adc_entry_c->calibration_mode = static_cast<hwdb4c_adc_entry::calibration_mode_t>(adc_entry_cpp.loadCalibration);
	size_t max_length = (adc_entry_cpp.coord.size() + 1 > HWDB4C_MAX_STRING_LENGTH) ? HWDB4C_MAX_STRING_LENGTH : adc_entry_cpp.coord.size() + 1;
	adc_entry_c->coord = (char*) malloc(adc_entry_cpp.coord.size() + 1); // + 1 for \0
	if (!adc_entry_c->coord)
		return HWDB4C_FAILURE;
	strncpy(adc_entry_c->coord, adc_entry_cpp.coord.c_str(), max_length);
	adc_entry_c->channel = adc_entry_cpp.channel.value();
	adc_entry_c->trigger = adc_entry_cpp.trigger.value();
	inet_aton(adc_entry_cpp.remote_ip.to_string().c_str(), &(adc_entry_c->remote_ip));
	adc_entry_c->remote_port = adc_entry_cpp.remote_port.value();
	*ret = adc_entry_c;
	return HWDB4C_SUCCESS;
}

// converts hwdb4cpp::WaferEntry to hwdb4c_wafer_entry
int _convert_wafer_entry(hwdb4cpp::WaferEntry wafer_entry_cpp, Wafer wafercoord, struct hwdb4c_wafer_entry** ret) {
	struct hwdb4c_wafer_entry* wafer_entry_c = (hwdb4c_wafer_entry*) malloc(sizeof(struct hwdb4c_wafer_entry));
	if (!wafer_entry_c)
		return HWDB4C_FAILURE;
	wafer_entry_c->wafer_id = wafercoord.value();
	wafer_entry_c->setup_type = static_cast<hwdb4c_wafer_entry::setup_type_t>(wafer_entry_cpp.setup_type);

	wafer_entry_c->num_fpga_entries = wafer_entry_cpp.fpgas.size();
	wafer_entry_c->fpgas = (hwdb4c_fpga_entry**) malloc(sizeof(struct hwdb4c_fpga_entry*) * wafer_entry_c->num_fpga_entries);
	if (!wafer_entry_c->fpgas)
		return HWDB4C_FAILURE;
	size_t fpga_counter = 0;
	for (auto fpga_it = wafer_entry_cpp.fpgas.begin(); fpga_it != wafer_entry_cpp.fpgas.end(); fpga_it++) {
		if(_convert_fpga_entry(fpga_it->second, fpga_it->first, &(wafer_entry_c->fpgas[fpga_counter])) == HWDB4C_FAILURE)
			 return HWDB4C_FAILURE;
		fpga_counter++;
	}

	wafer_entry_c->num_hicann_entries = wafer_entry_cpp.hicanns.size();
	wafer_entry_c->hicanns = (hwdb4c_hicann_entry**) malloc(sizeof(struct hwdb4c_hicann_entry*) * wafer_entry_c->num_fpga_entries);
	if (!wafer_entry_c->hicanns)
		return HWDB4C_FAILURE;
	size_t hicann_counter = 0;
	for (auto hicann_it = wafer_entry_cpp.hicanns.begin(); hicann_it != wafer_entry_cpp.hicanns.end(); hicann_it++) {
		if(_convert_hicann_entry(hicann_it->second, hicann_it->first, &(wafer_entry_c->hicanns[hicann_counter])) == HWDB4C_FAILURE)
			 return HWDB4C_FAILURE;
		hicann_counter++;
	}
	wafer_entry_c->num_adc_entries = wafer_entry_cpp.adcs.size();

	wafer_entry_c->num_adc_entries = wafer_entry_cpp.adcs.size();
	wafer_entry_c->adcs = (hwdb4c_adc_entry**) malloc(sizeof(struct hwdb4c_adc_entry*) * wafer_entry_c->num_fpga_entries);
	if (!wafer_entry_c->adcs)
		return HWDB4C_FAILURE;
	size_t adc_counter = 0;
	for (auto adc_it = wafer_entry_cpp.adcs.begin(); adc_it != wafer_entry_cpp.adcs.end(); adc_it++) {
		if(_convert_adc_entry(adc_it->second, adc_it->first, &(wafer_entry_c->adcs[adc_counter])) == HWDB4C_FAILURE)
			 return HWDB4C_FAILURE;
		adc_counter++;
	}

	inet_aton(wafer_entry_cpp.macu.to_string().c_str(), &(wafer_entry_c->macu_ip));
	*ret = wafer_entry_c;
	return HWDB4C_SUCCESS;
}

int hwdb4c_alloc_hwdb(struct hwdb4c_database_t** handle) {
	try {
		*handle = new struct hwdb4c_database_t();
	}
	catch (...) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_load_hwdb(struct hwdb4c_database_t* handle, char const* hwdb_path) {
	std::string path;
	if (hwdb_path == NULL)
		path = handle->database.get_default_path();
	else
		path = std::string(hwdb_path);
	try {
		handle->database.load(path);
	} catch(const std::exception& oor) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_store_hwdb(struct hwdb4c_database_t* handle, char const* hwdb_path)
{
	std::string path;
	if (hwdb_path == NULL)
		path = handle->database.get_default_path();
	else
		path = std::string(hwdb_path);
	try {
		std::ofstream out(path);
		handle->database.dump(out);
	} catch (const std::exception& oor) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

void hwdb4c_clear_hwdb(struct hwdb4c_database_t* handle)
{
	handle->database.clear();
}

void hwdb4c_free_hwdb(struct hwdb4c_database_t* handle) {
	delete(handle);
}

int hwdb4c_has_wafer_entry(struct hwdb4c_database_t* handle, size_t wafer_id, bool* ret) {
	try {
		*ret = handle->database.has_wafer_entry(Wafer(wafer_id));
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_has_fpga_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, bool* ret) {
	try {
		*ret = handle->database.has_fpga_entry(FPGAGlobal(Enum(fpgaglobal_id)));
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_has_hicann_entry(struct hwdb4c_database_t* handle, size_t hicannglobal_id, bool* ret) {
	try {
		*ret = handle->database.has_hicann_entry(HICANNGlobal(Enum(hicannglobal_id)));
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_has_adc_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, size_t analogonhicann, bool* ret) {
	try {
		*ret = handle->database.has_adc_entry(hwdb4cpp::GlobalAnalog_t(FPGAGlobal(Enum(fpgaglobal_id)), AnalogOnHICANN(uint8_t(analogonhicann))));
	} catch(const std::overflow_error& hdke) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_get_fpga_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_fpga_entry** ret) {
	hwdb4cpp::FPGAEntry fpga_entry_cpp;
	try {
		fpga_entry_cpp = handle->database.get_fpga_entry(FPGAGlobal(Enum(fpgaglobal_id)));
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	return _convert_fpga_entry(fpga_entry_cpp, FPGAGlobal(Enum(fpgaglobal_id)), ret);
}

int hwdb4c_get_hicann_entry(struct hwdb4c_database_t* handle, size_t hicannglobal_id, struct hwdb4c_hicann_entry** ret) {
	hwdb4cpp::HICANNEntry hicann_entry_cpp;
	try {
		hicann_entry_cpp = handle->database.get_hicann_entry(HICANNGlobal(Enum(hicannglobal_id)));
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	return _convert_hicann_entry(hicann_entry_cpp, HICANNGlobal(Enum(hicannglobal_id)), ret);
}

int hwdb4c_get_adc_entry(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, size_t analogonhicann, struct hwdb4c_adc_entry** adc) {
	hwdb4cpp::ADCEntry adc_entry_cpp;
	hwdb4cpp::GlobalAnalog_t adc_key;
	try {
		adc_key = hwdb4cpp::GlobalAnalog_t(FPGAGlobal(Enum(fpgaglobal_id)), AnalogOnHICANN(uint8_t(analogonhicann)));
		adc_entry_cpp = handle->database.get_adc_entry(adc_key);
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	bool returnval = _convert_adc_entry(adc_entry_cpp, adc_key, adc);
	return returnval;
}

int hwdb4c_get_wafer_entry(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_wafer_entry** ret) {
	hwdb4cpp::WaferEntry wafer_entry_cpp;
	try {
		wafer_entry_cpp = handle->database.get_wafer_entry(Wafer(wafer_id));
	} catch(const std::out_of_range& hdke) {
		return HWDB4C_FAILURE;
	}
	return _convert_wafer_entry(wafer_entry_cpp, Wafer(wafer_id), ret);
}

int hwdb4c_get_hicann_entries_of_FPGAGlobal(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_hicann_entry*** hicanns, size_t* num_hicanns) {
	std::map<HICANNGlobal, hwdb4cpp::HICANNEntry> hicann_map;
	try {
		hicann_map = handle->database.get_hicann_entries(FPGAGlobal(Enum(fpgaglobal_id)));
	} catch(const std::out_of_range& oor) {
		return HWDB4C_FAILURE;
	}
	*num_hicanns = hicann_map.size();
	*hicanns = (hwdb4c_hicann_entry**) malloc(sizeof(struct hwdb4c_hicann_entry*) * *num_hicanns);
	if(!*hicanns)
		return HWDB4C_FAILURE;
	size_t hicann_counter = 0;
	for (auto hicann_it = hicann_map.begin(); hicann_it != hicann_map.end(); hicann_it++) {
		if(_convert_hicann_entry(hicann_it->second, hicann_it->first, &((*hicanns)[hicann_counter])) == HWDB4C_FAILURE)
			return HWDB4C_FAILURE;
		hicann_counter++;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_get_hicann_entries_of_Wafer(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_hicann_entry*** hicanns, size_t* num_hicanns) {
	std::map<HICANNGlobal, hwdb4cpp::HICANNEntry> hicann_map;
	try {
		hicann_map = handle->database.get_hicann_entries(Wafer(wafer_id));
	} catch(const std::out_of_range& oor) {
		return HWDB4C_FAILURE;
	}
	*num_hicanns = hicann_map.size();
	*hicanns = (hwdb4c_hicann_entry**) malloc(sizeof(struct hwdb4c_hicann_entry*) * *num_hicanns);
	if(!*hicanns)
		return HWDB4C_FAILURE;
	size_t hicann_counter = 0;
	for (auto hicann_it = hicann_map.begin(); hicann_it != hicann_map.end(); hicann_it++) {
		if(_convert_hicann_entry(hicann_it->second, hicann_it->first, &(*hicanns)[hicann_counter]) == HWDB4C_FAILURE)
			return HWDB4C_FAILURE;
		hicann_counter++;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_get_adc_entries_of_Wafer(struct hwdb4c_database_t* handle, size_t wafer_id, struct hwdb4c_adc_entry*** adcs, size_t* num_adcs) {
	std::map<hwdb4cpp::GlobalAnalog_t, hwdb4cpp::ADCEntry> adc_map;
	try {
		adc_map = handle->database.get_adc_entries(Wafer(wafer_id));
	} catch(const std::out_of_range& oor) {
		return HWDB4C_FAILURE;
	}
	*num_adcs = adc_map.size();
	*adcs = (hwdb4c_adc_entry**) malloc(sizeof(struct hwdb4c_adc_entry*) * *num_adcs);
	if(!*adcs)
		return HWDB4C_FAILURE;
	size_t adc_counter = 0;
	for (auto adc_it = adc_map.begin(); adc_it != adc_map.end(); adc_it++) {
		if(_convert_adc_entry(adc_it->second, adc_it->first, &(*adcs)[adc_counter]) == HWDB4C_FAILURE)
			return HWDB4C_FAILURE;
		adc_counter++;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_get_adc_entries_of_FPGAGlobal(struct hwdb4c_database_t* handle, size_t fpgaglobal_id, struct hwdb4c_adc_entry*** adcs, size_t* num_adcs) {
	std::map<hwdb4cpp::GlobalAnalog_t, hwdb4cpp::ADCEntry> adc_map;
	try {
		adc_map = handle->database.get_adc_entries(FPGAGlobal(Enum(fpgaglobal_id)));
	} catch(const std::out_of_range& oor) {
		return HWDB4C_FAILURE;
	}
	*num_adcs = adc_map.size();
	*adcs = (hwdb4c_adc_entry**) malloc(sizeof(struct hwdb4c_adc_entry*) * *num_adcs);
	if(!*adcs)
		return HWDB4C_FAILURE;
	size_t adc_counter = 0;
	for (auto adc_it = adc_map.begin(); adc_it != adc_map.end(); adc_it++) {
		if(_convert_adc_entry(adc_it->second, adc_it->first, &(*adcs)[adc_counter]) == HWDB4C_FAILURE)
			return HWDB4C_FAILURE;
		adc_counter++;
	}
	return HWDB4C_SUCCESS;
}

void hwdb4c_free_fpga_entry(struct hwdb4c_fpga_entry* fpga) {
	free(fpga);
}

void hwdb4c_free_hicann_entry(struct hwdb4c_hicann_entry* hicann) {
	free(hicann->label);
	free(hicann);
}

void hwdb4c_free_adc_entry(struct hwdb4c_adc_entry* adc) {
	free(adc);
}

void hwdb4c_free_wafer_entry(struct hwdb4c_wafer_entry* wafer) {
	for (size_t fpga_counter = 0; fpga_counter < wafer->num_fpga_entries; fpga_counter++) {
		hwdb4c_free_fpga_entry(wafer->fpgas[fpga_counter]);
	}
	free(wafer->fpgas);
	for (size_t hicann_counter = 0; hicann_counter < wafer->num_hicann_entries; hicann_counter++) {
		hwdb4c_free_hicann_entry(wafer->hicanns[hicann_counter]);
	}
	free(wafer->hicanns);
	for (size_t adc_counter = 0; adc_counter < wafer->num_adc_entries; adc_counter++) {
		hwdb4c_free_adc_entry(wafer->adcs[adc_counter]);
	}
	free(wafer->adcs);
	free(wafer);
}

void hwdb4c_free_hicann_entries(struct hwdb4c_hicann_entry** hicanns, size_t num_hicanns) {
	size_t hicanncounter;
	for (hicanncounter = 0; hicanncounter < num_hicanns; hicanncounter++) {
		hwdb4c_free_hicann_entry(hicanns[hicanncounter]);
	}
	free(hicanns);
}

void hwdb4c_free_adc_entries(struct hwdb4c_adc_entry** adcs, size_t num_adcs) {
	size_t adccounter;
	for (adccounter = 0; adccounter < num_adcs; adccounter++) {
		hwdb4c_free_adc_entry(adcs[adccounter]);
	}
	free(adcs);
}

size_t hwdb4c_FPGAOnWafer_size() {
	return FPGAOnWafer::size;
}

size_t hwdb4c_HICANNOnWafer_size()
{
	return HICANNOnWafer::size;
}

size_t hwdb4c_master_FPGA_enum()
{
	return FPGAOnWafer::Master.toEnum().value();
}

int hwdb4c_ReticleOnWafer_toFPGAOnWafer(size_t id, size_t* ret) {
	try {
		 *ret = DNCGlobal(DNCOnWafer(Enum(id)), Wafer(HWDB4C_DEFAULT_WAFER_ID)).toFPGAOnWafer().value();
	} catch(const std::overflow_error& oor) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_FPGAOnWafer_toReticleOnWafer(size_t id, size_t* ret) {
	try {
		*ret = gridLookupDNCGlobal(FPGAGlobal(FPGAOnWafer(Enum(id)), Wafer(HWDB4C_DEFAULT_WAFER_ID)), DNCOnFPGA(Enum(0))).toDNCOnWafer().id().value() ;
	} catch(const std::overflow_error& oor) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_HICANNOnWafer_toReticleOnWafer(size_t id, size_t* ret) {
	try {
		*ret = HICANNOnWafer(Enum(id)).toDNCOnWafer().id().value();
	} catch(const std::overflow_error& oor) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

int hwdb4c_HICANNOnWafer_toFPGAOnWafer(size_t id, size_t* ret) {
	try {
		*ret = HICANNGlobal(HICANNOnWafer(Enum(id)), Wafer(HWDB4C_DEFAULT_WAFER_ID)).toFPGAOnWafer().value();
	} catch(const std::overflow_error& oor) {
		return HWDB4C_FAILURE;
	}
	return HWDB4C_SUCCESS;
}

} // extern "C"
