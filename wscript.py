# Hello world Waf script

from __future__ import print_function

rtems_version = "6"

try:
    import rtems_waf.rtems as rtems
except ImportError:
    print('error: no rtems_waf git submodule')
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version = rtems_version, long_commands = True)

def bsp_configure(conf, arch_bsp):
    # Add BSP specific configuration checks
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)
    bld(features='c cprogram',
        target='hello.exe',
        cflags='-g -O2',
        source=['hello.c', 'init.c'], 
        # The standard --whole-archive or --undefined flags cause linker
        # conflicts and executable size overflows because libbsd.a contains
        # multiple conflicting implementations (e.g. for crypto, other drivers).
        #
        # The definitive solution is to explicitly link ONLY the required driver
        # object file from within the archive. The path is constructed from the
        # BSP's libdir variable. This provides the 'if_stmac_attach' symbol
        # without pulling in any other conflicting objects from the archive.
        linkflags=[bld.env.LIBDIR[0] + '/libbsd.a(if_stmac.c.o)', '-lbsd'],
        lib=['c', 'm'])
    
    # Copy to an absolute path after the program is built
    bld(rule='cp ${SRC} ${TGT}',
        source='hello.exe',
        target='/out/hello.elf')
