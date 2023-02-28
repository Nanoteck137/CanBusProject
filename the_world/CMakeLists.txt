cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(the_world C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(the_world
	main.cpp
	usb_descriptors.c)

target_include_directories(the_world PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(the_world PUBLIC pico_stdlib tinyusb_device tinyusb_board)

pico_add_extra_outputs(the_world)
