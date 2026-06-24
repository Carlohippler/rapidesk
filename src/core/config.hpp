#pragma once

#include <string>
#include <mutex>

namespace rapiddesk::core {

    class Config {
    public:
        static Config& instance() {
            static Config config;
            return config;
        }

        // Configurações de servidor
        std::string signaling_server() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return signaling_server_;
        }

        void set_signaling_server(const std::string& url) {
            std::lock_guard<std::mutex> lock(mutex_);
            signaling_server_ = url;
        }

        int signaling_port() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return signaling_port_;
        }

        void set_signaling_port(int port) {
            std::lock_guard<std::mutex> lock(mutex_);
            signaling_port_ = port;
        }

        // Configurações de vídeo
        int video_width() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return video_width_;
        }

        int video_height() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return video_height_;
        }

        int video_fps() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return video_fps_;
        }

        int video_bitrate() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return video_bitrate_;
        }

        // Configurações de áudio
        bool audio_enabled() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return audio_enabled_;
        }

        // Configurações de entrada
        bool input_enabled() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return input_enabled_;
        }

        bool clipboard_sync_enabled() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return clipboard_sync_enabled_;
        }

        // Carregar/Salvar (stubs)
        bool load(const std::string& path = "rapiddesk.json") {
            // TODO: implementar carregamento de JSON
            return true;
        }

        bool save(const std::string& path = "rapiddesk.json") {
            // TODO: implementar salvamento de JSON
            return true;
        }

    private:
        Config() = default;
        ~Config() = default;
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        mutable std::mutex mutex_;

        // Defaults
        std::string signaling_server_ = "localhost";
        int signaling_port_ = 8080;
        int video_width_ = 1920;
        int video_height_ = 1080;
        int video_fps_ = 30;
        int video_bitrate_ = 5000000;  // 5 Mbps
        bool audio_enabled_ = true;
        bool input_enabled_ = true;
        bool clipboard_sync_enabled_ = true;
    };

} // namespace rapiddesk::core