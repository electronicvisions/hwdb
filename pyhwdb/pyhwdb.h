#pragma once
#include "hwdb4cpp/genpybind.h"

GENPYBIND_TAG_HWDB
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_common");
})

#include "hwdb4cpp/hwdb4cpp.h"
#if defined(__GENPYBIND__) or defined(__GENPYBIND_GENERATED__)
#include "cereal/types/hwdb/entries.h"
#include <cereal/archives/portable_binary.hpp>
#include <pybind11/pybind11.h>

namespace {
// FIXME: copy-and-pasted from haldls!
template <typename T>
static void apply_pickle(pybind11::module& parent, std::string const& name)
{
	auto cls = parent.attr(name.c_str());
	auto ism = pybind11::is_method(cls);
	auto custom_getstate = [](pybind11::object const& self) {
		auto thing = self.cast<T>();
		std::stringstream ss;
		{
			cereal::PortableBinaryOutputArchive ar(ss);
			ar(thing);
		}
		std::string serialized{ss.str()};
		if (pybind11::hasattr(self, "__dict__")) {
			return pybind11::make_tuple(pybind11::bytes(serialized), self.attr("__dict__"));
		} else {
			return pybind11::make_tuple(pybind11::bytes(serialized));
		}
	};

	auto custom_setstate = [](pybind11::detail::value_and_holder& v_h,
	                          pybind11::tuple const& data) {
		std::unique_ptr<T> thing = std::make_unique<T>();

		{
			// restore serialized C++ state
			std::stringstream ss(data[0].cast<std::string>());
			cereal::PortableBinaryInputArchive ar(ss);
			ar(*thing);
		}

		using Base = pybind11::class_<T>;
		// use new-style pickle implementation (detail namespace :/)
		if (data.size() == 1) {
			pybind11::detail::initimpl::setstate<Base>(
			    v_h, std::move(thing), Py_TYPE(v_h.inst) != v_h.type->type);
		} else if (data.size() == 2) {
			auto state = std::make_pair(std::move(thing), data[1].cast<pybind11::dict>());
			pybind11::detail::initimpl::setstate<Base>(
			    v_h, std::move(state), Py_TYPE(v_h.inst) != v_h.type->type);
		} else {
			std::stringstream ss;
			ss << "Wrong state tuple size: " << data.size();
			throw std::runtime_error(ss.str());
		}
	};


	cls.attr("__getstate__") = pybind11::cpp_function(custom_getstate, ism);
	cls.attr("__setstate__") = pybind11::cpp_function(
	    custom_setstate, pybind11::detail::is_new_style_constructor(),
	    pybind11::name("__setstate__"), ism, pybind11::arg("state"), pybind11::pos_only());
}
}

GENPYBIND_MANUAL({
	apply_pickle<hwdb4cpp::HXCubeWingEntry>(parent, "HXCubeWingEntry");
	apply_pickle<hwdb4cpp::HXCubeFPGAEntry>(parent, "HXCubeFPGAEntry");
	apply_pickle<hwdb4cpp::HXCubeSetupEntry>(parent, "HXCubeSetupEntry");
	apply_pickle<hwdb4cpp::JboaSetupEntry>(parent, "JboaSetupEntry");
})
#endif
