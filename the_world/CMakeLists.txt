cmake_minimum_required(VERSION 3.13)

include(CMakePrintHelpers)
cmake_print_variables(CMAKE_CURRENT_BINARY_DIR)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../third_party)
set(FREERTOS_KERNEL_PATH ${THIRD_PARTY_DIR}/FreeRTOS-Kernel)
set(TUSK_BINDINGS_PATH ${CMAKE_CURRENT_LIST_DIR}/../build/tusk/)

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(the_world C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if(NOT DEFINED DEVICE_NAME)
	message(FATAL_ERROR "DEVICE_NAME not defined")
endif()

add_executable(the_world
	src/main.cpp
	src/com.cpp
	src/can.cpp
	src/device.cpp
	src/usb_descriptors.cpp

	src/device/${DEVICE_NAME}.cpp

	src/util/serial_number.cpp
	src/util/status_light.cpp
	src/util/button.cpp

	${THIRD_PARTY_DIR}/pico-mcp2515/include/mcp2515/mcp2515.cpp
	)

target_include_directories(the_world PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(the_world PRIVATE ${TUSK_BINDINGS_PATH})
target_include_directories(the_world PRIVATE ${THIRD_PARTY_DIR}/pico-mcp2515/include)

target_link_libraries(the_world PRIVATE
	pico_stdlib
	pico_unique_id
	hardware_spi

	tinyusb_device
	tinyusb_board

	FreeRTOS-Kernel
	FreeRTOS-Kernel-Heap2
	)

pico_add_extra_outputs(the_world)
