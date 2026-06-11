// src/input/input_capture_win32.hpp
#pragma once
#include <windows.h>

struct MouseEvent {
    long dx;
    long dy;
    bool left_clicked;
};

struct InputEvent {
    enum { MOUSE, KEYBOARD } type;
    uint64_t timestamp_us;
    MouseEvent mouse;
};

class RawInputCapture {
public:
    void register_client_devices(HWND window_handle);
    void process_window_message(LPARAM lParam);
private:
    void dispatch_input_to_host(const InputEvent& event);
};