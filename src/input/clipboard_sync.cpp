#include "clipboard_sync.hpp"
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace rapiddesk::input {

    ClipboardSync::ClipboardSync() = default;
    ClipboardSync::~ClipboardSync() { shutdown(); }

    bool ClipboardSync::initialize(bool is_host) {
        is_host_ = is_host;

#ifdef _WIN32
        if (is_host) {
            // Create invisible window for clipboard monitoring
            // Or use AddClipboardFormatListener on Vista+
        }
#endif
        return true;
    }

    void ClipboardSync::shutdown() {
#ifdef _WIN32
        if (clipboard_window_) {
            // Cleanup
            clipboard_window_ = nullptr;
        }
#endif
    }

    void ClipboardSync::poll_clipboard() {
        if (!is_host_) return;

        if (detect_clipboard_change()) {
            send_offer();
        }
    }

    bool ClipboardSync::detect_clipboard_change() {
#ifdef _WIN32
        if (!OpenClipboard(nullptr)) return false;

        bool changed = false;
        if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                // Simple hash comparison would go here
                changed = true;
            }
        }

        CloseClipboard();
        return changed;
#else
        // Linux/macOS stubs
        return false;
#endif
    }

    void ClipboardSync::send_offer() {
        ClipboardMessage msg;
        msg.type = ClipboardMsgType::OFFER;
        msg.format = "text/plain"; // Would enumerate actual formats

        if (send_cb_) send_cb_(msg);
    }

    void ClipboardSync::request_format(const std::string& format) {
        if (is_host_) return; // Only client requests

        ClipboardMessage msg;
        msg.type = ClipboardMsgType::REQUEST;
        msg.format = format;

        if (send_cb_) send_cb_(msg);
    }

    void ClipboardSync::on_network_message(const ClipboardMessage& msg) {
        switch (msg.type) {
        case ClipboardMsgType::OFFER:
            // Client: auto-request text/plain if available
            if (!is_host_ && msg.format.find("text/plain") != std::string::npos) {
                request_format("text/plain");
            }
            break;

        case ClipboardMsgType::REQUEST:
            // Host: send requested data
            if (is_host_) {
#ifdef _WIN32
                if (OpenClipboard(nullptr)) {
                    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                        if (hData) {
                            wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                            if (pszText) {
                                // Convert to UTF-8 and send
                                int utf8_len = WideCharToMultiByte(CP_UTF8, 0, pszText, -1,
                                    nullptr, 0, nullptr, nullptr);
                                std::vector<uint8_t> data(utf8_len);
                                WideCharToMultiByte(CP_UTF8, 0, pszText, -1,
                                    reinterpret_cast<char*>(data.data()), utf8_len, nullptr, nullptr);
                                GlobalUnlock(hData);
                                send_data("text/plain", data);
                            }
                        }
                    }
                    CloseClipboard();
                }
#endif
            }
            break;

        case ClipboardMsgType::DATA:
            if (!is_host_ && receive_cb_) {
                receive_cb_(msg);
            }
            break;

        case ClipboardMsgType::CLEAR:
            // Handle clear
            break;
        }
    }

    void ClipboardSync::send_data(const std::string& format, const std::vector<uint8_t>& data) {
        const size_t MAX_FRAGMENT = 60'000; // Stay under 64KB

        size_t offset = 0;
        uint32_t total = static_cast<uint32_t>((data.size() + MAX_FRAGMENT - 1) / MAX_FRAGMENT);

        for (uint32_t i = 0; i < total; ++i) {
            ClipboardMessage msg;
            msg.type = ClipboardMsgType::DATA;
            msg.format = format;
            msg.fragment_index = i;
            msg.fragment_total = total;

            size_t chunk_size = std::min(MAX_FRAGMENT, data.size() - offset);
            msg.data.assign(data.begin() + offset, data.begin() + offset + chunk_size);
            offset += chunk_size;

            if (send_cb_) send_cb_(msg);
        }
    }

    void ClipboardSync::set_send_callback(ClipboardCallback cb) {
        send_cb_ = std::move(cb);
    }

    void ClipboardSync::set_receive_callback(ClipboardCallback cb) {
        receive_cb_ = std::move(cb);
    }

} // namespace rapiddesk::input