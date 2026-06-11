// src/capture/dxgi_capture.hpp
#pragma once

class ID3D11Texture2D; // Forward declaration

class DXGICaptureMock {
public:
    ID3D11Texture2D* acquire_next_frame();
    void release_frame();
};