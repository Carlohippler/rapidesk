// src/input/input_injector_win32.hpp
#pragma once
#include "input_capture_win32.hpp" // Para herdar a struct InputEvent

class Win32InputInjector {
public:
    void inject_event(const InputEvent& event);
};