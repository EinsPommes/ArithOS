# ArithOS Installation

This guide will help you set up and build ArithOS for your PicoCalc device.

## What you need

- Raspberry Pi Pico
- PicoCalc hardware (display, keypad, etc.)
- Computer with Linux, macOS, or Windows
- Git
- CMake (version 3.13+)
- Pico SDK
- ARM GNU Toolchain (for cross-compilation)

## Quick Start

1. **Install Pico SDK**

```bash
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=$(pwd)
cd ..
```

2. **Install ARM GNU Toolchain**

**Windows:**
- Download from: https://developer.arm.com/downloads/-/gnu-rm
- Extract to a folder (e.g., `C:\gcc-arm-none-eabi-10.3-2021.10\`)
- Add `bin` folder to PATH

**Linux/macOS:**
```bash
# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi

# macOS (with Homebrew)
brew install arm-none-eabi-gcc
```

3. **Build ArithOS**

```bash
git clone https://github.com/EinsPommes/ArithOS.git
cd ArithOS
cmake -S . -B build -G "Ninja" -DPICO_SDK_PATH="$PICO_SDK_PATH" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
python create_uf2.py
```

4. **Flash ArithOS**

**Detailed guide:** See [FLASH_GUIDE.md](FLASH_GUIDE.md)

**Quick version:**
- Hold BOOTSEL button and connect Pico
- Copy `build/arithmos.uf2` to the RPI-RP2 drive
- Pico will automatically restart with ArithOS

## Troubleshooting

### Build problems

**CMake errors:**
- Is `PICO_SDK_PATH` set correctly? → `echo $PICO_SDK_PATH`
- Is ARM toolchain in PATH? → `arm-none-eabi-gcc --version`
- Delete build folder: `rm -rf build/`

**Compiler errors:**
- Missing definitions → Check header files
- TinyUSB configuration → Is `src/tusb_config.h` present?
- Duplicate definitions → Warnings are normal, but check them

**Windows-specific problems:**
- Use PowerShell instead of Bash
- Use Ninja generator: `-G "Ninja"`
- picotool disabled (in CMakeLists.txt)

### Flashing problems

**RPI-RP2 drive doesn't appear:**
- Press BOOTSEL button again
- Try different USB cable/port
- Reconnect Pico

**UF2 file not recognized:**
- Copy file directly to RPI-RP2 (not in subfolder)
- Filename must end with `.uf2` (not `.uf2.txt`)

**ArithOS doesn't start:**
- Restart Pico completely
- Reflash firmware
- Check hardware connections

### More help

- **Flashing:** See [FLASH_GUIDE.md](FLASH_GUIDE.md)
- **GitHub Issues:** [ArithOS Repository](https://github.com/EinsPommes/ArithOS)
- **Community:** Raspberry Pi Pico Forums

## Development

ArithOS is modular. Adding new apps is easy:

1. Create a new file in `src/apps/`
2. Implement the app interface
3. Register your app in `app_manager.c`

For more details, see the README.md.