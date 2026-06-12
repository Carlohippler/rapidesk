#pragma once

#include "input_capture_win32.hpp"  // Reuse InputEvent definitions

namespace rapiddesk::input {

    /**
     * Windows input injection via SendInput API.
     * Converts absolute coordinates to normalized [0, 65535] range.
     */
    class InputInjectorWin32 {
    public:
        InputInjectorWin32();
        ~InputInjectorWin32() = default;

        bool initialize();
        void shutdown();

        /** Inject input event into host system. */
        void inject(const InputEvent& event);

        /** Update screen dimensions for coordinate normalization. */
        void set_screen_dimensions(uint32_t width, uint32_t height);

    private:
        uint32_t screen_width_ = 1920;
        uint32_t screen_height_ = 1080;

        void inject_mouse(const MouseEvent& m);
        void inject_keyboard(const KeyboardEvent& k);
    };

} // namespace rapiddesk::input