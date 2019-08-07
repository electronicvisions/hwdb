#!/usr/bin/env python
from waflib.extras.gtest import summary
from waflib import Utils

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
    cfg.load('gtest')
    cfg.load('python')
    cfg.check_python_version()
    cfg.check_python_headers()
    cfg.load('genpybind')

    cfg.check_cfg(package='yaml-cpp',
                  args=['yaml-cpp >= 0.5.3', '--cflags', '--libs'],
                  uselib_store='YAMLCPP')

    if cfg.env.with_pybind:
        cfg.check(
            compiler='cxx',
            features='cxx pyembed',
            uselib_store='PYBIND11HWDB',
            mandatory=True,
            header_name='pybind11/pybind11.h',
        )

def build(bld):
    bld.add_post_fun(summary)

    bld(
        target          = 'hwdb4cpp_inc',
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
        source = bld.path.ant_glob('test/test_*.cpp'),
        features = 'cxx gtest',
        test_main = 'test/test-main.cpp',
        use = [ 'GTEST', 'hwdb4c' ],
        install_path = '${PREFIX}/bin',
        linkflags = ['-lboost_program_options-mt'],
    )

    if bld.env.build_python_bindings:
        assert not bld.env.with_pybind
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

    if bld.env.with_pybind:
        assert not bld.env.build_python_bindings
        bld(
            target         = 'pyhwdb',
            features       = 'genpybind cxx cxxshlib pyembed pyext',
            source         = 'pyhwdb/pyhwdb.h',
            genpybind_tags = 'hwdb',
            use            = ['hwdb4cpp', 'pyhalco_common'],
            install_path   = '${PREFIX}/lib',
            linkflags      = '-Wl,-z,defs',
        )

    if bld.env.build_python_bindings or bld.env.with_pybind:
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

    if bld.env.build_python_bindings:
        bld(
            target          = 'pyhwdb_bss1_tools',
            features        = 'use py',
            use             = 'pyhwdb',
            source          = bld.path.ant_glob('pyhwdb/tools/bss1/*.py'),
            install_path    = '${PREFIX}/bin',
            relative_trick  = False,
            chmod           = Utils.O755
        )

        tools_tests_path='pyhwdb/test/bss1/pyhwdb_tools_test.py'

        bld(
            name="pyhwdb_bss1_tools_tests",
            tests=tools_tests_path,
            source=tools_tests_path,
            features='use pytest py',
            use='pyhwdb_bss1_tools',
            install_path='${PREFIX}/bin',
            test_timeout=15,
            relative_trick=False,
            chmod=Utils.O755
        )
