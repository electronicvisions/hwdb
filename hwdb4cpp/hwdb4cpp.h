#pragma once

#include <map>
#include <string>
#ifndef PYPLUSPLUS
#include <array>
#include <optional>
#endif

#include "genpybind.h"
#include "halco/common/misc_types.h"
#include "halco/hicann/v2/coordinates.h"
#include "hate/visibility.h"

namespace hwdb4cpp GENPYBIND_TAG_HWDB {

/// YAML Hardware Database File Format:
/// ===================================
///
/// Each wafer/DLS setup is a seperate YAML document (YAML documents in a single
/// file are seperated by a line containing '---').
///
/// The wafer entries have a map entry with the keys:
///  - wafer: Numerical coordinate of the wafer
///  - setuptype: vsetup facetswafer cubesetup bsswafer (case-insensitive)
///  - adcs: A sequence of mappings describing ADCs connection (see below)
///  - ananas: A sequence of mappings describing Ananas setting (see below)
///  - fpgas: A sequence of mappings describing the fpga setting (see below)
///  - reticles: A sequence of mappings describing the reticle setting (see below)
///  - hicanns: Either a sequence of the individual available HICANNs (see below)
///             or a mapping as shorthand for whole wafers(see below).
///  - macu: ip of Main System Control Unit (raspberry pi) (default: 0.0.0.0)
///
/// ADC sequence entries. Each entry discribes a single connection between a
/// HICANN analog output an a ADC input channel. It is a mapping containing
/// the following entries:
///  - fpga: Coordinate of the FPGA
///  - analog: Analog output of the FPGA/HICANN
///  - adc: Coordinate of the ADC
///  - channel: Channel of the ADC
///  - trigger: Channel of the trigger
///  - dnc_on_fpga: Coordinate of the DNC on FPGA (optional, required only
///                 for old facet systems)
///
/// Ananas sequence entries. Each entry describes an Ananas available in
/// the system. Each is a mapping containing the following entries:
///  - ananas: Coordinate of the Ananas
///  - ip: IP of the FlySpi on Ananas
///  - baseport_data: base UDP data port for all AnanasSlices
///  - baseport_reset: base UDP reset port for all AnanasSlices
///
/// FPGA sequence entries. Each entry describes an FPGA available in the
/// system. Each is a mapping containing the following entries:
///  - fpga: Coordinate of the FPGA
///  - ip: IP of the FPGA
///  - highspeed: Discribes, if there is a physical highspeed connection
///               available (optional, default true)
///
/// Reticle sequence entries. Each entry describes a Reticle available in the
/// system. Each is a mapping containing the following entries:
///  - reticle: Coordinate of the Reticle (DNCOnWafer)
///  - to_be_powered: boolean flag if reticle is allowed to be powered
///
/// HICANN sequence entries. Each entry describes an HICANN available in the
/// system. Each is a mapping containing the following entries:
///  - hicann: Coordinate of the HICANN
///  - version: HICANN version
///  - label: String, describing the HICANN (for HICANN-boards on the cube setups)///
///
/// HICANN mapping entry. As shorthand for a whole wafer, where all HICANNs
/// are connected, the HICANN sequence can be replaced by a map. In this case
/// all HICANNs on all specified FPGAs will be available. The mapping contains:
///  - version: HICANN version
///
/// The DLS setups have a map entry with the keys:
///  - fpga_name: Individual FPGA id as string
///  - board_version: Version of the baseboard
///  - chip_id: Individual id of the chip
///  - chip_version: Version of the DLS-chip
///  - ntpwr_ip: IP of the correspoding Netpower for remote power cycling
///  - ntpwr_slot: Slot of the setup in the Netpower
//
/// HX cube wing. Describes the unity of IO board, carrier board and Chip of
/// one of the sides (a wing) of a Cube setup:
///  - handwritten_chip_serial: Manually written (in decimal) unique ID on chip
///    carrier (valid range: [1,...])
///  - chip_revision: HICANN-X revision (int)
///  - (optional)eeprom_chip_serial: EEPROM serial on chip carrier (hex)
///  - (optional)synram_timing_pcconf: Array holding the pcconf timing configurations for both
///  hemispheres
///  - (optional)synram_timing_wconf: Array holding the wconf timing configurations for both
///  hemispheres
//
/// HX cube FPGA sequence entry:
///  - ip: FPGA Ethernet IP
///  - (optional)wing: HXCubeWingEntry corresponding to the FPGA
///  - (optional)fuse_dna: FPGA FUSE_DNA as provided by Xilinx toolchains, is
///                        required to identify the device in JTAG chain
///  - (optional)extoll_node_id: Node ID of the FPGA in the Extoll network
///  - (optional, default false)ci_test_node: Allow CI tests to be run on this FPGA
/// HX cube FPGA member functions:
///  - get_dna_port: calculates the FPGA-internal DNA_PORT sequence (which can be
///                  read out from the fabric logic) from the FUSE_DNA
///
/// HX cube setups have a map entry with the keys:
///  - hxcube_id: wafer id = hxcube_id + 60, defining FPGA IP range
///  - fpgas: map of HXCubeFPGAEntries
///  - usb_host: name of host that connects to MSP430 on cube-io
///  - usb_serial: serial code of MSP430 (as string)
///  - (optional)xilinx_hw_server: hostname/IP and port of xilinx hardware server
///                                (e.g. "xilinx-smartlynq-0:3121")
///
/// jBOA aggregators (aka KRACEN) entry
///  - ip: FPGA Ethernet IP (1GBASE-T PL interface)
///  - (optional, default false)ci_test_node: Allow CI tests to be run on this FPGA
///
/// jBOA setup (re-uses HXCubeFPGAEntries):
///  - jboa_id: wafer id = jboa_id + 80
///  - fpgas: map of HXCubeFPGAEntries
///  - aggregators: map of JboaAggregatorEntry
///  - (optional)xilinx_hw_server: hostname/IP and port of xilinx hardware server
///                                (e.g. "xilinx-smartlynq-0:3121")

/* HWDB Entry Types: Structs representing the data in the YAML database */
/* ******************************************************************** */

struct FPGAEntry
{
	halco::hicann::v2::IPv4 ip;
	bool highspeed;
};

struct ReticleEntry
{
	bool to_be_powered;
};

struct AnanasEntry
{
	halco::hicann::v2::IPv4 ip;
	halco::hicann::v2::UDPPort baseport_data;
	halco::hicann::v2::UDPPort baseport_reset;
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
typedef std::map< halco::hicann::v2::DNCGlobal, ReticleEntry> ReticleEntryMap;
typedef std::map<halco::hicann::v2::AnanasGlobal, AnanasEntry> AnanasEntryMap;
typedef std::map< halco::hicann::v2::HICANNGlobal, HICANNEntry> HICANNEntryMap;

struct WaferEntry
{
	halco::hicann::v2::SetupType setup_type;
	ADCEntryMap adcs;
	FPGAEntryMap fpgas;
	ReticleEntryMap reticles;
	AnanasEntryMap ananas;
	HICANNEntryMap hicanns;
	halco::hicann::v2::IPv4 macu;
	size_t macu_version;
};

struct GENPYBIND(visible) DLSSetupEntry
{
	std::string fpga_name;
	std::string board_name;
	size_t board_version;
	size_t chip_id;
	size_t chip_version;
	std::string ntpwr_ip;
	size_t ntpwr_slot;

	DLSSetupEntry()
	{
		ntpwr_ip = " ";
		ntpwr_slot = 0;
	}
};

#ifndef PYPLUSPLUS
struct GENPYBIND(visible) HXCubeWingEntry
{
	size_t handwritten_chip_serial;
	size_t chip_revision;
	std::optional<uint32_t> eeprom_chip_serial;
	std::optional<std::array<std::array<uint16_t, 2>, 2>> synram_timing_pcconf;
	std::optional<std::array<std::array<uint16_t, 2>, 2>> synram_timing_wconf;

	HXCubeWingEntry()
	{
		handwritten_chip_serial = 0; // valid IDs start from 1
		chip_revision = 0;
	}
};

struct GENPYBIND(visible) HXCubeFPGAEntry
{
	halco::common::IPv4 ip;
	std::optional<HXCubeWingEntry> wing;
	std::optional<uint64_t> fuse_dna;
	std::optional<uint16_t> extoll_node_id;
	bool ci_test_node;

	uint64_t get_dna_port() const SYMBOL_VISIBLE;
};

struct GENPYBIND(visible) HXCubeSetupEntry
{
	size_t hxcube_id;
	std::map<size_t, HXCubeFPGAEntry> fpgas;
	std::string usb_host;
	std::string usb_serial;
	std::optional<std::string> xilinx_hw_server;

	HXCubeSetupEntry()
	{
		hxcube_id = 0;
		usb_host = "None";
	}

	std::string get_unique_branch_identifier(size_t chip_serial) const SYMBOL_VISIBLE;

	/**
	 * Get tuple of (hxcube_id, fpga_id, chip_serial, setup_version) from unique branch identifier.
	 * @param identifier Identifier string
	 * @return Tuple of IDs
	 */
	static std::tuple<size_t, size_t, size_t, size_t> get_ids_from_unique_branch_identifier(
	    std::string identifier) SYMBOL_VISIBLE;
};

struct GENPYBIND(visible) JboaAggregatorEntry
{
	halco::common::IPv4 ip;
	bool ci_test_node;
};

struct GENPYBIND(visible) JboaSetupEntry
{
	size_t jboa_id;
	std::map<size_t, HXCubeFPGAEntry> fpgas;
	std::map<size_t, JboaAggregatorEntry> aggregators;
	std::optional<std::string> xilinx_hw_server;

	JboaSetupEntry()
	{
		jboa_id = 0;
	}

	std::string get_unique_branch_identifier(size_t chip_serial) const SYMBOL_VISIBLE;

	/**
	 * Get tuple of (hxcube_id, fpga_id, chip_serial, setup_version) from unique branch identifier.
	 * @param identifier Identifier string
	 * @return Tuple of IDs
	 */
	static std::tuple<size_t, size_t, size_t, size_t> get_ids_from_unique_branch_identifier(
	    std::string identifier) SYMBOL_VISIBLE;
};
#endif
/* ******************************************************************** */


/// This class provides an interface to the low-level database.
class GENPYBIND(visible) database
{
public:

	/// clear object
	void clear() SYMBOL_VISIBLE;

	/// load database from file
	void load(std::string const path) SYMBOL_VISIBLE;

	/// dump database
	void dump(std::ostream& out) const GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// get default_path member
	static std::string const& get_default_path() SYMBOL_VISIBLE;


	/** Query database for and return matching entry in the on-disk (YAML) format.
	 * @param path path to yaml database file
	 * @param node node which is queried, e.g. "wafer", "hxcube_id"
	 * @param query query string, litteraly string of corresponding id, e.g. 6
	 * @return std::string of the matching YAML entries
	 */
	static std::string get_yaml_entries(
	    std::string const& path, std::string const& node, std::string const& query) SYMBOL_VISIBLE;

	// FIXME: add const getters everywhere?

	/// Insert (and replace) a new wafer entry into the database
	void add_wafer_entry(halco::hicann::v2::Wafer const wafer, WaferEntry const entry)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	bool remove_wafer_entry(halco::hicann::v2::Wafer const wafer) GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Check if wafer entry exists
	bool has_wafer_entry(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	WaferEntry& get_wafer_entry(halco::hicann::v2::Wafer const wafer)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	WaferEntry const& get_wafer_entry(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	std::vector<halco::hicann::v2::Wafer> get_wafer_coordinates() const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// Insert (and replace) an FPGA into the database.
	/// The corresponding WaferEntry has to exist.
	void add_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga, FPGAEntry const entry)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	bool remove_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Check if fpga entry exists
	bool has_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get fpga entry (throws if coordinate isn't found)
	FPGAEntry const& get_fpga_entry(halco::hicann::v2::FPGAGlobal const fpga) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all FPGAs on a Wafer
	FPGAEntryMap get_fpga_entries(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// Insert (and replace) an Reticle into the database.
	/// The corresponding WaferEntry has to exist.
	void add_reticle_entry(halco::hicann::v2::DNCGlobal const reticle, ReticleEntry const entry)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	bool remove_reticle_entry(halco::hicann::v2::DNCGlobal const reticle)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Check if reticle entry exists
	bool has_reticle_entry(halco::hicann::v2::DNCGlobal const reticle) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get reticle entry (throws if coordinate isn't found)
	ReticleEntry const& get_reticle_entry(halco::hicann::v2::DNCGlobal const reticle) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all Reticles on a Wafer
	ReticleEntryMap get_reticle_entries(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// Insert (and replace) an Ananas into the database.
	/// The corresponding WaferEntry has to exist.
	void add_ananas_entry(halco::hicann::v2::AnanasGlobal const ananas, AnanasEntry const entry)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	bool remove_ananas_entry(halco::hicann::v2::AnanasGlobal const ananas)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Check if Ananans entry exists
	bool has_ananas_entry(halco::hicann::v2::AnanasGlobal const ananas) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get Ananans entry (throws if coordinate isn't found)
	AnanasEntry const& get_ananas_entry(halco::hicann::v2::AnanasGlobal const ananas) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all Ananas on a Wafer
	AnanasEntryMap get_ananas_entries(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// Insert (and replace) a HICANN into the database.
	/// The corresponding FPGAEntry has to exist.
	void add_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann, HICANNEntry const entry)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	bool remove_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Check if hicann entry exists
	bool has_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get hicann entry (throws if coordinate isn't found)
	HICANNEntry const& get_hicann_entry(halco::hicann::v2::HICANNGlobal const hicann) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all HICANNs on a Wafer
	HICANNEntryMap get_hicann_entries(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all HICANNs on a FPGAGlobal
	HICANNEntryMap get_hicann_entries(halco::hicann::v2::FPGAGlobal const fpga) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// Insert (and replace) an ADC  into the database.
	/// The corresponding Wafer has to exist.
	void add_adc_entry(GlobalAnalog_t const analog, ADCEntry const entry)
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	bool remove_adc_entry(GlobalAnalog_t const analog) GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Check if adc entry exists
	bool has_adc_entry(GlobalAnalog_t const analog) const GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get adc entry (throws if coordinate isn't found)
	ADCEntry const& get_adc_entry(GlobalAnalog_t const analog) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all ADCs on a Wafer
	ADCEntryMap get_adc_entries(halco::hicann::v2::Wafer const wafer) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;
	/// Get all entries for all ADCs on a FPGAGlobal
	ADCEntryMap get_adc_entries(halco::hicann::v2::FPGAGlobal const fpga) const
	    GENPYBIND(hidden) SYMBOL_VISIBLE;

	/// Insert (and replace) a new dls entry into the database
	void add_dls_entry(std::string const dls_setup, DLSSetupEntry const entry) SYMBOL_VISIBLE;
	bool remove_dls_entry(std::string const dls_setup) SYMBOL_VISIBLE;
	/// Check if dls entry exists
	bool has_dls_entry(std::string const dls_setup) const SYMBOL_VISIBLE;
	DLSSetupEntry& get_dls_entry(std::string const dls_setup) SYMBOL_VISIBLE;
	DLSSetupEntry const& get_dls_entry(std::string const dls_setup) const SYMBOL_VISIBLE;
	std::vector<std::string> get_dls_setup_ids() const SYMBOL_VISIBLE;

#ifndef PYPLUSPLUS
	/// Insert (and replace) a new HICANN-X cube setup entry into the database
	void add_hxcube_setup_entry(size_t const hxcube_id, HXCubeSetupEntry const entry)
	    SYMBOL_VISIBLE;
	/// Remove HICANN-X cube setup entry from the database
	bool remove_hxcube_setup_entry(size_t const hxcube_id) SYMBOL_VISIBLE;
	/// Check if entry exists
	bool has_hxcube_setup_entry(size_t const hxcube_id) const SYMBOL_VISIBLE;
	/// Get a HICANN-X cube setup entry
	HXCubeSetupEntry& get_hxcube_setup_entry(size_t const hxcube_id) SYMBOL_VISIBLE;
	/// Get a HICANN-X cube setup entry
	HXCubeSetupEntry const& get_hxcube_setup_entry(size_t const hxcube_id) const SYMBOL_VISIBLE;
	/// Get all HICANN-X cube setup entry ids
	std::vector<size_t> get_hxcube_ids() const SYMBOL_VISIBLE;

	/// Insert (and replace) a new HICANN-X cube setup entry into the database
	void add_jboa_setup_entry(size_t const hxcube_id, JboaSetupEntry const entry) SYMBOL_VISIBLE;
	/// Remove HICANN-X cube setup entry from the database
	bool remove_jboa_setup_entry(size_t const jboa_id) SYMBOL_VISIBLE;
	/// Check if entry exists
	bool has_jboa_setup_entry(size_t const jboa_id) const SYMBOL_VISIBLE;
	/// Get a HICANN-X cube setup entry
	JboaSetupEntry& get_jboa_setup_entry(size_t const jboa_id) SYMBOL_VISIBLE;
	/// Get a HICANN-X cube setup entry
	JboaSetupEntry const& get_jboa_setup_entry(size_t const jboa_id) const SYMBOL_VISIBLE;
	/// Get all HICANN-X cube setup entry ids
	std::vector<size_t> get_jboa_ids() const SYMBOL_VISIBLE;

private:
	// used by yaml-cpp => FIXME: change to add_{fpga,hicann,adc}_entry
	void add_fpga(halco::hicann::v2::FPGAGlobal const, const FPGAEntry& data);
	void add_reticle(halco::hicann::v2::DNCGlobal const, const ReticleEntry& data);
	void add_ananas(halco::hicann::v2::AnanasGlobal const, const AnanasEntry& data);
	void add_hicann(halco::hicann::v2::HICANNGlobal const, const HICANNEntry& data);
	void add_adc(GlobalAnalog_t const, const ADCEntry& data);

	std::map<halco::hicann::v2::Wafer, WaferEntry> mWaferData;
	std::map<std::string, DLSSetupEntry> mDLSData;
	std::map<size_t, HXCubeSetupEntry> mHXCubeData;
	std::map<size_t, JboaSetupEntry> mJboaData;

	static std::string const default_path;
#endif
};

} // namespace hwdb4cpp
