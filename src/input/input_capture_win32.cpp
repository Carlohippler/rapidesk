#include "input_capture_win32.hpp"
#include <vector>

namespace rapiddesk::input {

    InputCaptureWin32::InputCaptureWin32() = default;
    InputCaptureWin32::~InputCaptureWin32() { shutdown(); }

    bool InputCaptureWin32::initialize(HWND hwnd) {
        if (capturing_) return true;
        hwnd_ = hwnd;

        RAWINPUTDEVICE devices[2] = {};

        // Mouse
        devices[0].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        devices[0].usUsage = 0x02;     // HID_USAGE_GENERIC_MOUSE
        devices[0].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY;
        devices[0].hwndTarget = hwnd_;

        // Keyboard
        devices[1].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        devices[1].usUsage = 0x06;     // HID_USAGE_GENERIC_KEYBOARD
        devices[1].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY | RIDEV_DEVNOTIFY;
        devices[1].hwndTarget = hwnd_;

        if (!RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE))) {
            return false;
        }

        capturing_ = true;
        return true;
    }

    void InputCaptureWin32::shutdown() {
        if (!capturing_) return;

        RAWINPUTDEVICE devices[2] = {};
        devices[0].usUsagePage = 0x01;
        devices[0].usUsage = 0x02;
        devices[0].dwFlags = RIDEV_REMOVE;
        devices[0].hwndTarget = nullptr;

        devices[1].usUsagePage = 0x01;
        devices[1].usUsage = 0x06;
        devices[1].dwFlags = RIDEV_REMOVE;
        devices[1].hwndTarget = nullptr;

        RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
        capturing_ = false;
        hwnd_ = nullptr;
    }

    void InputCaptureWin32::process_raw_input(HRAWINPUT h_raw_input) {
        if (!callback_) return;

        UINT size = 0;
        GetRawInputData(h_raw_input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size == 0) return;

        std::vector<BYTE> buffer(size);
        if (GetRawInputData(h_raw_input, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
            return;
        }

        auto* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        InputEvent event{};
        event.type = InputType::NONE;

        if (raw->header.dwType == RIM_TYPEMOUSE) {
            parse_mouse(raw->data.mouse, event);
        }
        else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            parse_keyboard(raw->data.keyboard, event);
        }

        if (event.type != InputType::NONE) {
            callback_(event);
        }
    }

    void InputCaptureWin32::parse_mouse(const RAWMOUSE& raw, InputEvent& event) {
        event.type = InputType::MOUSE;
        event.mouse.timestamp_us = get_microseconds();
        event.mouse.dx = raw.lLastX;
        event.mouse.dy = raw.lLastY;
        event.mouse.abs_x = 0; // Will be resolved by viewer widget if needed
        event.mouse.abs_y = 0;
        event.mouse.buttons_down = raw.usButtonFlags & 0xFFFF;
        event.mouse.buttons_up = 0;
        event.mouse.wheel_delta = 0;

        if (raw.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
            event.mouse.buttons_down |= 0x01;
        if (raw.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
            event.mouse.buttons_down |= 0x02;
        if (raw.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
            event.mouse.buttons_down |= 0x04;

        if (raw.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
            event.mouse.buttons_up |= 0x01;
        if (raw.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
            event.mouse.buttons_up |= 0x02;
        if (raw.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
            event.mouse.buttons_up |= 0x04;

        if (raw.usButtonFlags & RI_MOUSE_WHEEL) {
            event.mouse.wheel_delta = static_cast<int16_t>(raw.usButtonData);
        }
    }

    void InputCaptureWin32::parse_keyboard(const RAWKEYBOARD& raw, InputEvent& event) {
        event.type = InputType::KEYBOARD;
        event.key.timestamp_us = get_microseconds();
        event.key.vkey = raw.VKey;
        event.key.scan_code = raw.MakeCode;
        event.key.is_down = !(raw.Flags & RI_KEY_BREAK);
        event.key.is_extended = (raw.Flags & RI_KEY_E0) != 0;
    }

    uint64_t InputCaptureWin32::get_microseconds() const {
        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        return static_cast<uint64_t>(count.QuadPart * 1'000'000 / freq.QuadPart);
    }

    void InputCaptureWin32::set_callback(InputCaptureCallback cb) {
        callback_ = std::move(cb);
    }

} // namespace rapiddesk::input