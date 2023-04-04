#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"

#include "com.h"
#include "can.h"

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

Config config = {.name = "Testing Device",
                 .version = MAKE_VERSION(0, 1, 0),
                 .type = DeviceType::GoldExperience};

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

const uint32_t LEFT_BUTTON_LINE = 10;
const uint32_t MIDDLE_BUTTON_LINE = 11;
const uint32_t RIGHT_BUTTON_LINE = 12;

const uint32_t LEFT_BUTTON_STATUS_LIGHT = 5;
const uint32_t MIDDLE_BUTTON_STATUS_LIGHT = 7;
const uint32_t RIGHT_BUTTON_STATUS_LIGHT = 8;

struct Context
{
    Button left;
    Button middle;
    Button right;
    Button left_middle;
    Button middle_right;
    Button left_right;

    StatusLight left_status;
    StatusLight middle_status;
    StatusLight right_status;

    void setup_line(uint32_t pin)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    void init()
    {
        setup_line(LEFT_BUTTON_LINE);
        setup_line(MIDDLE_BUTTON_LINE);
        setup_line(RIGHT_BUTTON_LINE);

        left_status.init(LEFT_BUTTON_STATUS_LIGHT);
        middle_status.init(MIDDLE_BUTTON_STATUS_LIGHT);
        right_status.init(RIGHT_BUTTON_STATUS_LIGHT);
    }

    void update()
    {
        bool left_line_state = !gpio_get(LEFT_BUTTON_LINE);
        bool middle_line_state = !gpio_get(MIDDLE_BUTTON_LINE);
        bool right_line_state = !gpio_get(RIGHT_BUTTON_LINE);

        left.update(left_line_state && !middle_line_state && !right_line_state);
        middle.update(middle_line_state && !left_line_state &&
                      !right_line_state);
        right.update(right_line_state && !left_line_state &&
                     !middle_line_state);

        left_middle.update(left_line_state && middle_line_state);
        middle_right.update(middle_line_state && right_line_state);
        left_right.update(left_line_state && right_line_state);

        left_status.update();
        middle_status.update();
        right_status.update();
    }
};

void button_test(const char* name, Button* button)
{
    if (button->is_click())
        printf("%s: Click\n", name);
    if (button->is_released())
        printf("%s: Released\n", name);

    if (button->is_single_click())
        printf("%s: Single click\n", name);
    if (button->is_long_click())
        printf("%s: Long click\n", name);
    if (button->is_double_click())
        printf("%s: Double click\n", name);
}

void update_thread(void* ptr)
{
    Context context;
    context.init();

    while (true)
    {
        context.update();

        button_test("Left", &context.left);
        button_test("Middle", &context.middle);
        button_test("Right", &context.right);

        button_test("Left Middle", &context.left_middle);
        button_test("Middle Right", &context.middle_right);
        button_test("Left Right", &context.left_right);

        if (context.right.is_double_click())
            context.right_status.blink_toggle(250 * 1000);

        vTaskDelay(1);
    }
}

static TaskHandle_t usb_thread_handle;
static TaskHandle_t can_thread_handle;
static TaskHandle_t update_thread_handle;
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
    xTaskCreate(update_thread, "Update Thread", configMINIMAL_STACK_SIZE,
                nullptr, tskIDLE_PRIORITY + 2, &update_thread_handle);
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
