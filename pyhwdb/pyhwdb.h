#pragma once
#include "hwdb4cpp/genpybind.h"

GENPYBIND_TAG_HWDB
GENPYBIND_MANUAL({
	parent.attr("__variant__") = "pybind11";
	parent->py::module::import("pyhalco_common");
})

#include "hwdb4cpp/hwdb4cpp.h"
