# ArithOS Flashing Guide

## Flash firmware to Raspberry Pi Pico

After successfully building ArithOS, you can flash the firmware to your Raspberry Pi Pico.

### What you need

- ArithOS successfully built (see `INSTALL.md`)
- Raspberry Pi Pico or PicoCalc
- USB cable (USB-A to Micro-USB or USB-C)

### Step 1: Put Pico in bootloader mode

1. **Find BOOTSEL button:**
   - On the Raspberry Pi Pico, the BOOTSEL button is a small button on the back
   - On PicoCalc devices, it's usually labeled "BOOT" or "RESET"

2. **Activate bootloader mode:**
   - Hold BOOTSEL button
   - Connect USB cable to computer
   - Release BOOTSEL button (after connecting)

3. **Confirm success:**
   - A new drive named "RPI-RP2" should appear in your file explorer
   - The drive contains `INDEX.HTM` and `INFO_UF2.TXT` files

### Step 2: Flash firmware

1. **Copy UF2 file:**
   - Open the `build/` folder in your ArithOS directory
   - Copy the `arithmos.uf2` file to the RPI-RP2 drive
   - Simply drag & drop or copy & paste

2. **Automatic flashing:**
   - The Pico automatically recognizes the UF2 file
   - Flashing starts immediately
   - The LED on the Pico blinks during the process

3. **Done:**
   - After flashing, the Pico automatically restarts
   - The RPI-RP2 drive disappears
   - ArithOS is now installed on the Pico!

### Step 3: Test ArithOS

1. **Connect device:**
   - Connect Pico with USB cable to computer
   - ArithOS should start automatically

2. **Test functions:**
   - Calculator: Basic calculations
   - Stopwatch: Time measurement
   - WiFi Scanner: Network scan (if WiFi module present)
   - BadUSB: USB-HID functions
   - Code Editor: Text editing

### Troubleshooting

**RPI-RP2 drive doesn't appear:**
- Press BOOTSEL button again and release
- Try different USB cable
- Try different USB port on computer
- Reconnect Pico

**UF2 file not recognized:**
- Make sure the file is named `arithmos.uf2` (not `.uf2.txt`)
- Copy file directly to RPI-RP2 drive (not in subfolder)
- Put Pico back in bootloader mode

**ArithOS doesn't start:**
- Completely restart Pico (disconnect and reconnect USB)
- Reflash firmware
- Run build process again

**Display stays black:**
- Check hardware connections (display cable)
- Restart Pico
- Reflash firmware

### Alternative Flashing Methods

**With picotool (if installed):**
```bash
picotool load build/arithmos.uf2
```

**With rp2040load (Python):**
```bash
pip install rp2040load
rp2040load build/arithmos.uf2
```

### Firmware Updates

To update ArithOS:

1. **Build new version:**
   ```bash
   cmake --build build -j
   python create_uf2.py
   ```

2. **Flash firmware:**
   - Put Pico in bootloader mode
   - Copy new `arithmos.uf2` to RPI-RP2

### Support

For problems:
- GitHub Issues: [ArithOS Repository](https://github.com/EinsPommes/ArithOS)
- Documentation: See `README.md` and `INSTALL.md`
- Community: Raspberry Pi Pico Community Forums
