#pragma once

#include <QWidget>
#include <QImage>
#include <memory>
#include <cstdint>

namespace rapiddesk::codec { struct DecodedFrame; }
namespace rapiddesk::input { struct InputEvent; }

namespace rapiddesk::ui {

    /**
     * Viewer rendering widget — displays decoded frames and forwards input.
     * Uses QImage for CPU rendering (can be upgraded to OpenGL/D3D later).
     */
    class ViewerWidget : public QWidget {
        Q_OBJECT

    public:
        explicit ViewerWidget(QWidget* parent = nullptr);
        ~ViewerWidget() override;

        /** Present a decoded frame for rendering. */
        void present_frame(const codec::DecodedFrame& frame);

        /** Set callback for input events (mouse/keyboard from user). */
        using InputForwardCallback = std::function<void(const input::InputEvent&)>;
        void set_input_callback(InputForwardCallback cb);

        /** Update host screen dimensions for coordinate mapping. */
        void set_host_dimensions(uint32_t width, uint32_t height);

        /** Enable/disable input forwarding. */
        void set_input_enabled(bool enabled);

    signals:
        void frame_presented(uint64_t timestamp_us);
        void input_event_generated(const input::InputEvent& event);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void update_scale_factors();
        QPoint map_to_host(const QPoint& local_pos) const;
        void emit_mouse_event(const QPoint& pos, uint16_t buttons_down,
            uint16_t buttons_up, int16_t wheel = 0);

        QImage current_frame_;
        QImage scaled_frame_;
        bool frame_updated_ = false;

        // Scaling
        double scale_x_ = 1.0;
        double scale_y_ = 1.0;
        int offset_x_ = 0;
        int offset_y_ = 0;

        // Host dimensions for coordinate mapping
        uint32_t host_width_ = 1920;
        uint32_t host_height_ = 1080;

        // Input
        bool input_enabled_ = true;
        InputForwardCallback input_callback_;
        uint16_t last_buttons_ = 0;

        // Performance
        uint64_t last_frame_time_ = 0;
        int fps_counter_ = 0;
        int current_fps_ = 0;
    };

} // namespace rapiddesk::ui