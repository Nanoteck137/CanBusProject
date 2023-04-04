#include "util/button.h"
#include "device.h"
#include "func.h"

struct Context
{
    PhysicalControl* relay;
    Button test;
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

void init(DeviceContext* device) { context.relay = &device->controls[3]; }

void update(DeviceContext* device)
{
    bool state = device->lines[0].get();
    context.test.update(state);

    button_test("Test", &context.test);
}

DEFINE_FUNC(change_first_relay)
{
    EXPECT_NUM_PARAMS(1);

    uint8_t control = GET_PARAM(0);

    bool on = (control & 0x01) == 0x01;
    context.relay->set(on);

    return ErrorCode::Success;
}

const DeviceSpec spec = {
    .name = "Test Controller",
    .version = MAKE_VERSION(0, 1, 0),
    .type = DeviceType::GoldExperience,

    .num_lines = 6,
    .lines = {10, 11, 12, 19, 20, 21},

    .num_controls = 6,
    .controls = {5, 7, 8, 16, 17, 18},

    .init = init,
    .update = update,

    .funcs =
        {
            change_first_relay, // 0x00
            nullptr,            // LAST
        },
};
