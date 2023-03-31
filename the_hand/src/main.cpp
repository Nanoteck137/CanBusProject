
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdio.h"
// #include "tusk.h"

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdio/driver.h"
#include "pico/stdlib.h"

#include "mcp2515/mcp2515.h"
#include "pico/time.h"

void init_system() { stdio_init_all(); }

#define PIN 19

#define CONTROL0 16
#define CONTROL1 17
#define CONTROL2 18

struct LineState
{
    bool last_button_state;
    uint32_t debounce_timer;

    bool state;
    bool last;
};

const int DEBOUNCE_DELAY = 50 * 1000;     // us
const int SINGLECLICK_DELAY = 350 * 1000; // us
const int LONGCLICK_DELAY = 500 * 1000;   // us

// NOTE(patrik): https://github.com/poelstra/arduino-multi-button
class Button
{
public:
    Button() : _lastTransition(time_us_64()), _state(StateIdle), _new(false){};

    void update()
    {
        bool pressed = !gpio_get(PIN);
        _new = false;

        // Optimization for the most common case: nothing happening.
        if (!pressed && _state == StateIdle)
        {
            return;
        }

        uint64_t now = time_us_64();
        int diff = now - _lastTransition;

        State next = StateIdle;
        switch (_state)
        {
            case StateIdle: next = _checkIdle(pressed, diff); break;
            case StateDebounce: next = _checkDebounce(pressed, diff); break;
            case StatePressed: next = _checkPressed(pressed, diff); break;
            case StateClickUp: next = _checkClickUp(pressed, diff); break;
            case StateClickIdle: next = _checkClickIdle(pressed, diff); break;
            case StateSingleClick:
                next = _checkSingleClick(pressed, diff);
                break;
            case StateDoubleClickDebounce:
                next = _checkDoubleClickDebounce(pressed, diff);
                break;
            case StateDoubleClick:
                next = _checkDoubleClick(pressed, diff);
                break;
            case StateLongClick: next = _checkLongClick(pressed, diff); break;
            case StateOtherUp: next = _checkOtherUp(pressed, diff); break;
        }

        if (next != _state)
        {
            _lastTransition = now;
            _state = next;
            _new = true;
        }
    }

    bool isClick() const
    {
        return _new && (_state == StatePressed || _state == StateDoubleClick);
    }

    bool isSingleClick() { return _new && _state == StateSingleClick; }
    bool isDoubleClick() { return _new && _state == StateDoubleClick; }
    bool isLongClick() { return _new && _state == StateLongClick; }
    bool isReleased()
    {
        return _new && (_state == StateClickUp || _state == StateOtherUp);
    }

private:
    enum State
    {
        StateIdle,
        StateDebounce,
        StatePressed,

        StateClickUp,
        StateClickIdle,

        StateSingleClick,

        StateDoubleClickDebounce,
        StateDoubleClick,

        StateLongClick,
        StateOtherUp,
    };

    uint64_t _lastTransition;
    State _state;
    bool _new;

    State _checkIdle(bool pressed, int diff)
    {
        (void)diff;
        // Wait for a key press
        return pressed ? StateDebounce : StateIdle;
    }

    State _checkDebounce(bool pressed, int diff)
    {
        // If released in this state: it was a glitch
        if (!pressed)
        {
            return StateIdle;
        }
        if (diff >= DEBOUNCE_DELAY)
        {
            // Still pressed after debounce delay: real 'press'
            return StatePressed;
        }
        return StateDebounce;
    }

    State _checkPressed(bool pressed, int diff)
    {
        // If released, go wait for either a double-click, or
        // to generate the actual SingleClick event,
        // but first mark that the button is released.
        if (!pressed)
        {
            return StateClickUp;
        }
        // If pressed, keep waiting to see if it's a long click
        if (diff >= LONGCLICK_DELAY)
        {
            return StateLongClick;
        }
        return StatePressed;
    }

    State _checkClickUp(bool pressed, int diff)
    {
        (void)pressed;
        (void)diff;
        return StateClickIdle;
    }

    State _checkClickIdle(bool pressed, int diff)
    {
        if (pressed)
        {
            return StateDoubleClickDebounce;
        }
        if (diff >= SINGLECLICK_DELAY)
        {
            return StateSingleClick;
        }
        return StateClickIdle;
    }

    State _checkSingleClick(bool pressed, int diff)
    {
        (void)pressed;
        (void)diff;

        return StateIdle;
    }

    State _checkDoubleClickDebounce(bool pressed, int diff)
    {
        if (!pressed)
        {
            return StateClickIdle;
        }
        if (diff >= DEBOUNCE_DELAY)
        {
            return StateDoubleClick;
        }
        return StateDoubleClickDebounce;
    }

    State _checkDoubleClick(bool pressed, int diff)
    {
        (void)diff;
        if (!pressed)
        {
            return StateOtherUp;
        }
        return StateDoubleClick;
    }

    State _checkLongClick(bool pressed, int diff)
    {
        (void)diff;
        if (!pressed)
        {
            return StateOtherUp;
        }
        return StateLongClick;
    }

    State _checkOtherUp(bool pressed, int diff)
    {
        (void)pressed;
        (void)diff;
        return StateIdle;
    }
};

int main()
{
    init_system();

    // uint lines[NUM_LINES] = {19, 20, 21};
    // LineState line_states[NUM_LINES] = {};

    gpio_init(PIN);
    gpio_set_dir(PIN, GPIO_IN);
    gpio_set_pulls(PIN, true, false);

    int index = 0;

    LineState line;

    Button button;

    bool control0 = false;
    bool control1 = false;
    bool control2 = false;

    gpio_init(CONTROL0);
    gpio_set_dir(CONTROL0, GPIO_OUT);

    gpio_init(CONTROL1);
    gpio_set_dir(CONTROL1, GPIO_OUT);

    gpio_init(CONTROL2);
    gpio_set_dir(CONTROL2, GPIO_OUT);

    while (true)
    {
        button.update();

        if (button.isClick())
        {
            printf("isClick\n");
        }

        if (button.isReleased())
            printf("isReleased\n");

        if (button.isSingleClick())
        {
            printf("isSingleClick\n");
            control1 = !control1;
            gpio_put(CONTROL1, control1);
        }

        if (button.isDoubleClick())
        {
            printf("isDoubleClick\n");
            control2 = !control2;
            gpio_put(CONTROL2, control2);
        }

        if (button.isLongClick())
        {
            printf("isLongClick\n");
            control0 = !control0;
            gpio_put(CONTROL0, control0);
        }

        // uint32_t current_time = to_ms_since_boot(get_absolute_time());
        //
        // bool button = !gpio_get(PIN);
        // if (button != line.last_button_state)
        // {
        //     line.debounce_timer = to_ms_since_boot(get_absolute_time());
        // }
        //
        // if (current_time - line.debounce_timer > 100)
        // {
        //     line.state = button;
        // }
        //
        // if (line.state && !line.last)
        // {
        //     printf("Wot %d\n", index);
        //     index++;
        // }
        //
        // line.last = line.state;
        // line.last_button_state = button;
    }
}
