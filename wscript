#!/usr/bin/env python
from waflib.extras.gtest import summary

APPNAME='hwdb'

def depends(ctx):
    ctx('halco')
    ctx('logger')
    ctx('pywrap')

def options(opt):
    opt.load('compiler_cxx')
    opt.load('gtest')

def configure(cfg):
    cfg.load('compiler_cxx')

    cfg.check_cfg(package='yaml-cpp',
                  args=['yaml-cpp >= 0.5.3', '--cflags', '--libs'],
                  uselib_store='YAMLCPP')

    cfg.check_cxx(uselib_store='GTEST4HWDB',
                   mandatory=True,
                   header_name='gtest/gtest.h',
                   lib='gtest'
    )

def build(bld):
    bld.add_post_fun(summary)

    bld(
        target          = 'hwdb4cpp_inc',
        use             = 'halco_hicann_v2_inc',
        export_includes = '.',
    )

    bld.shlib(
        target          = 'hwdb4cpp',
        features        = 'cxx',
        source          = 'hwdb4cpp/hwdb4cpp.cpp',
        use             = 'halco_hicann_v2 hwdb4cpp_inc logger_obj YAMLCPP',
        install_path    = '${PREFIX}/lib',
    )

    bld(
        target          = 'hwdb4c',
        features        = 'cxx cxxshlib',
        source          = 'hwdb4cpp/hwdb4c.cpp',
        use             = 'hwdb4cpp',
        install_path    = '${PREFIX}/lib',
        cxxflags        = '-fvisibility=hidden',
    )

    bld.program(
        target = 'hwdb_tests',
        source = [
            'test/test_hwdb4c.cpp',
        ],
        features = 'cxx gtest',
        use = [ 'GTEST4HWDB', 'hwdb4c' ],
    )

    bld(
        target         = 'pyhwdb',
        features       = 'cxx cxxshlib pypp pyembed pyext',
        script         = 'pyhwdb/generate.py',
        gen_defines    = 'PYPLUSPLUS __STRICT_ANSI__',
        defines        = 'PYBINDINGS',
        headers        = 'pyhwdb/pyhwdb.h',
        use            = ['hwdb4cpp', 'pyhalco_hicann_v2', 'pywrap'],
        install_path   = '${PREFIX}/lib',
        linkflags      = '-Wl,-z,defs',
    )

    bld(
        name            = "pyhwdb_tests",
        tests           = 'pyhwdb/test/pyhwdb_test.py',
        features        = 'use pytest',
        use             = 'pyhwdb',
        install_path    = '${PREFIX}/bin',
        linkflags      = '-Wl,-z,defs',
        test_timeout    = 45,
        pythonpath      = ["test"],
    )
