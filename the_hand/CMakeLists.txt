cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(the_hand C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(the_hand
	src/main.cpp

	../third_party/pico-mcp2515/include/mcp2515/mcp2515.cpp
	)

pico_enable_stdio_usb(the_hand 1)
pico_enable_stdio_uart(the_hand 1)

target_include_directories(the_hand PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(the_hand PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../tusk)
target_include_directories(the_hand PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/pico-mcp2515/include)

target_link_libraries(the_hand PRIVATE
	pico_stdlib
	hardware_spi
	hardware_pio
	)

pico_add_extra_outputs(the_hand)
