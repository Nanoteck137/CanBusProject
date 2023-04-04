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

const uint8_t FUNC_TURN_CONTROL_ON = 0x00;
const uint8_t FUNC_TURN_CONTROL_OFF = 0x01;
const uint8_t FUNC_TOGGLE_CONTROL = 0x02;

#define DEFINE_FUNC(name) ErrorCode name(uint8_t* params, size_t num_params)

#define EXPECT_NUM_PARAMS(num)                                                 \
    if (num_params < num)                                                      \
    {                                                                          \
        return ErrorCode::InsufficientFunctionParameters;                      \
    }

#define GET_PARAM(index) params[index]

DEFINE_FUNC(turn_control_on)
{
    EXPECT_NUM_PARAMS(1);

    uint8_t control = GET_PARAM(0);

    return ErrorCode::Success;
}

DEFINE_FUNC(turn_control_off)
{
    EXPECT_NUM_PARAMS(1);
    return ErrorCode::Success;
}

DEFINE_FUNC(toggle_control)
{
    EXPECT_NUM_PARAMS(1);
    return ErrorCode::Success;
}

void update_thread(void* ptr)
{
    DeviceContext* device = (DeviceContext*)ptr;
    spec.init(device);

    while (true)
    {
        spec.update(device);
        vTaskDelay(1);
    }
}

static TaskHandle_t usb_thread_handle;
static TaskHandle_t can_thread_handle;
static TaskHandle_t update_thread_handle;
static TaskHandle_t com_thread_handle;

void init_device(DeviceContext* context)
{
    for (int i = 0; i < spec.num_lines; i++)
        context->lines[i].init(spec.lines[i]);

    for (int i = 0; i < spec.num_controls; i++)
        context->controls[i].init(spec.controls[i]);
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
    // xTaskCreate(com_thread, "COM Thread", configMINIMAL_STACK_SIZE, nullptr,
    //             tskIDLE_PRIORITY + 1, &com_thread_handle);

    vTaskStartScheduler();
}

extern "C" void vApplicationTickHook() {}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t Task,
                                              char* pcTaskName)
{
    panic("stack overflow (not the helpful kind) for %s\n", *pcTaskName);
}

extern "C" void vApplicationMallocFailedHook() { panic("Malloc Failed\n"); }
