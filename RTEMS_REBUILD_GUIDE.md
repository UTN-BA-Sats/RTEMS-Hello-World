# RTEMS 6.1 Rebuild Guide for nucleo-h743zi with libbsd Support

## Problem
Error 13 (RTEMS_INVALID_CLOCK) from `rtems_bsd_initialize()` indicates the BSP wasn't built with proper libbsd/network support.

## Root Cause Analysis
Your Docker build uses `bspdefaults` which may not include all necessary libbsd configuration options. The BSP needs to be built with specific flags to enable:
- libbsd support
- Network driver initialization
- Proper memory allocation for network stack

## Solution: Rebuild RTEMS with Explicit libbsd Support

### Step 1: Generate BSP Defaults with Network Support
```bash
cd /path/to/rtems-6.1
./waf bspdefaults --rtems-bsps=arm/nucleo-h743zi > nucleo-h743zi.ini
```

### Step 2: Edit the .ini File
Add or enable these options in `nucleo-h743zi.ini`:

```ini
# Enable libbsd support
--enable-libbsd

# Network driver support
--enable-network-driver=stmac

# Memory allocation for network
--rtems-bsd-mbufs=2000
--rtems-bsd-clusters=2000
```

### Step 3: Configure with Explicit Options
```bash
./waf configure \
  --prefix=/opt/rtems/6.1 \
  --rtems-bsps=arm/nucleo-h743zi \
  --rtems-config=$(pwd)/nucleo-h743zi.ini \
  --rtems-tools=/opt/rtems/6.1 \
  --enable-posix \
  --disable-networking \
  --enable-libbsd
```

### Step 4: Build and Install
```bash
./waf -j$(nproc)
./waf install
```

## Alternative: Check Existing BSP Options

If rebuilding is too complex, first check what options your current BSP supports:

```bash
# List available BSP options
./waf list-bsps --rtems-bsps=arm/nucleo-h743zi

# Check if libbsd is mentioned
nm /opt/rtems/6.1/arm-rtems6/nucleo-h743zi/lib/librtemsbsp.a | grep -i "bsd\|network" | head -10

# Verify libbsd is linked properly
ldd /opt/rtems/6.1/arm-rtems6/nucleo-h743zi/lib/libbsd.a 2>/dev/null || \
  ar t /opt/rtems/6.1/arm-rtems6/nucleo-h743zi/lib/libbsd.a | head -10
```

## Docker Build Template (Fixed Version)

```dockerfile
# Build with proper libbsd support
RUN cd ${RTEMS_SRC} && \
    ./waf bspdefaults --rtems-bsps=${BSP} > ${BSP##*/}.ini

# Add these lines to the .ini file
RUN sed -i '/\[${BSP##*/}\]/a --enable-posix\n--disable-networking\n--enable-libbsd' \
    ${RTEMS_SRC}/${BSP##*/}.ini

RUN cd ${RTEMS_SRC} && \
    ./waf configure \
      --prefix=${RTEMS_PREFIX} \
      --rtems-bsps=${BSP} \
      --rtems-config=$(pwd)/${BSP##*/}.ini \
      --rtems-tools=${RTEMS_PREFIX} \
      --enable-posix \
      --disable-networking \
      --enable-libbsd \
  && ./waf -j"$(nproc)" \
  && ./waf install
```

## Quick Diagnostic

If you can't rebuild, check if the current libbsd is actually functional:

```bash
# Check if stmac driver is available
strings /opt/rtems/6.1/arm-rtems6/nucleo-h743zi/lib/libbsd.a | grep -i stmac

# Check for libbsd symbols
nm /opt/rtems/6.1/arm-rtems6/nucleo-h743zi/lib/libbsd.a | grep rtems_bsd | head -20
```

## Expected Result After Fix

When properly configured, `rtems_bsd_initialize()` should return `RTEMS_SUCCESSFUL` and you should see:
```
Network stack initialized successfully
Configuring network interface stm0...
UDP ready to 192.168.0.104:5000
```

## References
- [RTEMS LibBSD Documentation](https://docs.rtems.org/doc-current/c-user/networking.html)
- [RTEMS WAF Build System](https://docs.rtems.org/doc-current/user/build/index.html)
- [STM32H7 Ethernet Support in RTEMS](https://docs.rtems.org/doc-current/arm/nucleo-h743zi.html)
