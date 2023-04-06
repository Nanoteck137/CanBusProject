#include "util/button.h"
#include "util/status_light.h"
#include "device.h"
#include "func.h"
#include "can.h"

struct Context
{
    PhysicalControl* relay;
    PhysicalControl* backup_lamps;
    StatusLight light;
    Button button;

    bool test;

    uint64_t send_update_timer;

    void update_status()
    {
        if (backup_lamps->is_on())
        {
            light.blink_count(500 * 1000, 2);
            return;
        }

        light.set(relay->is_on());
    }
};

static Context context;

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

void init(DeviceContext* device)
{
    context.relay = &device->controls[3];
    context.backup_lamps = &device->controls[5];
    context.light.init(&device->controls[2]);
}

void update(DeviceContext* device)
{
    uint64_t current_time = time_us_64();

    bool state = device->lines[2].get();
    context.button.update(state);
    context.light.update();

    if (context.button.is_single_click())
    {
        context.relay->toggle();
        context.update_status();
    }

    if (context.button.is_long_click())
    {
        context.test = !context.test;
    }

    if (current_time - context.send_update_timer > 100 * 1000)
    {
        uint8_t status =
            (uint8_t)context.test << 1 | (uint8_t)context.relay->is_on() << 0;
        uint8_t data[] = {status};
        send_can_message(0x100, data, sizeof(data));
        context.send_update_timer = current_time;
    }

    button_test("Button", &context.button);
}

void get_status(uint8_t* buffer)
{
    // Byte 0 - Relay Status
    buffer[0] = context.relay->is_on();
}

static void on_can_message(uint32_t can_id, uint8_t* data, size_t len)
{
    printf("Got CAN Message: 0x%x\n", can_id);

    // if (send_can_message(0x200, nullptr, 0))
    //     printf("Sent CAN Message: Success\n");
    // else
    //     printf("Sent CAN Message: Failed\n");
}

DEFINE_CMD(change_first_relay)
{
    EXPECT_NUM_PARAMS(1);

    uint8_t control = GET_PARAM(0);

    bool on = (control & 0x01) == 0x01;
    context.relay->set(on);
    context.update_status();

    return ErrorCode::Success;
}

DEFINE_CMD(change_backup_lamps)
{
    EXPECT_NUM_PARAMS(1);

    uint8_t control = GET_PARAM(0);

    bool on = (control & 0x01) == 0x01;
    context.backup_lamps->set(on);
    context.update_status();

    return ErrorCode::Success;
}

const DeviceSpec spec = {
    .name = "Test Controller",
    .version = MAKE_VERSION(0, 1, 0),

    .num_lines = 6,
    .lines = {10, 11, 12, 19, 20, 21},

    .num_controls = 6,
    .controls = {5, 7, 8, 16, 17, 18},

    .init = init,
    .update = update,
    .get_status = get_status,
    .on_can_message = on_can_message,

    .funcs =
        {
            change_first_relay,  // 0x00
            change_backup_lamps, // 0x01
            nullptr,             // LAST
        },
};
