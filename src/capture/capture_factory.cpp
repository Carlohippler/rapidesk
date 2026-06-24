// capture_factory.cpp
#include "capture_factory.hpp"
#include "dxgi_capture.hpp"
#include "capture/dxgi_capture.hpp"

namespace rapiddesk::capture {

    std::unique_ptr<DXGICapture> CaptureFactory::create_capture() {
        auto capture = std::make_unique<DXGICapture>();
        if (!capture->initialize()) {
            return nullptr;
        }
        return capture;
    }

} // namespace rapiddesk::capture