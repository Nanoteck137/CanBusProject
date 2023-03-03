cmake_minimum_required(VERSION 3.13)

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS-Kernel)

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(the_world C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(the_world
	src/main.cpp
	src/usb_descriptors.c

	../tusk/tusk.cpp
	../pico-mcp2515/include/mcp2515/mcp2515.cpp
	)

target_include_directories(the_world PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(the_world PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../tusk)
target_include_directories(the_world PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../pico-mcp2515/include)

target_link_libraries(the_world PRIVATE
	pico_stdlib
    hardware_spi
	tinyusb_device
	tinyusb_board
	FreeRTOS-Kernel
	FreeRTOS-Kernel-Heap2
	)

pico_add_extra_outputs(the_world)
