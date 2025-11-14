# RTEMS-Hello-World

This is a "Hello World" example project for the [RTEMS Real-Time Operating System](https://www.rtems.org/) that uses the [Waf build system](https://waf.io/) via the `rtems_waf` submodule.

The application senses the blue user pushbutton on the target board and sends a string through the serial console (UART) when it is pressed.

## Prerequisites

Before you begin, ensure you have the following installed:

1.  **RTEMS Toolchain and BSPs**: You need a working RTEMS development environment for your target architecture. For instructions, please see the RTEMS Documentation.
2.  **Waf Build Tool**: Waf is a Python-based build system. You can download it from the official website but it should be already there.
    ```shell
    # Download waf
    wget https://waf.io/waf-2.0.19

    # Make it executable and place it in your path
    chmod +x waf-2.0.19
    mv waf-2.0.19 ~/bin/waf
    ```
3.  **Git**: For cloning the repository.

## Getting Started

### 1. Clone the Repository

Clone the repository and its `rtems_waf` submodule:

```shell
git clone --recurse-submodules https://github.com/Clod/RTEMS-Hello-World.git
cd RTEMS-Hello-World
```

If you cloned without `--recurse-submodules`, you can initialize the submodule separately:

```shell
git submodule update --init
```

### 2. Configure the Build

Run `waf configure` and point it to your RTEMS installation and the desired Board Support Package (BSP).

The example below is for RTEMS 6.1 on the `nucleo-h743zi` board. **You must change these paths and BSP to match your setup.**

```shell
./waf configure --rtems=/opt/rtems/6.1 --rtems-bsp=arm/nucleo-h743zi
```

### 3. Build the Application

Compile the project by running `waf`:

```shell
./waf
```

The executable will be created in the `build/` directory, inside a folder named after your toolchain and BSP. For the example above, it would be: `build/arm-rtems6-nucleo-h743zi/hello.exe`.

## 4. Flash and Run

### Flashing the Board

Copy the `hello.exe` executable to your board. The method for flashing depends on your specific hardware and debug probe (e.g., OpenOCD, J-Link, ST-Link).

For example, if you are using a development environment where a host directory is mounted (like a Docker container), you might copy it like this:

```shell
# This is an example of copying the file to a shared volume.
# The path /out/hello.elf is specific to a particular dev environment.
# This should be automatically done by waf
cp build/arm-rtems6-nucleo-h743zi/hello.exe /out/hello.elf
```

### Monitoring Output

You can view the "Hello World" message by connecting to the board's serial port with a terminal emulator like `screen`, `minicom`, or PuTTY.

The serial device name will vary depending on your operating system and hardware.

**Example on macOS or Linux:**

```shell
# The device /dev/tty.usbmodem11303 may be different on your system.
screen /dev/tty.usbmodem11303 115200
```
