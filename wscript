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
        source=['hello.c', 'init.c'])
    
    # Copy to an absolute path after the program is built
    bld(rule='cp ${SRC} ${TGT}',
        source='hello.exe',
        target='/home/utndev/rtems-out/hello.elf')
