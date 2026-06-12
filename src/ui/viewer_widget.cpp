#include "viewer_widget.hpp"
#include "codec/ffmpeg_decoder.hpp"  // For DecodedFrame
#include "input/input_capture_win32.hpp"  // For InputEvent definitions

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>

namespace rapiddesk::ui {

    ViewerWidget::ViewerWidget(QWidget* parent) : QWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAttribute(Qt::WA_OpaquePaintEvent);

        // FPS counter timer
        auto* fps_timer = new QTimer(this);
        connect(fps_timer, &QTimer::timeout, [this]() {
            current_fps_ = fps_counter_;
            fps_counter_ = 0;
            });
        fps_timer->start(1000);
    }

    ViewerWidget::~ViewerWidget() = default;

    void ViewerWidget::present_frame(const codec::DecodedFrame& frame) {
        if (frame.width == 0 || frame.height == 0) return;

        // Convert BGRA to RGBA for QImage if needed
        // QImage::Format_ARGB32 uses BGRA on little-endian, so direct mapping works
        QImage new_frame(
            const_cast<uchar*>(frame.rgba_data.data()),
            static_cast<int>(frame.width),
            static_cast<int>(frame.height),
            static_cast<int>(frame.width * 4),
            QImage::Format_ARGB32);

        // Deep copy since frame data is ephemeral
        current_frame_ = new_frame.copy();
        frame_updated_ = true;
        fps_counter_++;

        update(); // Trigger repaint

        emit frame_presented(frame.timestamp_us);
    }

    void ViewerWidget::paintEvent(QPaintEvent* /*event*/) {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);

        if (current_frame_.isNull()) {
            // Draw placeholder
            painter.setPen(QPen(QColor(80, 80, 80), 1, Qt::DashLine));
            painter.drawRect(rect().adjusted(10, 10, -10, -10));
            painter.setPen(Qt::gray);
            painter.drawText(rect(), Qt::AlignCenter, "Aguardando vídeo...");
            return;
        }

        // Calculate scaled rect maintaining aspect ratio
        update_scale_factors();

        QRect target_rect(offset_x_, offset_y_,
            static_cast<int>(current_frame_.width() * scale_x_),
            static_cast<int>(current_frame_.height() * scale_y_));

        // Fast scaling: use smooth transform only if significantly downscaled
        Qt::TransformationMode mode =
            (scale_x_ < 0.5 || scale_y_ < 0.5) ? Qt::SmoothTransformation : Qt::FastTransformation;

        painter.drawImage(target_rect, current_frame_, current_frame_.rect(), mode);

        // Draw FPS/latency overlay
        painter.setPen(QPen(QColor(0, 255, 0), 1));
        painter.setFont(QFont("Consolas", 9));
        QString stats = QString("FPS: %1 | %2x%3")
            .arg(current_fps_)
            .arg(current_frame_.width())
            .arg(current_frame_.height());
        painter.drawText(10, 20, stats);

        frame_updated_ = false;
    }

    void ViewerWidget::update_scale_factors() {
        if (current_frame_.isNull()) return;

        double widget_aspect = static_cast<double>(width()) / height();
        double frame_aspect = static_cast<double>(current_frame_.width()) / current_frame_.height();

        if (widget_aspect > frame_aspect) {
            // Widget is wider — fit to height
            scale_y_ = static_cast<double>(height()) / current_frame_.height();
            scale_x_ = scale_y_;
        }
        else {
            // Widget is taller — fit to width
            scale_x_ = static_cast<double>(width()) / current_frame_.width();
            scale_y_ = scale_x_;
        }

        offset_x_ = static_cast<int>((width() - current_frame_.width() * scale_x_) / 2);
        offset_y_ = static_cast<int>((height() - current_frame_.height() * scale_y_) / 2);
    }

    QPoint ViewerWidget::map_to_host(const QPoint& local_pos) const {
        int x = static_cast<int>((local_pos.x() - offset_x_) / scale_x_);
        int y = static_cast<int>((local_pos.y() - offset_y_) / scale_y_);

        // Clamp to host dimensions
        x = std::max(0, std::min(static_cast<int>(host_width_) - 1, x));
        y = std::max(0, std::min(static_cast<int>(host_height_) - 1, y));

        return QPoint(x, y);
    }

    void ViewerWidget::emit_mouse_event(const QPoint& pos, uint16_t buttons_down,
        uint16_t buttons_up, int16_t wheel) {
        if (!input_enabled_ || !input_callback_) return;

        input::InputEvent event{};
        event.type = input::InputType::MOUSE;
        event.mouse.abs_x = pos.x();
        event.mouse.abs_y = pos.y();
        event.mouse.dx = 0; // Absolute mode
        event.mouse.dy = 0;
        event.mouse.buttons_down = buttons_down;
        event.mouse.buttons_up = buttons_up;
        event.mouse.wheel_delta = wheel;
        event.mouse.timestamp_us = 0; // Will be set by network layer

        input_callback_(event);
        emit input_event_generated(event);
    }

    void ViewerWidget::mouseMoveEvent(QMouseEvent* event) {
        if (!input_enabled_) return;

        QPoint host_pos = map_to_host(event->pos());
        emit_mouse_event(host_pos, last_buttons_, 0);
    }

    void ViewerWidget::mousePressEvent(QMouseEvent* event) {
        if (!input_enabled_) return;

        uint16_t btn = 0;
        if (event->button() == Qt::LeftButton) btn |= 0x01;
        if (event->button() == Qt::RightButton) btn |= 0x02;
        if (event->button() == Qt::MiddleButton) btn |= 0x04;

        last_buttons_ |= btn;

        QPoint host_pos = map_to_host(event->pos());
        emit_mouse_event(host_pos, btn, 0);
    }

    void ViewerWidget::mouseReleaseEvent(QMouseEvent* event) {
        if (!input_enabled_) return;

        uint16_t btn = 0;
        if (event->button() == Qt::LeftButton) btn |= 0x01;
        if (event->button() == Qt::RightButton) btn |= 0x02;
        if (event->button() == Qt::MiddleButton) btn |= 0x04;

        last_buttons_ &= ~btn;

        QPoint host_pos = map_to_host(event->pos());
        emit_mouse_event(host_pos, 0, btn);
    }

    void ViewerWidget::wheelEvent(QWheelEvent* event) {
        if (!input_enabled_) return;

        QPoint host_pos = map_to_host(event->pos().toPoint());
        int16_t delta = static_cast<int16_t>(event->angleDelta().y() / 120);
        emit_mouse_event(host_pos, last_buttons_, 0, delta);
    }

    void ViewerWidget::keyPressEvent(QKeyEvent* event) {
        if (!input_enabled_ || !input_callback_) return;

        input::InputEvent ev{};
        ev.type = input::InputType::KEYBOARD;
        ev.key.vkey = static_cast<uint16_t>(event->nativeVirtualKey());
        ev.key.scan_code = static_cast<uint16_t>(event->nativeScanCode());
        ev.key.is_down = true;
        ev.key.is_extended = (event->modifiers() & Qt::KeypadModifier) != 0;

        input_callback_(ev);
        emit input_event_generated(ev);
    }

    void ViewerWidget::keyReleaseEvent(QKeyEvent* event) {
        if (!input_enabled_ || !input_callback_) return;

        input::InputEvent ev{};
        ev.type = input::InputType::KEYBOARD;
        ev.key.vkey = static_cast<uint16_t>(event->nativeVirtualKey());
        ev.key.scan_code = static_cast<uint16_t>(event->nativeScanCode());
        ev.key.is_down = false;
        ev.key.is_extended = (event->modifiers() & Qt::KeypadModifier) != 0;

        input_callback_(ev);
        emit input_event_generated(ev);
    }

    void ViewerWidget::resizeEvent(QResizeEvent* /*event*/) {
        update_scale_factors();
    }

    void ViewerWidget::set_input_callback(InputForwardCallback cb) {
        input_callback_ = std::move(cb);
    }

    void ViewerWidget::set_host_dimensions(uint32_t width, uint32_t height) {
        host_width_ = width;
        host_height_ = height;
    }

    void ViewerWidget::set_input_enabled(bool enabled) {
        input_enabled_ = enabled;
        if (!enabled) {
            last_buttons_ = 0;
        }
    }

} // namespace rapiddesk::ui