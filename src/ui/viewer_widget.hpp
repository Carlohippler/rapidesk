#pragma once

#include <QWidget>
#include <QImage>
#include <functional>
#include <atomic>
#include <vector>
#include <cstdint>

namespace rapiddesk::codec {
    // Stub structure for decoded frame — matches the expected interface
    // When you implement real FFmpeg decoding, replace this with actual struct
    struct DecodedFrame {
        uint32_t width = 0;
        uint32_t height = 0;
        uint64_t timestamp_us = 0;
        std::vector<uint8_t> rgba_data;

        DecodedFrame() = default;
        DecodedFrame(uint32_t w, uint32_t h, const uint8_t* data, size_t len)
            : width(w), height(h) {
            rgba_data.assign(data, data + len);
        }
    };
}

namespace rapiddesk::input {
    enum class InputType : uint8_t {
        MOUSE = 0,
        KEYBOARD = 1
    };

    struct MouseEvent {
        int32_t abs_x = 0;
        int32_t abs_y = 0;
        int32_t dx = 0;
        int32_t dy = 0;
        uint16_t buttons_down = 0;
        uint16_t buttons_up = 0;
        int16_t wheel_delta = 0;
        uint64_t timestamp_us = 0;
    };

    struct KeyboardEvent {
        uint16_t vkey = 0;
        uint16_t scan_code = 0;
        bool is_down = false;
        bool is_extended = false;
    };

    struct InputEvent {
        InputType type = InputType::MOUSE;
        union {
            MouseEvent mouse;
            KeyboardEvent key;
        };

        InputEvent() { mouse = MouseEvent{}; }
    };
}

namespace rapiddesk::ui {

    using InputForwardCallback = std::function<void(const input::InputEvent&)>;

    /**
     * @brief Enterprise-grade remote desktop viewport widget
     *
     * Features:
     * - Hardware-accelerated rendering via QPainter
     * - Aspect ratio preservation with letterboxing
     * - Real-time FPS/latency overlay (auto-hiding)
     * - Input forwarding (mouse, keyboard, wheel)
     * - Smooth scaling with quality adaptation
     * - Focus indication for accessibility
     *
     * NOTE: This is a stub implementation. The actual codec integration
     * will be added in Phase 2 (Video Optimization).
     */
    class ViewerWidget : public QWidget {
        Q_OBJECT

    public:
        explicit ViewerWidget(QWidget* parent = nullptr);
        ~ViewerWidget() override;

        // Prevent copying
        ViewerWidget(const ViewerWidget&) = delete;
        ViewerWidget& operator=(const ViewerWidget&) = delete;

        void set_input_callback(InputForwardCallback cb);
        void set_host_dimensions(uint32_t width, uint32_t height);
        void set_input_enabled(bool enabled);
        void set_show_stats(bool show);
        void set_quality_hint(int quality_percent); // 0-100, affects scaling mode

        void present_frame(const codec::DecodedFrame& frame);

        // Current display metrics
        int current_fps() const { return current_fps_.load(); }
        QSize host_size() const { return QSize(static_cast<int>(host_width_), static_cast<int>(host_height_)); }
        bool is_input_enabled() const { return input_enabled_; }

    signals:
        void frame_presented(uint64_t timestamp_us);
        void input_event_generated(const input::InputEvent& event);
        void mouse_moved(QPoint host_pos);
        void clicked(QPoint host_pos, Qt::MouseButton button);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    private:
        void update_scale_factors();
        QPoint map_to_host(const QPoint& local_pos) const;
        void emit_mouse_event(const QPoint& pos, uint16_t buttons_down,
            uint16_t buttons_up, int16_t wheel = 0);
        void draw_stats_overlay(QPainter& painter);
        void draw_empty_state(QPainter& painter);
        void draw_focus_indicator(QPainter& painter);
        void draw_letterbox(QPainter& painter);

        // Frame data
        QImage current_frame_;
        std::atomic<bool> frame_updated_{ false };
        std::atomic<int> fps_counter_{ 0 };
        std::atomic<int> current_fps_{ 0 };

        // Scaling
        double scale_x_ = 1.0;
        double scale_y_ = 1.0;
        int offset_x_ = 0;
        int offset_y_ = 0;

        // Host dimensions
        uint32_t host_width_ = 1920;
        uint32_t host_height_ = 1080;

        // Input state
        InputForwardCallback input_callback_;
        bool input_enabled_ = false;
        uint16_t last_buttons_ = 0;
        bool has_focus_ = false;
        bool mouse_inside_ = false;

        // Stats overlay
        bool show_stats_ = true;
        int quality_hint_ = 100;
        uint64_t last_frame_timestamp_ = 0;

        // FPS timer
        QTimer* fps_timer_ = nullptr;

        // Visual state
        bool show_empty_state_ = true;
    };

} // namespace rapiddesk::ui