cmake_minimum_required(VERSION 3.25)
project(RapidDesk VERSION 1.0.0 LANGUAGES CXX)

# ===== FORÇAR APENAS A CONFIGURAÇÃO RELEASE =====
set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)

# ===== DESABILITAR VCPKG =====
set(CMAKE_TOOLCHAIN_FILE "" CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.25)
project(RapidDesk VERSION 1.0.0 LANGUAGES CXX)

# ===== DESABILITAR VCPKG =====
set(CMAKE_TOOLCHAIN_FILE "" CACHE STRING "" FORCE)
set(VCPKG_TOOLCHAIN "" CACHE STRING "" FORCE)
set(VCPKG_TARGET_TRIPLET "" CACHE STRING "" FORCE)

# Ou mais agressivo:
set(VCPKG_MANIFEST_MODE OFF)
set(VCPKG_APPLOCAL_DEPS OFF)

# ===== CONAN TOOLCHAIN (PRIMEIRO) =====
if(EXISTS "${CMAKE_SOURCE_DIR}/build/generators/conan_toolchain.cmake")
    include("${CMAKE_SOURCE_DIR}/build/generators/conan_toolchain.cmake")
    message(STATUS "Conan toolchain: ${CMAKE_SOURCE_DIR}/build/generators/conan_toolchain.cmake")
elseif(EXISTS "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
    message(STATUS "Conan toolchain: ${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
else()
    message(FATAL_ERROR "Conan toolchain não encontrado. Execute: conan install . --build=missing")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-specific settings
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(EXISTS "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
endif()

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(OpenSSL REQUIRED)
#find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil swscale)
find_package(Boost REQUIRED COMPONENTS asio)

if(NOT TARGET Boost::asio)
    add_library(Boost::asio ALIAS Boost::boost)
endif()

# Conan integration
if(EXISTS "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
endif()

# NVENC headers (usually in CUDA toolkit or standalone)
# NVENC headers - opcional (apenas para hardware encoding)
find_path(NVENC_INCLUDE_DIR nvEncodeAPI.h
    PATHS 
        "$ENV{CUDA_PATH}/include"
        "$ENV{NVENCODE_SDK_DIR}/Interface"
        "$ENV{CUDA_PATH}/v12.0/include"
        "$ENV{CUDA_PATH}/v11.8/include"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0/include"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.8/include"
    DOC "NVENC API headers"
    NO_DEFAULT_PATH)

if(NOT NVENC_INCLUDE_DIR)
    message(WARNING "NVENC headers nao encontrados. Hardware encoding sera desabilitado.")
    set(NVENC_INCLUDE_DIR "")
endif()

# Source files — Windows only
set(CORE_SOURCES
    src/core/session.cpp
    src/core/logger.cpp
    src/core/config.cpp
)

set(NETWORK_SOURCES
    src/network/signaling_client.cpp
    src/network/ice_transport.cpp
    src/network/media_channel.cpp
    src/network/bitrate_controller.cpp
)

set(CRYPTO_SOURCES
    src/crypto/crypto_session.cpp
    src/crypto/auth.cpp
)

set(CAPTURE_SOURCES
    src/capture/capture_factory.cpp
    src/capture/dxgi_capture.cpp
)

set(CODEC_SOURCES
    src/codec/encoder_factory.cpp
    src/codec/nvenc_encoder.cpp
    #src/codec/ffmpeg_encoder.cpp
    #src/codec/ffmpeg_decoder.cpp
)

set(INPUT_SOURCES
    src/input/input_capture_win32.cpp
    src/input/input_injector_win32.cpp
    src/input/clipboard_sync.cpp
)

set(UI_SOURCES
    src/ui/main.cpp
    src/ui/main_window.cpp
    src/ui/viewer_widget.cpp
)

# Main executable
add_executable(rapiddesk WIN32
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${CRYPTO_SOURCES}
    ${CAPTURE_SOURCES}
    ${CODEC_SOURCES}
    ${INPUT_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(rapiddesk PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

if(NVENC_INCLUDE_DIR)
    target_include_directories(rapiddesk PRIVATE ${NVENC_INCLUDE_DIR})
    target_compile_definitions(rapiddesk PRIVATE RAPIDDESK_NVENC_ENABLED)
endif()

target_link_libraries(rapiddesk PRIVATE
    Qt6::Core
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
    #FFmpeg::avcodec
    #FFmpeg::avutil
    #FFmpeg::swscale
    Boost::boost
    d3d11
    dxgi
    user32
    gdi32
    ws2_32
    winmm
    imm32
    dwmapi
    setupapi
    cfgmgr32
)

# Compile definitions
target_compile_definitions(rapiddesk PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
    RAPIDDESK_VERSION="${PROJECT_VERSION}"
)

# MSVC specific
if(MSVC)
    target_compile_options(rapiddesk PRIVATE
        /W4           # High warning level
        /WX-          # Don't treat warnings as errors (during dev)
        /MP           # Multi-processor compilation
        /permissive-  # Strict conformance
        /Zc:__cplusplus
        /EHsc
    )
    
    # Enable address sanitizer in Debug
    target_compile_options(rapiddesk PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
endif()

# Tests
option(RAPIDDESK_BUILD_TESTS "Build tests" ON)
if(RAPIDDESK_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# Install
install(TARGETS rapiddesk
    RUNTIME DESTINATION bin
)