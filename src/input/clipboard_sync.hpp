#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <span>

namespace rapiddesk::input {

    enum class ClipboardMsgType : uint8_t {
        OFFER,    // Host announces available formats
        REQUEST,  // Client requests specific format
        DATA,     // Actual clipboard data
        CLEAR     // Clipboard cleared
    };

    struct ClipboardMessage {
        ClipboardMsgType type;
        std::string format;        // "text/plain", "image/png", etc.
        std::vector<uint8_t> data; // Payload for DATA type
        uint32_t fragment_index;   // For large data > 64KB
        uint32_t fragment_total;
    };

    using ClipboardCallback = std::function<void(const ClipboardMessage&)>;

    /**
     * Bidirectional clipboard synchronization.
     * Host detects changes and offers formats; client requests data.
     */
    class ClipboardSync {
    public:
        ClipboardSync();
        ~ClipboardSync();

        bool initialize(bool is_host);
        void shutdown();

        // Host side: detect local clipboard changes
        void poll_clipboard();  // Call periodically (e.g. 500ms)

        // Client side: request format from host
        void request_format(const std::string& format);

        // Receive message from network
        void on_network_message(const ClipboardMessage& msg);

        void set_send_callback(ClipboardCallback cb);
        void set_receive_callback(ClipboardCallback cb);

    private:
        bool is_host_ = false;
        ClipboardCallback send_cb_;
        ClipboardCallback receive_cb_;

        std::string last_text_hash_;  // Simple change detection
        std::vector<std::string> available_formats_;

        bool detect_clipboard_change();
        void send_offer();
        void send_data(const std::string& format, const std::vector<uint8_t>& data);

#ifdef _WIN32
        void* clipboard_window_ = nullptr; // HWND
#endif
    };

} // namespace rapiddesk::input