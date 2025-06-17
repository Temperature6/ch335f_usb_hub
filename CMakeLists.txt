cmake_minimum_required(VERSION 3.21)
# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(rp2040_ch335f_usb_hub C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

# Initialize the SDK
pico_sdk_init()

set(DRV_INA219_DIR drv_ina219)
file(GLOB SRC_INA219 ${DRV_INA219_DIR}/*.c)
include_directories(${DRV_INA219_DIR})

set(DRV_ST7789_DIR drv_st7789)
file(GLOB SRC_ST7789 ${DRV_ST7789_DIR}/*.c)
include_directories(${DRV_ST7789_DIR})

set(UI_DIR ui)
include_directories(${UI_DIR})
file(GLOB_RECURSE SRC_UI ${UI_DIR}/*.c)

add_subdirectory(lvgl-8.3.5)
add_executable(${PROJECT_NAME} main.cpp InfoLabel.cpp
        ${SRC_INA219} ${SRC_ST7789} ${SRC_UI}
        lvgl-8.3.5/lv_port_disp.c
        lvgl-8.3.5/lv_port_indev.c)
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_i2c hardware_spi hardware_dma lvgl lvgl::demos)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
