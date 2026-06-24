#pragma once
#include <functional>

namespace rapiddesk::network {

    // Remover forward decl de NVENCEncoderMock se existir
    // Não precisa de nada relacionado a NVENC aqui

    using BitrateChangedCallback = std::function<void(int bitrate)>;

    class BitrateController {
    public:
        BitrateController() = default;
        ~BitrateController() = default;

        void set_on_bitrate_changed(BitrateChangedCallback cb) {
            on_bitrate_changed_ = std::move(cb);
        }

        void update_network_stats(int rtt_ms, float packet_loss) {
            // TODO: implementar controle de bitrate adaptativo
            if (on_bitrate_changed_) {
                // Exemplo: reduzir bitrate se perda > 5%
                int new_bitrate = 5000000;  // 5 Mbps default
                if (packet_loss > 0.05f) {
                    new_bitrate = static_cast<int>(new_bitrate * 0.8f);
                }
                on_bitrate_changed_(new_bitrate);
            }
        }

    private:
        BitrateChangedCallback on_bitrate_changed_;
    };

} // namespace rapiddesk::network