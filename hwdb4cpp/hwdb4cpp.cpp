#include "hwdb4cpp.h"

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/make_shared.hpp>
#include <log4cxx/logger.h>
#include <yaml-cpp/yaml.h>

#include "halco/common/iter_all.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("hwdb4cpp");
std::string const hwdb4cpp::database::default_path = "/wang/data/bss-hwdb/db.yaml";

using namespace halco::common;
using namespace halco::hicann::v2;
using namespace hwdb4cpp;

namespace {
// Map to translate back to enum, we compare agains the sting converted
// to lower chars
const std::vector<std::string> SetupType_NAMES = {
    {"vsetup"}, {"facetswafer"}, {"cubesetup"}, {"bsswafer"}};

// To store the entries in YAML easier, the map is converted into a list.
// These helper structs extend the entries with the corresponding key to
// simplify YAML (de)serialization
struct ADCYAML : public ADCEntry
{
	ADCYAML() : ADCEntry() {}
	ADCYAML(ADCEntry const& base) : ADCEntry(base) {}
	size_t analog = 0;
	size_t fpga = 0;
};

struct FPGAYAML : public FPGAEntry
{
	FPGAYAML() {}
	FPGAYAML(FPGAEntry const& base) : FPGAEntry(base) {}
	size_t coordinate;
};

struct ANANASYAML : public ANANASEntry
{
	ANANASYAML() {}
	ANANASYAML(ANANASEntry const& base) : ANANASEntry(base) {}
	size_t coordinate;
};

struct HICANNYAML : public HICANNEntry
{
	HICANNYAML() {}
	HICANNYAML(HICANNEntry const& base) : HICANNEntry(base) {}
	size_t coordinate;
};

} // anonymous namespace

// Converter for our YAML entries
// Trying to convert an not existent key in a mapping, will result in an
// InvalidNode exception, we catch this and return false, otherwise the
// location of the error is hard to find
namespace YAML {

namespace {
// Helper to retrive key from mapping and give a reasonable error
// message on failure
template <typename T>
T get_entry(const Node& node, const std::string name) try {
	const Node& key = node[name];
	if (!key.IsDefined()) {
		throw KeyNotFound(node.Mark(), name);
	} else {
		return key.as<T>();
	}
} catch (const YAML::Exception& err) {
	LOG4CXX_ERROR(
	    logger, "Error converting node YAML'" << node << "' to type '" << ZTL::typestring<T>()
	                                          << "': " << err.what());
	throw;
}

// Helper to retrive optional key with a default value
template <typename T>
T get_entry(const Node& node, const std::string name, const T default_value)
{
	const Node& key = node[name];
	if (!key.IsDefined()) {
		return default_value;
	} else {
		return key.as<T>();
	}
}

} // anonymous namespace

template <>
struct convert<SetupType>
{
	static Node encode(const SetupType& data)
	{
		Node node;
		node = SetupType_NAMES.at(static_cast<size_t>(data));
		return node;
	}

	static bool decode(const Node& node, SetupType& data)
	{
		std::string value = node.as<std::string>();
		boost::algorithm::to_lower(value);
		auto it = std::find(SetupType_NAMES.begin(), SetupType_NAMES.end(), value);
		if (it == SetupType_NAMES.end()) {
			throw std::runtime_error("Unkown value for chip: " + value);
		}
		data = static_cast<SetupType>(std::distance(SetupType_NAMES.begin(), it));
		return true;
	}
};

template <>
struct convert<IPv4>
{
	static Node encode(const IPv4& data)
	{
		Node node;
		node = data.to_string();
		return node;
	}

	static bool decode(const Node& node, IPv4& data)
	{
		std::string value = node.as<std::string>();
		data = IPv4::from_string(value);
		return true;
	}
};

template <>
struct convert<ADCYAML>
{
	static Node encode(const ADCYAML& data)
	{
		Node node;
		node["adc"] = data.coord;
		node["channel"] = data.channel.value();
		node["trigger"] = data.trigger.value();
		node["analog"] = data.analog;
		node["fpga"] = data.fpga;
		if (!data.remote_ip.is_unspecified()) {
			node["remote_ip"] = data.remote_ip;
			node["remote_port"] = data.remote_port.value();
		}
		return node;
	}

	static bool decode(const Node& node, ADCYAML& data)
	{
		if (!node.IsMap() || node.size() > 8) {
			LOG4CXX_ERROR(logger, "Decoding failed of: '''\n" << node << "'''")
			return false;
		}
		data.coord = get_entry<std::string>(node, "adc");
		data.channel = ChannelOnADC(get_entry<size_t>(node, "channel"));
		data.trigger = TriggerOnADC(get_entry<size_t>(node, "trigger"));
		data.analog = get_entry<size_t>(node, "analog");
		data.fpga = get_entry<size_t>(node, "fpga");
		data.remote_ip = get_entry<IPv4>(node, "remote_ip", IPv4());
		data.remote_port = TCPPort(get_entry<size_t>(node, "remote_port", 0));
		return true;
	}
};

template <>
struct convert<FPGAYAML>
{
	static Node encode(const FPGAYAML& data)
	{
		Node node;
		node["fpga"] = data.coordinate;
		node["ip"] = data.ip.to_string();
		if (data.highspeed == false) {
			node["highspeed"] = data.highspeed;
		}
		return node;
	}

	static bool decode(const Node& node, FPGAYAML& data)
	{
		if (!node.IsMap() || node.size() > 3) {
			LOG4CXX_ERROR(logger, "Decoding failed of: '''\n" << node << "'''")
			return false;
		}
		data.coordinate = get_entry<size_t>(node, "fpga");
		data.ip = IPv4::from_string(get_entry<std::string>(node, "ip"));
		data.highspeed = get_entry<bool>(node, "highspeed", true);
		return true;
	}
};

template <>
struct convert<ANANASYAML>
{
	static Node encode(const ANANASYAML& data)
	{
		Node node;
		node["ananas"] = data.coordinate;
		node["ip"] = data.ip.to_string();
		return node;
	}

	static bool decode(const Node& node, ANANASYAML& data)
	{
		if (!node.IsMap() || node.size() > 2) {
			LOG4CXX_ERROR(logger, "Decoding failed of: '''\n" << node << "'''")
			return false;
		}
		data.coordinate = get_entry<size_t>(node, "ananas");
		data.ip = IPv4::from_string(get_entry<std::string>(node, "ip"));
		return true;
	}
};

template <>
struct convert<HICANNYAML>
{
	static Node encode(const HICANNYAML& data)
	{
		Node node;
		node["hicann"] = data.coordinate;
		node["version"] = data.version;
		if (!data.label.empty()) {
			node["label"] = data.label;
		}
		return node;
	}

	static bool decode(const Node& node, HICANNYAML& data)
	{
		if (!node.IsMap() || node.size() > 3) {
			LOG4CXX_ERROR(logger, "Decoding failed of: '''\n" << node << "'''")
			return false;
		}
		data.coordinate = get_entry<size_t>(node, "hicann", 0);
		data.version = get_entry<size_t>(node, "version");
		data.label = get_entry<std::string>(node, "label", "");
		return true;
	}
};

} // namespace YAML


namespace hwdb4cpp {

void database::clear()
{
	mWaferData.clear();
	mDLSData.clear();
	mHXCubeData.clear();
}

void database::load(std::string const path)
{
	if (!(mWaferData.empty() && mDLSData.empty() && mHXCubeData.empty()))
		throw std::runtime_error("database has to be empty before loading new file");

	for (YAML::Node config : YAML::LoadAllFromFile(path)) {

		// yaml node is from a wafer
		if (config["wafer"].IsDefined()) {

			Wafer wafer(YAML::get_entry<size_t>(config, "wafer"));
			// FIXME: use encode/decode schema (see FPGAYAML below)
			WaferEntry entry;
			entry.setup_type = YAML::get_entry<SetupType>(config, "setuptype");
			if (entry.setup_type == SetupType::BSSWafer) {
				entry.macu = YAML::get_entry<IPv4>(config, "macu");
				entry.macu_version = YAML::get_entry<size_t>(config, "macuversion");
			} else {
				entry.macu = YAML::get_entry<IPv4>(config, "macu", IPv4());
				entry.macu_version = YAML::get_entry<size_t>(config, "macuversion", 0);
			}
			add_wafer_entry(wafer, entry);

			auto fpga_entries = config["fpgas"];
			if (fpga_entries.IsDefined()) {
				for (const auto& entry : fpga_entries.as<std::vector<FPGAYAML> >()) {
					FPGAGlobal fpga(FPGAGlobal(FPGAOnWafer(entry.coordinate), wafer));
					add_fpga_entry(fpga, entry);
				}
			}

			auto ananas_entries = config["ananas"];
			if (ananas_entries.IsDefined()) {
				for (const auto& entry : ananas_entries.as<std::vector<ANANASYAML> >()) {
					ANANASGlobal ananas(ANANASGlobal(ANANASOnWafer(entry.coordinate), wafer));
					add_ananas_entry(ananas, entry);
				}
			}

			auto adc_entries = config["adcs"];
			if (adc_entries.IsDefined()) {
				for (const auto& entry : adc_entries.as<std::vector<ADCYAML> >()) {
					FPGAGlobal fpga(FPGAOnWafer(entry.fpga), wafer);
					GlobalAnalog_t coord(fpga, AnalogOnHICANN(entry.analog));
					add_adc_entry(coord, entry);
				}
			}

			YAML::Node hicanns_node = config["hicanns"];
			if (!hicanns_node.IsDefined()) {
				// No HICANNs -> ignore
			} else if (hicanns_node.IsSequence()) {
				auto hicann_entries = hicanns_node.as<std::vector<HICANNYAML> >();
				for (const auto& entry : hicann_entries) {
					HICANNGlobal hicann(HICANNOnWafer(Enum(entry.coordinate)), wafer);
					add_hicann_entry(hicann, entry);
				}
			} else if (hicanns_node.IsMap()) {
				const HICANNYAML entry = hicanns_node.as<HICANNYAML>();
				for (auto hicann : iter_all<HICANNOnWafer>()) {
					if (has_fpga_entry(HICANNGlobal(hicann, wafer).toFPGAGlobal())) {
						HICANNGlobal hicann_global(hicann, wafer);
						add_hicann_entry(hicann_global, entry);
					}
				}
			} else {
				throw std::runtime_error("hicanns entry must be a squence or a map");
			}

		}

		// yaml node is from a dls setup
		else if (config["dls_setup"].IsDefined()) {

			auto dls_setup = config["dls_setup"].as<std::string>();
			DLSSetupEntry entry;
			add_dls_entry(dls_setup, entry);

			auto fpga_name_entry = config["fpga_name"];
			if (fpga_name_entry.IsDefined()) {
				mDLSData.at(dls_setup).fpga_name = fpga_name_entry.as<std::string>();
			}

			auto board_name_entry = config["board_name"];
			if (board_name_entry.IsDefined()) {
				mDLSData.at(dls_setup).board_name = board_name_entry.as<std::string>();
			}

			auto board_version_entry = config["board_version"];
			if (board_version_entry.IsDefined()) {
				mDLSData.at(dls_setup).board_version = board_version_entry.as<size_t>();
			}

			auto chip_id_entry = config["chip_id"];
			if (chip_id_entry.IsDefined()) {
				mDLSData.at(dls_setup).chip_id = chip_id_entry.as<size_t>();
			}

			auto chip_version_entry = config["chip_version"];
			if (chip_version_entry.IsDefined()) {
				mDLSData.at(dls_setup).chip_version = chip_version_entry.as<size_t>();
			}

			auto ntpwr_ip_entry = config["ntpwr_ip"];
			if (ntpwr_ip_entry.IsDefined()) {
				mDLSData.at(dls_setup).ntpwr_ip = ntpwr_ip_entry.as<std::string>();
			}

			auto ntpwr_slot_entry = config["ntpwr_slot"];
			if (ntpwr_slot_entry.IsDefined()) {
				mDLSData.at(dls_setup).ntpwr_slot = ntpwr_slot_entry.as<size_t>();
			}
		}
		// yaml node is from a HXCube setup
		else if (config["hxcube_id"].IsDefined()) {
			auto hxcube_id = config["hxcube_id"].as<size_t>();
			HXCubeSetupEntry entry;
			add_hxcube_entry(hxcube_id, entry);

			auto fpga_entries = config["fpga_ips"];
			if (fpga_entries.IsDefined()) {
				try {
					mHXCubeData.at(hxcube_id).fpga_ips = fpga_entries.as<std::array<IPv4, 2> >();
				} catch (const YAML::TypedBadConversion<std::array<IPv4, 2> >& e) {
					throw std::runtime_error(std::string("Two FPGA IP addresses must be defined:") + e.what());
				}
			}

			auto usb_host_entry = config["usb_host"];
			if (usb_host_entry.IsDefined()) {
				mHXCubeData.at(hxcube_id).usb_host = usb_host_entry.as<std::string>();
			}

			auto ldo_version_entry = config["ldo_version"];
			if (ldo_version_entry.IsDefined()) {
				mHXCubeData.at(hxcube_id).ldo_version = ldo_version_entry.as<size_t>();
			}

			auto usb_serial_entry = config["usb_serial"];
			if (usb_serial_entry.IsDefined()) {
				mHXCubeData.at(hxcube_id).usb_serial = usb_serial_entry.as<std::string>();
			}

			auto chip_serial_entry = config["chip_serial"];
			if (chip_serial_entry.IsDefined()) {
				mHXCubeData.at(hxcube_id).chip_serial = chip_serial_entry.as<size_t>();
			}
		}
		// yaml node does not contain wafer or dls setup or hxcube setup
		else {
			LOG4CXX_WARN(logger, "Found node entry neither from Wafer, DLS setup nor HX setup, ignore");
		}
	}
}

namespace {
/// Check that all HICANNs are in the vector and that they have the
/// same settings
bool can_merge_hicanns(const std::vector<HICANNYAML>& data)
{
	if (data.size() != HICANNOnWafer::enum_type::size) {
		return false;
	}
	size_t version = data[0].version;
	std::string label = data[0].label;
	for (auto& item : data) {
		if (item.version != version || item.label != label) {
			return false;
		}
	}
	return true;
}
}

/// Better error messages while decoding nodes are upstream, but not in the
/// currently released version: see https://github.com/jbeder/yaml-cpp/issues/200
void database::dump(std::ostream& out) const
{
	// Serialize entries entry-by-entry to maintain a nice order.
	// There is an unresolved issue about this
	// https://github.com/jbeder/yaml-cpp/issues/169
	// so this might getting nicer in future
	// First dump the wafer entries
	for (const auto& item : mWaferData) {
		const Wafer wafer = item.first;
		const WaferEntry& data = item.second;

		out << "---\n";

		{
			YAML::Node config;
			config["wafer"] = wafer.value();
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["setuptype"] = data.setup_type;
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["macu"] = data.macu;
			config["macuversion"] = data.macu_version;
			out << config << '\n';
		}

		if (!data.fpgas.empty()) {
			YAML::Node config;
			std::vector<FPGAYAML> fpga_data;
			for (auto& it : data.fpgas) {
				FPGAYAML entry(it.second);
				entry.coordinate = it.first;
				fpga_data.push_back(entry);
			}
			config["fpgas"] = fpga_data;
			out << config << '\n';
		}

		if (!data.ananas.empty()) {
			YAML::Node config;
			std::vector<ANANASYAML> ananas_data;
			for (auto& it : data.ananas) {
				ANANASYAML entry(it.second);
				entry.coordinate = it.first;
				ananas_data.push_back(entry);
			}
			config["ananas"] = ananas_data;
			out << config << '\n';
		}

		if (!data.adcs.empty()) {
			YAML::Node config;
			std::vector<ADCYAML> adc_data;
			for (auto& it : data.adcs) {
				ADCYAML entry(it.second);
				entry.fpga = it.first.first;
				entry.analog = it.first.second;
				adc_data.push_back(entry);
			}
			config["adcs"] = adc_data;
			out << config << '\n';
		}

		if (!data.hicanns.empty()) {
			YAML::Node config;
			std::vector<HICANNYAML> hicann_data;
			for (auto it : data.hicanns) {
				HICANNYAML entry(it.second);
				entry.coordinate = it.first.toHICANNOnWafer().toEnum();
				hicann_data.push_back(entry);
			}

			/// Check if we can merge all HICANNs
			if (can_merge_hicanns(hicann_data)) {
				YAML::Node hicanns;
				hicanns["version"] = hicann_data[0].version;
				if (!hicann_data[0].label.empty()) {
					hicanns["label"] = hicann_data[0].label;
				}
				config["hicanns"] = hicanns;
			} else {
				config["hicanns"] = hicann_data;
			}
			out << config << '\n';
		}
	}

	// Also dump the dls setups
	for (const auto& item : mDLSData) {
		const std::string dls_entry = item.first;
		const DLSSetupEntry& data = item.second;

		out << "---\n";

		{
			YAML::Node config;
			config["dls_setup"] = dls_entry;
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["fpga_name"] = data.fpga_name;
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["board_name"] = data.board_name;
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["board_version"] = data.board_version;
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["chip_id"] = data.chip_id;
			out << config << '\n';
		}

		{
			YAML::Node config;
			config["chip_version"] = data.chip_version;
			out << config << '\n';
		}

		if (data.ntpwr_ip != " ") {
			YAML::Node config;
			config["ntpwr_ip"] = data.ntpwr_ip;
			out << config << '\n';
		}

		if (data.ntpwr_slot != 0) {
			YAML::Node config;
			config["ntpwr_slot"] = data.ntpwr_slot;
			out << config << '\n';
		}

	}

	for (const auto& item : mHXCubeData) {
		const size_t hxcube_id = item.first;
		const HXCubeSetupEntry& data = item.second;

		out << "---\n";

		{
			YAML::Node config;
			config["hxcube_id"] = hxcube_id;
			out << config << '\n';
		}

		if (!data.fpga_ips.empty()) {
			YAML::Node config;
			config["fpga_ips"] = data.fpga_ips;
			out << config << '\n';
		}

		if (data.usb_host != "") {
			YAML::Node config;
			config["usb_host"] = data.usb_host;
			out << config << '\n';
		}

		if (data.ldo_version != 0) {
			YAML::Node config;
			config["ldo_version"] = data.ldo_version;
			out << config << '\n';
		}

		if (data.usb_serial != "") {
			YAML::Node config;
			config["usb_serial"] = data.usb_serial;
			out << config << '\n';
		}

		if (data.chip_serial != 0) {
			YAML::Node config;
			config["chip_serial"] = data.chip_serial;
			out << config << '\n';
		}
	}
}

void database::add_wafer_entry(Wafer const wafer, WaferEntry const entry) {
	mWaferData[wafer] = entry;
}

bool database::remove_wafer_entry(Wafer const wafer) {
	return mWaferData.erase(wafer);
}

bool database::has_wafer_entry(Wafer const wafer) const {
	return mWaferData.count(wafer);
}

WaferEntry& database::get_wafer_entry(Wafer const wafer) {
	return mWaferData.at(wafer);
}

WaferEntry const& database::get_wafer_entry(Wafer const wafer) const {
	return mWaferData.at(wafer);
}

std::vector<halco::hicann::v2::Wafer> database::get_wafer_coordinates() const
{
	std::vector<halco::hicann::v2::Wafer> ret;
	for (auto it : mWaferData) {
		ret.push_back(it.first);
	}
	return ret;
}

void database::add_fpga_entry(FPGAGlobal const fpga, FPGAEntry const entry) {
	mWaferData.at(fpga.toWafer()).fpgas[fpga] = entry;
}

bool database::remove_fpga_entry(FPGAGlobal const fpga) {
	bool ok = mWaferData.at(fpga.toWafer()).fpgas.erase(fpga);
	if (ok) {
		for (auto hicann : fpga.toHICANNGlobal()) {
			remove_hicann_entry(hicann);
		}
	}
	return ok;
}

bool database::has_fpga_entry(FPGAGlobal const fpga) const {
	if (has_wafer_entry(fpga.toWafer())) {
		return mWaferData.at(fpga.toWafer()).fpgas.count(fpga);
	}
	return false;
}

FPGAEntry const& database::get_fpga_entry(FPGAGlobal const fpga) const {
	return mWaferData.at(fpga.toWafer()).fpgas.at(fpga);
}

FPGAEntryMap database::get_fpga_entries(Wafer const wafer) const {
	return mWaferData.at(wafer).fpgas;
}

void database::add_ananas_entry(ANANASGlobal const ananas, ANANASEntry const entry) {
	mWaferData.at(ananas.toWafer()).ananas[ananas] = entry;
}

bool database::remove_ananas_entry(ANANASGlobal const ananas) {
	return mWaferData.at(ananas.toWafer()).ananas.erase(ananas);
}

bool database::has_ananas_entry(ANANASGlobal const ananas) const {
	if (has_wafer_entry(ananas.toWafer())) {
		return mWaferData.at(ananas.toWafer()).ananas.count(ananas);
	}
	return false;
}

ANANASEntry const& database::get_ananas_entry(ANANASGlobal const ananas) const {
	return mWaferData.at(ananas.toWafer()).ananas.at(ananas);
}

ANANASEntryMap database::get_ananas_entries(Wafer const wafer) const {
	return mWaferData.at(wafer).ananas;
}

void database::add_hicann_entry(HICANNGlobal const hicann, HICANNEntry const entry) {
	WaferEntry& wafer = mWaferData.at(hicann.toWafer());
	wafer.fpgas.at(hicann.toFPGAGlobal());
	wafer.hicanns[hicann] = entry;
}

bool database::remove_hicann_entry(HICANNGlobal const hicann) {
	return mWaferData.at(hicann.toWafer()).hicanns.erase(hicann);
}

bool database::has_hicann_entry(HICANNGlobal const hicann) const {
	if (has_wafer_entry(hicann.toWafer())) {
		return mWaferData.at(hicann.toWafer()).hicanns.count(hicann);
	}
	return false;
}

HICANNEntry const& database::get_hicann_entry(HICANNGlobal const hicann) const {
	return mWaferData.at(hicann.toWafer()).hicanns.at(hicann);
}

HICANNEntryMap database::get_hicann_entries(Wafer const wafer) const {
	return mWaferData.at(wafer).hicanns;
}

HICANNEntryMap database::get_hicann_entries(FPGAGlobal const fpga) const {
	HICANNEntryMap ret_map;
	for (auto hicann : iter_all<HICANNOnDNC>()) {
		//FIXME replace dnc coordinate with reticle, will make this much less ugly
		auto dnconwafer = gridLookupDNCGlobal(FPGAGlobal(fpga), DNCOnFPGA(Enum(0))).toDNCOnWafer();
		auto hicannglobal = HICANNGlobal(hicann.toHICANNOnWafer(dnconwafer), Wafer(fpga.toWafer()));
		if (has_hicann_entry(hicannglobal)) {
			ret_map[hicannglobal] = mWaferData.at(hicannglobal.toWafer()).hicanns.at(hicannglobal);
		}
	}
	return ret_map;
}

void database::add_adc_entry(GlobalAnalog_t const analog, ADCEntry const entry) {
	mWaferData.at(analog.first.toWafer()).adcs[analog] = entry;
}

bool database::remove_adc_entry(GlobalAnalog_t const analog) {
	return mWaferData.at(analog.first.toWafer()).adcs.erase(analog);
}

bool database::has_adc_entry(GlobalAnalog_t const analog) const {
	return mWaferData.at(analog.first.toWafer()).adcs.count(analog);
}

ADCEntry const& database::get_adc_entry(GlobalAnalog_t const analog) const {
	return mWaferData.at(analog.first.toWafer()).adcs.at(analog);
}

ADCEntryMap database::get_adc_entries(Wafer const wafer) const {
	return mWaferData.at(wafer).adcs;
}

ADCEntryMap database::get_adc_entries(FPGAGlobal const fpga) const {
	ADCEntryMap ret_map;
	for (auto analog : iter_all<AnalogOnHICANN>()) {
		if (mWaferData.at(fpga.toWafer()).adcs.count(GlobalAnalog_t(fpga, analog)) > 0) {
			ret_map[GlobalAnalog_t(fpga, analog)] = mWaferData.at(fpga.toWafer()).adcs.at(GlobalAnalog_t(fpga, analog));
		}
	}
	return ret_map;
}

void database::add_dls_entry(std::string const dls_setup, DLSSetupEntry const entry) {
	mDLSData[dls_setup] = entry;
}

bool database::remove_dls_entry(std::string const dls_setup) {
	return mDLSData.erase(dls_setup);
}

bool database::has_dls_entry(std::string const dls_setup) const {
	return mDLSData.count(dls_setup);
}

DLSSetupEntry& database::get_dls_entry(std::string const dls_setup) {
	return mDLSData.at(dls_setup);
}

DLSSetupEntry const& database::get_dls_entry(std::string const dls_setup) const {
	return mDLSData.at(dls_setup);
}

std::vector<std::string> database::get_dls_setup_ids() const
{
	std::vector<std::string> ret;
	for (auto it : mDLSData) {
		ret.push_back(it.first);
	}
	return ret;
}

void database::add_hxcube_entry(size_t const hxcube_id, HXCubeSetupEntry const entry) {
	mHXCubeData[hxcube_id] = entry;
}

bool database::remove_hxcube_entry(size_t const hxcube_id) {
	return mHXCubeData.erase(hxcube_id);
}

bool database::has_hxcube_entry(size_t const hxcube_id) const {
	return mHXCubeData.count(hxcube_id);
}

HXCubeSetupEntry& database::get_hxcube_entry(size_t const hxcube_id) {
	return mHXCubeData.at(hxcube_id);
}

HXCubeSetupEntry const& database::get_hxcube_entry(size_t const hxcube_id) const {
	return mHXCubeData.at(hxcube_id);
}

std::vector<size_t> database::get_hxcube_ids() const {
	std::vector<size_t> ret;
	for (auto it : mHXCubeData) {
		ret.push_back(it.first);
	}
	return ret;
}

std::string const& database::get_default_path()
{
	return default_path;
}

} // namespace hwdb4cpp

