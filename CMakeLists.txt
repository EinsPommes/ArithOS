cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ArithOS C CXX)
pico_sdk_init()

add_executable(arithmos
  src/main.c
  src/board_pins.h
  src/keys.c
  src/display.c
  src/apps/app_manager.c
  src/apps/launcher_app.c
  src/apps/calculator_app.c
  src/apps/stopwatch_app.c
  src/apps/wifi_scanner_app.c
  src/apps/badusb_app.c
  src/apps/packet_sniffer_app.c
  src/apps/bruteforce_app.c
  src/apps/code_editor_app.c
)

target_include_directories(arithmos PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(arithmos
  pico_stdlib
  hardware_spi
  hardware_i2c
  hardware_pwm
  hardware_adc
  hardware_gpio
  hardware_uart
  hardware_flash
  pico_multicore
  tinyusb_device
  tinyusb_board
)

pico_enable_stdio_usb(arithmos 0)
pico_enable_stdio_uart(arithmos 1)
# pico_add_extra_outputs(arithmos)  # Temporarily disabled to avoid picotool issues