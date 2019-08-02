#!/usr/bin/env python

from pywrap.wrapper import Wrapper
from pywrap import containers, namespaces, matchers, classes
from pyplusplus.module_builder import call_policies

wrap = Wrapper()
mb = wrap.mb

# Special fix up
containers.extend_std_containers(mb)
namespaces.include_default_copy_constructors(mb)

ns_hwdb4cpp = mb.namespace('hwdb4cpp')
ns_hwdb4cpp.include()
namespaces.extend_array_operators(ns_hwdb4cpp)

for c in ns_hwdb4cpp.classes(allow_empty=True):
    c.include()
    if c.name.startswith('database'):
        for f in c.mem_funs('get_wafer_entry', allow_empty=True):
            f.call_policies = call_policies.return_internal_reference()
        for f in c.mem_funs('get_dls_entry', allow_empty=True):
            f.call_policies = call_policies.return_internal_reference()
        for f in c.mem_funs('get_hxcube_entry', allow_empty=True):
            f.call_policies = call_policies.return_internal_reference()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'protected')

wrap.finish()
