# Networking Setup Guide for STM32H743ZI Nucleo with RTEMS 6

## Current Issue: libbsd Error 13

The application is failing to initialize libbsd with error code 13 (RTEMS_INVALID_CLOCK). This indicates the RTEMS kernel or BSP is not properly configured for libbsd support.

## Root Causes

1. **RTEMS Kernel Built Without POSIX Support**
   - libbsd requires POSIX API support
   - Check if kernel was built with `--enable-posix`

2. **RTEMS Kernel Built With Old Networking**
   - RTEMS cannot have both old networking AND libbsd enabled
   - If kernel has `--enable-networking`, rebuild with `--disable-networking`

3. **BSP Not Configured for Ethernet**
   - The nucleo-h743zi BSP may not have the Ethernet driver enabled
   - Check BSP device tree or startup configuration

4. **Missing libbsd Patches**
   - Some BSPs require specific patches for libbsd support
   - Check RTEMS source for arm/nucleo-h743zi network configuration

## How to Check Your RTEMS Kernel Configuration

```bash
# Check RTEMS install info
cd /path/to/rtems-6.1
cat cpukit/build-variant

# Look for RTEMS version info
arm-rtems6-gcc --version
```

## Rebuilding RTEMS Kernel with Proper Configuration

### Prerequisites
- RTEMS source code
- arm-rtems6 toolchain installed

### Build Steps for RTEMS 6.1

```bash
# 1. Download/extract RTEMS source
cd ~/rtems
wget https://ftp.rtems.org/pub/rtems/releases/6.1/rtems-6.1.tar.xz
tar xf rtems-6.1.tar.xz

# 2. Configure for nucleo-h743zi with libbsd support
cd ~/rtems-6.1
./source-builder/sb-set-builder \
  --prefix=/opt/rtems/6.1 \
  --target=arm-rtems6 \
  arm/nucleo-h743zi

# 3. Use proper enable/disable flags
./configure \
  --prefix=/opt/rtems/6.1 \
  --target=arm-rtems6 \
  --enable-posix \           # REQUIRED for libbsd
  --disable-networking \     # REQUIRED (use libbsd instead)
  --enable-rtemsbsp=arm/nucleo-h743zi

# 4. Build and install
make -j4 all install
```

### Verify Installation

```bash
# Check if kernel has POSIX support
arm-rtems6-objdump -s /opt/rtems/6.1/arm-rtems6/nucleo-h743zi/lib/start.o | grep -i posix

# Test your application with new kernel
./waf distclean
./waf configure --rtems=/opt/rtems/6.1 --rtems-bsp=arm/nucleo-h743zi
./waf
```

## Alternative: Using Static IP Without ifconfig

If libbsd won't initialize, you can still use UDP by:

1. Manually configuring the network interface at the driver level
2. Using raw socket operations without the full BSD stack

See `hello.c` alternative implementations for fallback approaches.

## Network Interface Name for STM32H743ZI

- **Correct interface**: `stm0` (STMicroelectronics MAC driver)
- **NOT** `eth0` or `st0` or other variations

## Testing the Fix

Once kernel is rebuilt:

```bash
# Flash the new hello.elf to board
./waf
cp build/arm-rtems6-nucleo-h743zi/hello.exe /out/hello.elf

# Monitor serial output - should show:
# "Network stack initialized successfully"
# "Configuring network interface stm0..."
# "UDP ready to 192.168.0.104:5000"
```

## Useful RTEMS Documentation

- [RTEMS LibBSD Manual](https://docs.rtems.org/doc-current/c-user/networking.html)
- [RTEMS Networking Architecture](https://docs.rtems.org/doc-current/c-user/bsd_networking.html)
- [STM32H7 Ethernet Initialization](https://docs.rtems.org/doc-current/arm/nucleo-h743zi.html)

## Contact

For RTEMS-specific help:
- RTEMS Community: https://lists.rtems.org
- RTEMS Documentation: https://docs.rtems.org
