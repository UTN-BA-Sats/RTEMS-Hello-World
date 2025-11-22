from __future__ import print_function

rtems_version = "6"

try:
    import rtems_waf.rtems as rtems
except:
    print('error: no rtems_waf git submodule')
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)

def bsp_configure(conf, arch_bsp):
    # Add any custom BSP configuration here
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)
    
    # Define the application
    bld(features='c cprogram',
        target='hello.exe',
        source='hello.c',
        includes='.',
        # Link against libbsd and its dependencies
        lib=['bsd', 'm'],
        # Add library search paths
        libpath=[bld.env.PREFIX + '/arm-rtems6/nucleo-h743zi/lib'],
        # Add include paths
        cflags=['-g', '-O2'],
        use=['RTEMS']
    )

    # Copy to an absolute path after the program is built
    bld(rule='cp ${SRC} ${TGT}',
        source='hello.exe',
        target='/out/hello.elf')