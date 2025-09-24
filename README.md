# ArithOS - Pentesting Tool for PicoCalc

A disguised hacking firmware for PicoCalc hardware. Looks like a calculator, but is actually a complete pentesting toolkit.

## Features

- **Cover Apps:** Calculator, Stopwatch
- **Hacking Tools:**
  - WiFi Scanner (finds networks in range)
  - Packet Sniffer (captures WLAN packets)
  - WiFi Bruteforce (cracks passwords)
  - BadUSB (automated keyboard inputs)
- **Development:**
  - Code Editor with syntax highlighting
  - File Browser

## Hardware

- Raspberry Pi Pico
- 240x240 Display
- 4x4 Keypad
- ESP8266/ESP32 for WiFi

## Build & Flash

```bash
# Set SDK path
export PICO_SDK_PATH=/path/to/pico-sdk

# Build
./build.sh

# Flash: Hold BOOTSEL and copy arithmos.uf2 to RPI-RP2
```

## Warning

Only use for legitimate security testing with explicit permission!

## License

MIT