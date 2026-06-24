#include "viewer_widget.hpp"
#include "codec/ffmpeg_decoder.hpp"
#include "input/input_capture_win32.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QFontDatabase>

namespace rapiddesk::ui {

    ViewerWidget::ViewerWidget(QWidget* parent) : QWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAttribute(Qt::WA_OpaquePaintEvent);

        // FPS counter timer
        fps_timer_ = new QTimer(this);
        connect(fps_timer_, &QTimer::timeout, [this]() {
            current_fps_.store(fps_counter_.exchange(0));
            update();
            });
        fps_timer_->start(1000);

        // Set enterprise dark background
        setStyleSheet("background: #0D1117;");
    }

    ViewerWidget::~ViewerWidget() = default;

    void ViewerWidget::present_frame(const codec::DecodedFrame& frame) {
        if (frame.width == 0 || frame.height == 0) return;

        show_empty_state_ = false;

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
        frame_updated_.store(true);
        fps_counter_.fetch_add(1);
        last_frame_timestamp_ = frame.timestamp_us;

        update(); // Trigger repaint

        emit frame_presented(frame.timestamp_us);
    }

    void ViewerWidget::paintEvent(QPaintEvent* /*event*/) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, quality_hint_ < 80);

        // Fill background
        painter.fillRect(rect(), QColor("#0D1117"));

        if (show_empty_state_ || current_frame_.isNull()) {
            draw_empty_state(painter);
            draw_focus_indicator(painter);
            return;
        }

        // Calculate scaled rect maintaining aspect ratio
        update_scale_factors();

        QRect target_rect(offset_x_, offset_y_,
            static_cast<int>(current_frame_.width() * scale_x_),
            static_cast<int>(current_frame_.height() * scale_y_));

        // Draw letterbox areas
        draw_letterbox(painter);

        // Choose scaling mode based on quality hint and scale factor
        Qt::TransformationMode mode = Qt::FastTransformation;
        if (quality_hint_ >= 90 && (scale_x_ < 0.5 || scale_y_ < 0.5)) {
            mode = Qt::SmoothTransformation;
        }

        // Draw frame with optional rounded corners for enterprise feel
        painter.drawImage(target_rect, current_frame_, current_frame_.rect(), mode);

        // Draw subtle border around frame
        painter.setPen(QPen(QColor("#30363D"), 1));
        painter.drawRect(target_rect.adjusted(-1, -1, 0, 0));

        // Stats overlay
        if (show_stats_) {
            draw_stats_overlay(painter);
        }

        // Focus indicator
        draw_focus_indicator(painter);

        frame_updated_.store(false);
    }

    void ViewerWidget::draw_empty_state(QPainter& painter) {
        // Center icon
        painter.setPen(Qt::NoPen);

        // Draw large eye icon placeholder using text
        QFont icon_font("Segoe UI Emoji", 48);
        painter.setFont(icon_font);
        painter.setPen(QColor("#30363D"));

        QRect text_rect = rect();
        text_rect.moveCenter(rect().center() - QPoint(0, 30));
        painter.drawText(text_rect, Qt::AlignCenter, "👁");

        // Title
        QFont title_font("Inter", 16, QFont::DemiBold);
        painter.setFont(title_font);
        painter.setPen(QColor("#8B949E"));

        QRect title_rect = rect();
        title_rect.moveCenter(rect().center() + QPoint(0, 20));
        painter.drawText(title_rect, Qt::AlignCenter, "Conecte-se a um host remoto");

        // Subtitle
        QFont sub_font("Inter", 12);
        painter.setFont(sub_font);
        painter.setPen(QColor("#484F58"));

        QRect sub_rect = rect();
        sub_rect.moveCenter(rect().center() + QPoint(0, 48));
        painter.drawText(sub_rect, Qt::AlignCenter,
            "Digite o ID de sessao e a senha para iniciar");

        // Dashed border box
        painter.setPen(QPen(QColor("#30363D"), 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QRect dash_rect = rect().adjusted(40, 40, -40, -40);
        painter.drawRoundedRect(dash_rect, 8, 8);
    }

    void ViewerWidget::draw_stats_overlay(QPainter& painter) {
        // Semi-transparent background
        QRect overlay_rect(width() - 220, 12, 200, 28);
        painter.fillRect(overlay_rect, QColor(13, 17, 23, 200));

        // Border
        painter.setPen(QPen(QColor("#30363D"), 1));
        painter.drawRoundedRect(overlay_rect, 4, 4);

        // Stats text
        QFont stats_font("JetBrains Mono", 10);
        painter.setFont(stats_font);

        int x = overlay_rect.left() + 10;
        int y = overlay_rect.top() + 18;
        int spacing = 55;

        // FPS
        painter.setPen(QColor("#D29922"));
        painter.drawText(x, y, QString("FPS:%1").arg(current_fps_.load()));
        x += spacing;

        // Resolution
        if (!current_frame_.isNull()) {
            painter.setPen(QColor("#8B949E"));
            painter.drawText(x, y, QString("%1x%2")
                .arg(current_frame_.width())
                .arg(current_frame_.height()));
        }

        // Quality indicator dot
        x = overlay_rect.right() - 14;
        painter.setPen(Qt::NoPen);
        if (quality_hint_ >= 80) {
            painter.setBrush(QColor("#3FB950"));
        }
        else if (quality_hint_ >= 50) {
            painter.setBrush(QColor("#D29922"));
        }
        else {
            painter.setBrush(QColor("#F85149"));
        }
        painter.drawEllipse(x, overlay_rect.top() + 10, 8, 8);
    }

    void ViewerWidget::draw_focus_indicator(QPainter& painter) {
        if (!has_focus_ && !mouse_inside_) return;

        // Subtle focus ring
        painter.setPen(QPen(QColor("#2F81F7"), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }

    void ViewerWidget::draw_letterbox(QPainter& painter) {
        if (current_frame_.isNull()) return;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#0D1117"));

        // Top/bottom bars
        if (offset_y_ > 0) {
            painter.drawRect(0, 0, width(), offset_y_);
            painter.drawRect(0, height() - offset_y_, width(), offset_y_);
        }

        // Left/right bars
        if (offset_x_ > 0) {
            painter.drawRect(0, 0, offset_x_, height());
            painter.drawRect(width() - offset_x_, 0, offset_x_, height());
        }
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

        // Clamp to reasonable limits
        scale_x_ = std::min(scale_x_, 3.0);
        scale_y_ = std::min(scale_y_, 3.0);

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
        emit mouse_moved(host_pos);
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
        emit clicked(host_pos, event->button());
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

        // Handle fullscreen toggle
        if (event->key() == Qt::Key_F11) {
            event->ignore(); // Let parent handle
            return;
        }

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

    void ViewerWidget::enterEvent(QEnterEvent* /*event*/) {
        mouse_inside_ = true;
        update();
    }

    void ViewerWidget::leaveEvent(QEvent* /*event*/) {
        mouse_inside_ = false;
        update();
    }

    void ViewerWidget::focusInEvent(QFocusEvent* /*event*/) {
        has_focus_ = true;
        update();
    }

    void ViewerWidget::focusOutEvent(QFocusEvent* /*event*/) {
        has_focus_ = false;
        // Release all buttons on focus loss
        if (last_buttons_ != 0) {
            emit_mouse_event(QPoint(0, 0), 0, last_buttons_);
            last_buttons_ = 0;
        }
        update();
    }

    void ViewerWidget::set_input_callback(InputForwardCallback cb) {
        input_callback_ = std::move(cb);
    }

    void ViewerWidget::set_host_dimensions(uint32_t width, uint32_t height) {
        host_width_ = width;
        host_height_ = height;
        update_scale_factors();
        update();
    }

    void ViewerWidget::set_input_enabled(bool enabled) {
        input_enabled_ = enabled;
        if (!enabled) {
            last_buttons_ = 0;
        }
        setCursor(enabled ? Qt::ArrowCursor : Qt::ForbiddenCursor);
        update();
    }

    void ViewerWidget::set_show_stats(bool show) {
        show_stats_ = show;
        update();
    }

    void ViewerWidget::set_quality_hint(int quality_percent) {
        quality_hint_ = std::clamp(quality_percent, 0, 100);
    }

} // namespace rapiddesk::ui