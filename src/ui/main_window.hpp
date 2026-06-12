#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <memory>

namespace rapiddesk::core { class Session; }
namespace rapiddesk::network { class SignalingClient; }
namespace rapiddesk::network { class ICETransport; }

namespace rapiddesk::ui {

    class HostPage;
    class ViewerPage;

    /**
     * Main application window — manages host/viewer modes and session lifecycle.
     */
    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        enum class Mode { HOST, VIEWER, IDLE };

    public slots:
        void start_hosting();
        void stop_hosting();
        void start_viewing(const QString& session_id, const QString& password);
        void disconnect_viewer();
        void on_connection_established();
        void on_connection_error(const QString& error);
        void on_session_id_received(const QString& id);

    signals:
        void mode_changed(Mode mode);
        void latency_updated(double glass_to_glass_ms, double input_ms);

    protected:
        void closeEvent(QCloseEvent* event) override;

    private:
        void setup_ui();
        void setup_tray();
        void create_menu();
        void update_window_title();

        QStackedWidget* stack_ = nullptr;
        HostPage* host_page_ = nullptr;
        ViewerPage* viewer_page_ = nullptr;
        QSystemTrayIcon* tray_icon_ = nullptr;

        std::unique_ptr<core::Session> session_;
        Mode current_mode_ = Mode::IDLE;

        bool close_to_tray_ = true;
    };

} // namespace rapiddesk::ui