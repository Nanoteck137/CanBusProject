cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(the_world C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(the_world
	main.cpp
	)

target_link_libraries(the_world pico_stdlib)

pico_enable_stdio_usb(the_world 1)
pico_enable_stdio_uart(the_world 0)

pico_add_extra_outputs(the_world)
