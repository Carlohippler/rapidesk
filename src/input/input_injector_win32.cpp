#include "input_injector_win32.hpp"
#include <windows.h>

namespace rapiddesk::input {

    InputInjectorWin32::InputInjectorWin32() = default;

    bool InputInjectorWin32::initialize() {
        // Get current screen dimensions
        screen_width_ = GetSystemMetrics(SM_CXSCREEN);
        screen_height_ = GetSystemMetrics(SM_CYSCREEN);
        return true;
    }

    void InputInjectorWin32::shutdown() {
        // Nothing to cleanup
    }

    void InputInjectorWin32::set_screen_dimensions(uint32_t width, uint32_t height) {
        screen_width_ = width;
        screen_height_ = height;
    }

    void InputInjectorWin32::inject(const InputEvent& event) {
        switch (event.type) {
        case InputType::MOUSE:
            inject_mouse(event.mouse);
            break;
        case InputType::KEYBOARD:
            inject_keyboard(event.key);
            break;
        default:
            break;
        }
    }

    void InputInjectorWin32::inject_mouse(const MouseEvent& m) {
        INPUT input[2] = {}; // Support up to 2 events (move + click)
        int num_inputs = 0;

        // Mouse move (absolute coordinates normalized to [0, 65535])
        if (m.abs_x >= 0 && m.abs_y >= 0) {
            input[num_inputs].type = INPUT_MOUSE;
            input[num_inputs].mi.dx = static_cast<LONG>((m.abs_x / static_cast<float>(screen_width_)) * 65535.0f);
            input[num_inputs].mi.dy = static_cast<LONG>((m.abs_y / static_cast<float>(screen_height_)) * 65535.0f);
            input[num_inputs].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
            num_inputs++;
        }

        // Button states
        if (m.buttons_down & 0x01) {
            input[num_inputs].type = INPUT_MOUSE;
            input[num_inputs].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            num_inputs++;
        }
        if (m.buttons_up & 0x01) {
            input[num_inputs].type = INPUT_MOUSE;
            input[num_inputs].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            num_inputs++;
        }
        if (m.buttons_down & 0x02) {
            input[num_inputs].type = INPUT_MOUSE;
            input[num_inputs].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            num_inputs++;
        }
        if (m.buttons_up & 0x02) {
            input[num_inputs].type = INPUT_MOUSE;
            input[num_inputs].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            num_inputs++;
        }

        // Wheel
        if (m.wheel_delta != 0 && num_inputs < 2) {
            input[num_inputs].type = INPUT_MOUSE;
            input[num_inputs].mi.dwFlags = MOUSEEVENTF_WHEEL;
            input[num_inputs].mi.mouseData = m.wheel_delta * WHEEL_DELTA;
            num_inputs++;
        }

        if (num_inputs > 0) {
            SendInput(num_inputs, input, sizeof(INPUT));
        }
    }

    void InputInjectorWin32::inject_keyboard(const KeyboardEvent& k) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = k.vkey;
        input.ki.wScan = k.scan_code;
        input.ki.dwFlags = KEYEVENTF_SCANCODE;

        if (k.is_extended) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        if (!k.is_down) input.ki.dwFlags |= KEYEVENTF_KEYUP;

        SendInput(1, &input, sizeof(INPUT));
    }

} // namespace rapiddesk::input