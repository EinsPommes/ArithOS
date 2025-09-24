# ArithOS Installation

This guide will help you set up and flash ArithOS onto your PicoCalc device.

## Prerequisites

- Raspberry Pi Pico
- PicoCalc hardware (display, keypad, etc.)
- Computer with Linux, macOS, or Windows
- Git
- CMake (version 3.13+)
- Pico SDK

## Quick Start

1. **Install Pico SDK**

```bash
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=$(pwd)
cd ..
```

2. **Build ArithOS**

```bash
git clone https://github.com/EinsPommes/ArithOS.git
cd ArithOS
./build.sh
```

3. **Flash**

- Hold BOOTSEL while connecting the Pico to your computer
- Copy build/arithmos.uf2 to the RPI-RP2 drive

## Troubleshooting

If you encounter issues:

- Check if `PICO_SDK_PATH` is set correctly
- Make sure all dependencies are installed
- Delete the build folder and try again

## Development

ArithOS is modular. Adding new apps is easy:

1. Create a new file in `src/apps/`
2. Implement the app interface
3. Register your app in `app_manager.c`

For more details, see the README.md.