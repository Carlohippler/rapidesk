#pragma once
#include <cstdint>

namespace rapiddesk::input {

    enum class InputType : uint8_t {
        MOUSE,
        KEYBOARD,
        CLIPBOARD
    };

    struct MouseEvent {
        int x = 0;
        int y = 0;
        int button = 0;   // 0=left, 1=right, 2=middle
        bool pressed = false;
        int delta = 0;    // para wheel
    };

    struct KeyboardEvent {
        uint32_t keycode = 0;
        bool pressed = false;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };

    struct ClipboardMessage {
        std::string text;
    };

    struct InputEvent {
        InputType type = InputType::MOUSE;
        union {
            MouseEvent mouse;
            KeyboardEvent keyboard;
            ClipboardMessage clipboard;
        };

        InputEvent() : mouse{} {}
    };

} // namespace rapiddesk::input