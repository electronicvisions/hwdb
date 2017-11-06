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
	uint8_t analog = 0;
	size_t fpga = 0;
};

struct FPGAYAML : public FPGAEntry
{
	FPGAYAML() {}
	FPGAYAML(FPGAEntry const& base) : FPGAEntry(base) {}
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
	mData.clear();
}

void database::load(std::string const path)
{
	if (!mData.empty())
		throw std::runtime_error("database has to be empty before loading new file");

	for (YAML::Node config : YAML::LoadAllFromFile(path)) {
		Wafer wafer(YAML::get_entry<size_t>(config, "wafer"));
		// FIXME: use encode/decode schema (see FPGAYAML below)
		WaferEntry entry;
		entry.setup_type = YAML::get_entry<SetupType>(config, "setuptype");
		entry.macu = YAML::get_entry<IPv4>(config, "macu", IPv4());
		add_wafer_entry(wafer, entry);

		auto fpga_entries = config["fpgas"];
		if (fpga_entries.IsDefined()) {
			for (const auto& entry : fpga_entries.as<std::vector<FPGAYAML> >()) {
				FPGAGlobal fpga(FPGAGlobal(FPGAOnWafer(entry.coordinate), wafer));
				add_fpga_entry(fpga, entry);
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
	for (const auto& item : mData) {
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
				entry.coordinate = it.first.toEnum();
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
}

void database::add_wafer_entry(Wafer const wafer, WaferEntry const entry) {
	mData[wafer] = entry;
}

bool database::remove_wafer_entry(Wafer const wafer) {
	return mData.erase(wafer);
}

bool database::has_wafer_entry(Wafer const wafer) const {
	return mData.count(wafer);
}

WaferEntry& database::get_wafer_entry(Wafer const wafer) {
	return mData.at(wafer);
}
WaferEntry const& database::get_wafer_entry(Wafer const wafer) const {
	return mData.at(wafer);
}

void database::add_fpga_entry(FPGAGlobal const fpga, FPGAEntry const entry) {
	mData.at(fpga.toWafer()).fpgas[fpga] = entry;
}

bool database::remove_fpga_entry(FPGAGlobal const fpga) {
	return mData.at(fpga.toWafer()).fpgas.erase(fpga);
}

bool database::has_fpga_entry(FPGAGlobal const fpga) const {
	return mData.at(fpga.toWafer()).fpgas.count(fpga);
}

FPGAEntry const& database::get_fpga_entry(FPGAGlobal const fpga) const {
	return mData.at(fpga.toWafer()).fpgas.at(fpga);
}

std::map<FPGAGlobal, FPGAEntry>
database::get_fpga_entries(Wafer const wafer) const {
	return mData.at(wafer).fpgas;
}

void database::add_hicann_entry(HICANNGlobal const hicann, HICANNEntry const entry) {
	mData.at(hicann.toWafer()).hicanns[hicann] = entry;
}

bool database::remove_hicann_entry(HICANNGlobal const hicann) {
	return mData.at(hicann.toWafer()).hicanns.erase(hicann);
}

bool database::has_hicann_entry(HICANNGlobal const hicann) const {
	return mData.at(hicann.toWafer()).hicanns.count(hicann);
}

HICANNEntry const& database::get_hicann_entry(HICANNGlobal const hicann) const {
	return mData.at(hicann.toWafer()).hicanns.at(hicann);
}

std::map<HICANNGlobal, HICANNEntry>
database::get_hicann_entries(Wafer const wafer) const {
	return mData.at(wafer).hicanns;
}

std::map<HICANNGlobal, HICANNEntry>
database::get_hicann_entries(FPGAGlobal const fpga) const {
	std::map<HICANNGlobal, HICANNEntry> ret_map;
	for (auto hicann : iter_all<HICANNOnDNC>()) {
		//FIXME replace dnc coordinate with reticle, will make this much less ugly
		auto dnconwafer = gridLookupDNCGlobal(FPGAGlobal(fpga), DNCOnFPGA(Enum(0))).toDNCOnWafer();
		auto hicannglobal = HICANNGlobal(hicann.toHICANNOnWafer(dnconwafer), Wafer(fpga.toWafer()));
		if (has_hicann_entry(hicannglobal)) {
			ret_map[hicannglobal] = mData.at(hicannglobal.toWafer()).hicanns.at(hicannglobal);
		}
	}
	return ret_map;
}

void database::add_adc_entry(GlobalAnalog_t const analog, ADCEntry const entry) {
	mData.at(analog.first.toWafer()).adcs[analog] = entry;
}

bool database::remove_adc_entry(GlobalAnalog_t const analog) {
	return mData.at(analog.first.toWafer()).adcs.erase(analog);
}

bool database::has_adc_entry(GlobalAnalog_t const analog) const {
	return mData.at(analog.first.toWafer()).adcs.count(analog);
}

ADCEntry const& database::get_adc_entry(GlobalAnalog_t const analog) const {
	return mData.at(analog.first.toWafer()).adcs.at(analog);
}

std::map<GlobalAnalog_t, ADCEntry>
database::get_adc_entries(Wafer const wafer) const {
	return mData.at(wafer).adcs;
}

std::map<GlobalAnalog_t, ADCEntry>
database::get_adc_entries(FPGAGlobal const fpga) const {
	std::map<GlobalAnalog_t, ADCEntry> ret_map;
	for (auto analog : iter_all<AnalogOnHICANN>()) {
		if (mData.at(fpga.toWafer()).adcs.count(GlobalAnalog_t(fpga, analog)) > 0) {
			ret_map[GlobalAnalog_t(fpga, analog)] = mData.at(fpga.toWafer()).adcs.at(GlobalAnalog_t(fpga, analog));
		}
	}
	return ret_map;
}


std::string const& database::get_default_path()
{
	return default_path;
}

} // namespace hwdb4cpp

