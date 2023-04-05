#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"

#include "com.h"
#include "can.h"
#include "device.h"

#include "util/serial_number.h"
#include "util/status_light.h"
#include "util/button.h"

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

    can_init();

    stdio_uart_init();
    stdio_set_driver_enabled(&debug_driver, true);
}

void usb_thread(void* ptr)
{
    do
    {
        tud_task();
        vTaskDelay(1);
    } while (1);
}

void update_thread(void* ptr)
{
    DeviceContext* device = (DeviceContext*)ptr;
    spec.init(device);

    while (true)
    {
        spec.update(device);
        can_update();

        vTaskDelay(1);
    }
}

static TaskHandle_t usb_thread_handle;
static TaskHandle_t can_thread_handle;
static TaskHandle_t update_thread_handle;
static TaskHandle_t com_thread_handle;

void init_device(DeviceContext* context)
{
    context->num_lines = spec.num_lines;
    context->num_controls = spec.num_controls;

    for (int i = 0; i < spec.num_lines; i++)
        context->lines[i].init(spec.lines[i]);

    for (int i = 0; i < spec.num_controls; i++)
        context->controls[i].init(spec.controls[i]);

    size_t num_cmds = 0;

    for (int i = 0;; i++)
    {
        if (spec.funcs[i])
            num_cmds++;
        else
            break;
    }

    context->num_cmds = num_cmds;
}

static DeviceContext device_context;

int main()
{
    init_system();

    init_device(&device_context);

    // SP Device:
    //  - COM
    //  - Check Can bus

    // GE Device:
    //  - COM
    //  - Check Can bus
    //  - Check pins

    xTaskCreate(usb_thread, "USB Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 4, &usb_thread_handle);
    // xTaskCreate(can_thread, "Can Thread", configMINIMAL_STACK_SIZE, nullptr,
    //             tskIDLE_PRIORITY + 3, &can_thread_handle);
    xTaskCreate(update_thread, "Update Thread", configMINIMAL_STACK_SIZE,
                &device_context, tskIDLE_PRIORITY + 2, &update_thread_handle);
    xTaskCreate(com_thread, "COM Thread", configMINIMAL_STACK_SIZE,
                &device_context, tskIDLE_PRIORITY + 1, &com_thread_handle);

    vTaskStartScheduler();
}

extern "C" void vApplicationTickHook() {}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t Task,
                                              char* pcTaskName)
{
    panic("stack overflow (not the helpful kind) for %s\n", *pcTaskName);
}

extern "C" void vApplicationMallocFailedHook() { panic("Malloc Failed\n"); }
