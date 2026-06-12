#pragma once

#include <cstdint>
#include <functional>
#include <windows.h>

namespace rapiddesk::input {

    enum class InputType : uint8_t { MOUSE, KEYBOARD, NONE };

    struct MouseEvent {
        int32_t dx;           // Relative movement
        int32_t dy;
        int32_t abs_x;        // Absolute position (if available)
        int32_t abs_y;
        uint16_t buttons_down;
        uint16_t buttons_up;
        int16_t wheel_delta;
        uint64_t timestamp_us;
    };

    struct KeyboardEvent {
        uint16_t vkey;
        uint16_t scan_code;
        bool is_down;
        bool is_extended;
        uint64_t timestamp_us;
    };

    struct InputEvent {
        InputType type;
        union {
            MouseEvent mouse;
            KeyboardEvent key;
        };
    };

    using InputCaptureCallback = std::function<void(const InputEvent&)>;

    /**
     * Windows Raw Input capture — bypasses normal OS processing.
     * Uses RIDEV_INPUTSINK to capture even without window focus.
     */
    class InputCaptureWin32 {
    public:
        InputCaptureWin32();
        ~InputCaptureWin32();

        bool initialize(HWND hwnd);
        void shutdown();

        /** Process WM_INPUT message — call from window proc. */
        void process_raw_input(HRAWINPUT h_raw_input);

        void set_callback(InputCaptureCallback cb);

        bool is_capturing() const noexcept { return capturing_; }

    private:
        HWND hwnd_ = nullptr;
        bool capturing_ = false;
        InputCaptureCallback callback_;

        uint64_t get_microseconds() const;
        void parse_mouse(const RAWMOUSE& raw, InputEvent& event);
        void parse_keyboard(const RAWKEYBOARD& raw, InputEvent& event);
    };

} // namespace rapiddesk::input