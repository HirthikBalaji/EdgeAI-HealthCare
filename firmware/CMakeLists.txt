cmake_minimum_required(VERSION 3.15)
project(VITALIS_Edge_Firmware C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(FIRMWARE_SOURCES
    src/sensors/ad8232_ecg.c
    src/sensors/max30102_ppg.c
    src/sensors/lsm6dsox_imu.c
    src/sensors/tmp117_temp.c
    src/tinyml/dsp_filters.c
    src/tinyml/vitalis_tinyml.c
    src/triage/triage_engine.c
    src/triage/blackbox_storage.c
    src/comms/ble_service.c
)

# Static Library
add_library(vitalis_core STATIC ${FIRMWARE_SOURCES})
target_link_libraries(vitalis_core m)

# Main Firmware Executable
add_executable(vitalis_firmware src/main.c)
target_link_libraries(vitalis_firmware vitalis_core m)

# Unit Tests Executable
add_executable(test_vitalis tests/test_firmware.c)
target_link_libraries(test_vitalis vitalis_core m)
