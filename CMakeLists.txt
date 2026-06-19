# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

#set(PICO_PLATFORM rp2350 CACHE STRING "Platform type" FORCE)
set(PICO_BOARD pico2_w CACHE STRING "Board type" FORCE)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
#include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(LoRa-TEST C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(LoRa-TEST LoRa-TEST.cpp sx126x.cpp)

pico_set_program_name(LoRa-TEST "LoRa-TEST")
pico_set_program_version(LoRa-TEST "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(LoRa-TEST 0)
pico_enable_stdio_usb(LoRa-TEST 1)

# Add the standard include files to the build
target_include_directories(LoRa-TEST PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(LoRa-TEST 
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_cyw43_arch_none
    #pico_cyw43_arch 
    #hardware_i2c
        )

pico_add_extra_outputs(LoRa-TEST)
