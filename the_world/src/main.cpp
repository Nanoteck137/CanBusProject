#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"

#include "com.h"
#include "can.h"

#include "util/serial_number.h"
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

    // for (int i = 0; i < NUM_DEVICES; i++)
    // {
    //     device_data[i].device = config.devices[i];
    // }
    //
    // can_init();
    //
    // if (config.type == DeviceType::GoldExperience)
    // {
    //     for (int i = 0; i < config.num_lines; i++)
    //     {
    //         gpio_init(config.lines[i]);
    //         gpio_set_dir(config.lines[i], GPIO_IN);
    //         gpio_set_pulls(config.lines[i], true, false);
    //     }
    //
    //     for (int i = 0; i < config.num_controls; i++)
    //     {
    //         gpio_init(config.controls[i]);
    //         gpio_set_dir(config.controls[i], GPIO_OUT);
    //     }
    // }
}

void usb_thread(void* ptr)
{
    do
    {
        tud_task();
        vTaskDelay(1);
    } while (1);
}

#define MAX_BUTTONS 8

const size_t NUM_LINES = 3;

const uint32_t LEFT_BUTTON_LINE = 10;
const uint32_t MIDDLE_BUTTON_LINE = 11;
const uint32_t RIGHT_BUTTON_LINE = 12;

const uint32_t LEFT_BUTTON_STATUS_LIGHT = 5;
const uint32_t MIDDLE_BUTTON_STATUS_LIGHT = 7;
const uint32_t RIGHT_BUTTON_STATUS_LIGHT = 8;

enum class StatusLightState
{
    Off,
    On,
    Blink,
    BlinkCount,
};

struct StatusLight
{
    uint32_t pin = 0xffffffff;
    StatusLightState state = StatusLightState::Off;
    bool state_changed = false;

    uint64_t last_time = 0;
    uint64_t blink_time = 0;
    uint32_t blink_count = 0;
    uint32_t current_blink_count = 0;
    bool blink_value = false;

    void init(uint32_t pin)
    {
        this->pin = pin;

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, false);
    }

    void update()
    {
        uint32_t current_time = time_us_64();

        if (state_changed)
        {
            switch (state)
            {
                case StatusLightState::On: gpio_put(pin, true); break;
                case StatusLightState::Off: gpio_put(pin, false); break;
                case StatusLightState::Blink: {
                    last_time = current_time;
                    gpio_put(pin, false);
                }
                break;
                case StatusLightState::BlinkCount: {
                    last_time = current_time;
                    gpio_put(pin, false);
                }
                break;
            }

            state_changed = false;
        }

        switch (state)
        {
            case StatusLightState::Blink: {
                if (current_time - last_time > blink_time)
                {
                    blink_value = !blink_value;
                    gpio_put(pin, blink_value);
                    last_time = current_time;
                }
            }

            case StatusLightState::BlinkCount: {
                if (current_time - last_time > blink_time)
                {
                    if (current_blink_count >= blink_count)
                    {
                        gpio_put(pin, false);
                    }
                    else
                    {
                        blink_value = !blink_value;
                        gpio_put(pin, blink_value);
                        printf("Before: %d\n", current_blink_count);
                        if (blink_value)
                            current_blink_count++;
                        printf("After: %d\n", current_blink_count);
                    }

                    last_time = current_time;
                }
            }

            break;
            default: break;
        }
    }

    void set_blink(uint64_t blink_time)
    {
        state = StatusLightState::Blink;
        this->blink_time = blink_time;
    }

    void set_blink_toggle(uint64_t blink_time)
    {
        if (state == StatusLightState::Blink)
        {
            state = StatusLightState::Off;
        }
        else
        {
            set_blink(blink_time);
        }
    }

    void set_blink_count(uint64_t blink_time, uint32_t blink_count)
    {
        state = StatusLightState::BlinkCount;
        this->blink_time = blink_time;
        this->blink_count = blink_count;
        this->current_blink_count = 0;
    }

    void set_toggle_on_off()
    {
        if (state == StatusLightState::Off)
        {
            state = StatusLightState::On;
            state_changed = true;
        }
        else if (state == StatusLightState::On)
        {
            state = StatusLightState::Off;
            state_changed = true;
        }
        else
        {
            state = StatusLightState::On;
            state_changed = true;
        }
    }
};

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
        {
            context.right_status.set_toggle_on_off();
        }

        // bool left_state = !gpio_get(lines[0]);
        // bool middle_state = !gpio_get(lines[1]);
        // bool right_state = !gpio_get(lines[2]);
        //
        // left.update(left_state && !middle_state && !right_state);
        // middle.update(middle_state && !left_state && !right_state);
        // right.update(right_state && !left_state && !middle_state);
        //
        // left_middle.update(left_state && middle_state);
        // middle_right.update(middle_state && right_state);
        // left_right.update(left_state && right_state);
        //
        // if (left.is_click())
        //     printf("Left Click\n");
        // if (middle.is_click())
        //     printf("Middle Click\n");
        // if (left_middle.is_click())
        //     printf("Left Middle Click\n");
        //
        // if (left.is_single_click())
        //     printf("Left Single Click\n");
        // if (middle.is_single_click())
        //     printf("Middle Single Click\n");
        // if (left_middle.is_single_click())
        //     printf("Left Middle Single Click\n");

        // if (left.is_click())
        //     printf("Left\n");
        // if (middle.is_click())
        //     printf("Middle\n");
        // if (right.is_click())
        //     printf("Right\n");
        // if (left_middle.is_click())
        //     printf("Left Middle\n");
        // if (middle_right.is_click())
        //     printf("Middle Right\n");
        // if (left_right.is_click())
        //     printf("Left Right\n");

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
