#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"

#include "com.h"

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

#include "mcp2515/mcp2515.h"

// TODO(patrik):
//   - Change the name of command_queue to command_buffer
//   - Refactoring

MCP2515 can0(spi0, 5, 3, 4, 2);

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
    board_init();
    tusb_init();

    stdio_uart_init();
    stdio_set_driver_enabled(&debug_driver, true);

    can0.reset();
    can0.setBitrate(CAN_125KBPS, MCP_8MHZ);
    can0.setNormalMode();

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

void can_bus_thread(void* ptr)
{
    // TODO(patrik): We need to read all the sensor data
    // TODO(patrik): Then we store the data so we can retrive it later
    // TODO(patrik): Then use the data to send updates

    // NOTE(patrik):
    // Device On the Can Bus Network
    // Every devices gets 4 ids allocated to them
    // ID Allocation:
    //  id + 0x00 - Communication ID
    //  id + 0x01 - Outputs (Control Relays)
    //  id + 0x02 - Inputs (State of the inputs, Send to main unit)
    //  id + 0x03 - Misc

    while (true)
    {
        can_frame frame;
        if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        {
            // for (int i = 0; i < NUM_DEVICES; i++)
            // {
            //     Device* device = devices + i;
            //     if (frame.can_id == device->line_id() && frame.can_dlc == 2)
            //     {
            //         uint8_t lines = frame.data[0];
            //         uint8_t toggled_lines = frame.data[1];
            //         device->lines = lines;
            //         device->toggled_lines = toggled_lines;
            //         printf("Wot: %d %d\n", lines, toggled_lines);
            //     }
            // }
        }

        // for (int i = 0; i < NUM_DEVICES; i++)
        // {
        //     Device* device = devices + i;
        //     if (device->need_update)
        //     {
        //         can_frame send;
        //         send.can_id = device->control_id();
        //         send.can_dlc = 1;
        //         send.data[0] = device->controls;
        //         can0.sendMessage(&send);
        //
        //         device->need_update = false;
        //     }
        // }

        vTaskDelay(1);
    }
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
static TaskHandle_t com_thread_handle;
static TaskHandle_t can_bus_thread_handle;

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
    xTaskCreate(can_bus_thread, "Can Bus Thread", configMINIMAL_STACK_SIZE,
                nullptr, tskIDLE_PRIORITY + 3, &can_bus_thread_handle);
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
