// capture_factory.hpp
#pragma once
#include <memory>

namespace rapiddesk::capture {

    class DXGICapture;

    class CaptureFactory {
    public:
        static std::unique_ptr<DXGICapture> create_capture();
        // Remover: create_x11(), create_pipewire(), create_screencapturekit()
    };

} // namespace rapiddesk::capture