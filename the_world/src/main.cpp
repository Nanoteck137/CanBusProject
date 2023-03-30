#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"

#include "serial_number.h"
#include "com.h"
#include "can.h"

#include "tusk.h"

#include "FreeRTOS.h"
#include "task.h"

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "pico/stdio/driver.h"

#include "bsp/board.h"
#include "class/cdc/cdc_device.h"
#include "tusb.h"

// const size_t NUM_DEVICES = 1;
// static_assert(NUM_DEVICES <= MAX_DEVICES, "Too many devices");
// static Device devices[NUM_DEVICES];

static void debug_driver_output(const char* buf, int length)
{
    tud_cdc_n_write(PORT_DEBUG, buf, length);
    tud_cdc_n_write_flush(PORT_DEBUG);
}

stdio_driver_t debug_driver = {
    .out_chars = debug_driver_output,
};

void init_system()
{
    serial_number_init();
    board_init();
    tusb_init();

    stdio_uart_init();
    stdio_set_driver_enabled(&debug_driver, true);

    can_init();

    // init_devices();
}

void usb_thread(void* ptr)
{
    do
    {
        tud_task();
        vTaskDelay(1);
    } while (1);
}

// void test_thread(void* ptr)
// {
//     Device* device = devices + 0;
//     bool last = (device->toggled_lines & 0x1) > 0;
//     while (true)
//     {
//         bool current = (device->toggled_lines & 0x1) > 0;
//
//         if (current && last != current)
//         {
//             printf("On\n");
//             device->controls = 0b1;
//             device->need_update = true;
//         }
//
//         if (!current && last != current)
//         {
//             printf("Off\n");
//             device->controls = 0b0;
//             device->need_update = true;
//         }
//
//         last = current;
//
//         vTaskDelay(1);
//     }
// }

static TaskHandle_t usb_thread_handle;
static TaskHandle_t can_thread_handle;
static TaskHandle_t com_thread_handle;

int main()
{
    init_system();

    // SP Device:
    //  - COM
    //  - Check Can bus

    // GE Device:
    //  - COM
    //  - Check Can bus
    //  - Check pins

    xTaskCreate(usb_thread, "USB Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 4, &usb_thread_handle);
    xTaskCreate(can_thread, "Can Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 3, &can_thread_handle);
    // xTaskCreate(test_thread, "Test Thread", configMINIMAL_STACK_SIZE,
    // nullptr,
    //             tskIDLE_PRIORITY + 2, &can_bus_thread_handle);
    xTaskCreate(com_thread, "COM Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 1, &com_thread_handle);

    vTaskStartScheduler();
}

extern "C" void vApplicationTickHook() {}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t Task,
                                              char* pcTaskName)
{
    panic("stack overflow (not the helpful kind) for %s\n", *pcTaskName);
}

extern "C" void vApplicationMallocFailedHook() { panic("Malloc Failed\n"); }
