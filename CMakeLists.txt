cmake_minimum_required(VERSION 3.13)

include (pico_sdk_import.cmake)
project (simple_vga C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(SIMPLE_VGA_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()
add_executable(simple_vga)
pico_generate_pio_header(simple_vga ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(simple_vga ${CMAKE_CURRENT_LIST_DIR}/vsync.pio) 
pico_generate_pio_header(simple_vga ${CMAKE_CURRENT_LIST_DIR}/nrgb.pio)
pico_generate_pio_header(simple_vga ${CMAKE_CURRENT_LIST_DIR}/z80io.pio)
target_sources(simple_vga PRIVATE main.c usb_main.c hid_app.c font.h text_mode.c text_mode.h ansi_terminal.c ansi_terminal.h graphics_mode.h graphics_mode.c main.h font.h bell.c bell.h basic_progs.c basic_progs.h keyboard.h keyboard.c)
target_include_directories(simple_vga PUBLIC ./)

target_link_libraries(simple_vga PRIVATE
        pico_stdlib
        hardware_pio
	hardware_dma
	hardware_pwm
	hardware_uart
	pico_multicore
	tinyusb_host
	tinyusb_board)

pico_add_extra_outputs(simple_vga)


