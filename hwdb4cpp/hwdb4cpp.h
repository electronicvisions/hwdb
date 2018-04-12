#pragma once

#include <map>
#include <string>

#include "halco/hicann/v2/coordinates.h"

namespace hwdb4cpp {

/// YAML Hardware Database File Format:
/// ===================================
///
/// Each wafer is a seperate YAML document (YAML documents in a single file
/// are seperated by a line containing '---').
/// The document has a map entry with the keys:
///  - wafer: Numerical coordinate of the wafer
///  - setuptype: vsetup facetswafer cubesetup bsswafer (case-insensitive)
///  - adcs: A sequence of mappings describing ADCs connection (see below)
///  - fpgas: A sequence of mappings describing the fpga setting (see below)
///  - hicanns: Either a sequence of the individual available HICANNs (see below)
///             or a mapping as shorthand for whole wafers(see below).
///  - macu: ip of Main System Control Unit (raspberry pi) (default: 0.0.0.0)
///
/// ADC sequence entries. Each entry discribes a single connection between a
/// HICANN analog output an a ADC input channel. It is a mapping containing
/// the following entries:
///  - fpga: Coordinate of the FPGA
///  - analog: Analog output of the FPGA/HICANN
///  - adc: Coordiante of the ADC
///  - channel: Channel of the ADC
///  - trigger: Channel of the trigger
///  - dnc_on_fpga: Coordinate of the DNC on FPGA (optional, required only
///                 for old facet systems)
///
/// FPGA sequence entries. Each entry describes an FPGA available in the
/// system. Each is a mapping containing the following entries:
///  - fpga: Coordinate of the FPGA
///  - ip: IP of the FPGA
///  - highspeed: Discribes, if there is a physical highspeed connection
///               available (optional, default true)
///
/// HICANN sequence entries. Each entry describes an HICANN available in the
/// system. Each is a mapping containing the following entries:
///  - hicann: Coordinate of the HCIANN
///  - version: HICANN version
///  - label: String, describing the HICANN (for HICANN-boards on the cube setups)///
///
/// HICANN mapping entry. As shorthand for a whole wafer, where all HICANNs
/// are connected, the HICANN sequence can be replaced by a map. In this case
/// all HICANNs on all specified FPGAs will be available. The mapping contains:
///  - version: HICANN version


/* HWDB Entry Types: Structs representing the data in the YAML database */
/* ******************************************************************** */

struct FPGAEntry
{
	halco::hicann::v2::IPv4 ip;
	bool highspeed;
};

struct HICANNEntry
{
	size_t version;
	std::string label;
};

struct ADCEntry
{
	enum CalibrationMode
	{
		LOAD_CALIBRATION,
		ESS_CALIBRATION,
		DEFAULT_CALIBRATION
	};
	CalibrationMode loadCalibration;
	std::string coord;
	halco::hicann::v2::ChannelOnADC channel;
	halco::hicann::v2::TriggerOnADC trigger;
	halco::hicann::v2::IPv4 remote_ip;
	halco::hicann::v2::TCPPort remote_port;
};

typedef std::pair< halco::hicann::v2::FPGAGlobal, halco::hicann::v2::AnalogOnHICANN> GlobalAnalog_t;
typedef std::map< GlobalAnalog_t, ADCEntry> ADCEntryMap;
typedef std::map< halco::hicann::v2::FPGAGlobal, FPGAEntry> FPGAEntryMap;
typedef std::map< halco::hicann::v2::HICANNGlobal, HICANNEntry> HICANNEntryMap;

struct WaferEntry
{
	halco::hicann::v2::SetupType setup_type;
	ADCEntryMap adcs;
	FPGAEntryMap fpgas;
	HICANNEntryMap hicanns;
	halco::hicann::v2::IPv4 macu;
};
/* ******************************************************************** */


/// This class provides an interface to the low-level database.
class database
{
public:

	/// clear object
	void clear();

	/// load database from file
	void load(std::string const path);

	/// dump database
	void dump(std::ostream& out) const;

	/// get default_path member
	static std::string const& get_default_path();

	// FIXME: add const getters everywhere?

	/// Insert (and replace) a new wafer entry into the database
	void add_wafer_entry(halco::hicann::v2::Wafer const wafer, WaferEntry const entry);
	bool remove_wafer_entry(halco::hicann::v2::Wafer const wafer);
	/// Check if wafer entry exists
	bool has_wafer_entry(halco::hicann::v2::Wafer const wafer) const;
	WaferEntry& get_wafer_entry(halco::hicann::v2::Wafer const wafer);
	WaferEntry const& get_wafer_entry(halco::hicann::v2::Wafer const wafer) const;
	std::vector<halco::hicann::v2::Wafer> get_wafer_coordinates() const;

	/// Insert (and replace) an FPGA into the database.
	/// The corresponding WaferEntry has to exist.
	void add_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga, FPGAEntry const entry);
	bool remove_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga);
	/// Check if fpga entry exists
	bool has_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga) const;
	/// Get fpga entry (throws if coordinate isn't found)
	FPGAEntry const& get_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga) const;
	/// Get all entries for all FPGAs on a Wafer
	FPGAEntryMap get_fpga_entries(halco::hicann::v2::Wafer const wafer) const;

	/// Insert (and replace) a HICANN into the database.
	/// The corresponding FPGAEntry has to exist.
	void add_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann, HICANNEntry const entry);
	bool remove_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann);
	/// Check if hicann entry exists
	bool has_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann) const;
	/// Get hicann entry (throws if coordinate isn't found)
	HICANNEntry const& get_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann) const;
	/// Get all entries for all HICANNs on a Wafer
	HICANNEntryMap get_hicann_entries(halco::hicann::v2::Wafer const wafer) const;
	/// Get all entries for all HICANNs on a FPGAGlobal
	HICANNEntryMap get_hicann_entries(halco::hicann::v2::FPGAGlobal const fpga) const;

	/// Insert (and replace) an ADC  into the database.
	/// The corresponding Wafer has to exist.
	void add_adc_entry(GlobalAnalog_t const analog, ADCEntry const entry);
	bool remove_adc_entry(GlobalAnalog_t const analog);
	/// Check if adc entry exists
	bool has_adc_entry(GlobalAnalog_t const analog) const;
	/// Get adc entry (throws if coordinate isn't found)
	ADCEntry const& get_adc_entry(GlobalAnalog_t const analog) const;
	/// Get all entries for all ADCs on a Wafer
	ADCEntryMap get_adc_entries(halco::hicann::v2::Wafer const wafer) const;
	/// Get all entries for all ADCs on a FPGAGlobal
	ADCEntryMap get_adc_entries(halco::hicann::v2::FPGAGlobal const fpga) const;

private:
	// used by yaml-cpp => FIXME: change to add_{fpga,hicann,adc}_entry
	void add_fpga(halco::hicann::v2::FPGAGlobal const, const FPGAEntry& data);
	void add_hicann(halco::hicann::v2::HICANNGlobal const, const HICANNEntry& data);
	void add_adc(GlobalAnalog_t const, const ADCEntry& data);

	std::map<halco::hicann::v2::Wafer, WaferEntry> mData;

	static std::string const default_path;
};

} // namespace hwdb4cpp
