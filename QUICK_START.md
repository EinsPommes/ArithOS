# ArithOS Quick Start

## Getting started with ArithOS

Welcome to ArithOS! This guide helps you get started quickly.

### What you need

- **Hardware:** Raspberry Pi Pico or PicoCalc
- **Software:** Computer with Windows/Linux/macOS
- **Time:** ~30 minutes for complete installation

### Installation & Build

**1. Download and build ArithOS:**
```bash
git clone https://github.com/EinsPommes/ArithOS.git
cd ArithOS
```

**2. Follow detailed guide:**
📖 **[INSTALL.md](INSTALL.md)** - Complete installation guide

### Flash firmware

**1. Prepare Pico:**
- Hold BOOTSEL button
- Connect USB cable
- RPI-RP2 drive should appear

**2. Copy firmware:**
- Copy `build/arithmos.uf2` to RPI-RP2 drive
- Pico will automatically restart

**3. Detailed guide:**
📖 **[FLASH_GUIDE.md](FLASH_GUIDE.md)** - Complete flashing guide

### First steps with ArithOS

After successful flashing you can:

- **Calculator:** Basic calculations
- **Stopwatch:** Time measurement
- **WiFi Scanner:** Network scan
- **BadUSB:** USB-HID functions
- **Code Editor:** Text editing

### Need help?

**Common problems:**
- **Build fails:** See [INSTALL.md](INSTALL.md#troubleshooting)
- **Flashing doesn't work:** See [FLASH_GUIDE.md](FLASH_GUIDE.md#troubleshooting)
- **ArithOS doesn't start:** Check hardware connections

**More help:**
- **GitHub Issues:** [ArithOS Repository](https://github.com/EinsPommes/ArithOS)
- **Community:** Raspberry Pi Pico Forums
- **Documentation:** See `README.md`

### Documentation

- **[INSTALL.md](INSTALL.md)** - Installation & Build
- **[FLASH_GUIDE.md](FLASH_GUIDE.md)** - Flash firmware
- **[README.md](README.md)** - Project overview
