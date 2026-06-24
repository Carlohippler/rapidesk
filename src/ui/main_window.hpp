#pragma once
#include <qlayout.h>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QStackedWidget>
#include <QTimer>
#include <memory>

// Forward declarations
class QPushButton;
class QLabel;
class QLineEdit;
class QAction;
class QCloseEvent;
class QResizeEvent;
class QKeyEvent;

namespace rapiddesk::ui {
    class HostPage;
    class ViewerWidget;  // se também der erro
}

namespace rapiddesk::core {
    class Session;
}

namespace rapiddesk::ui {

    // Forward declarations of page widgets
    class NavigationRail;
    class StatusBar;
    class HostPage;
    class ViewerPage;
    class SessionsPage;
    class StatsPage;
    class ToastManager;

    enum class AppMode {
        IDLE,
        HOST,
        VIEWER
    };

    /**
     * @brief Enterprise-grade main window for RapidDesk
     *
     * Features:
     * - Navigation rail with icon+label layout (collapsible)
     * - Persistent status bar with real-time connection metrics
     * - Stacked content pages with smooth transitions
     * - Toast notification system
     * - System tray integration with modern menu
     * - Keyboard shortcut support for power users
     */
    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        // Prevent copying
        MainWindow(const MainWindow&) = delete;
        MainWindow& operator=(const MainWindow&) = delete;

    signals:
        void mode_changed(AppMode mode);
        void latency_updated(double rtt_ms, double glass_ms);
        void connection_quality_changed(int quality_percent); // 0-100

    public slots:
        void show_toast(const QString& message, const QString& type = "info");
        void update_status_bar(const QString& status, const QString& detail = "");

    protected:
        void closeEvent(QCloseEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private slots:
        void on_nav_host_clicked();
        void on_nav_viewer_clicked();
        void on_nav_sessions_clicked();
        void on_nav_stats_clicked();
        void on_nav_settings_clicked();

        void start_hosting();
        void stop_hosting();
        void start_viewing(const QString& session_id, const QString& password);
        void disconnect_viewer();

        void on_session_id_received(const QString& id);
        void on_connection_established();
        void on_connection_error(const QString& error);
        void on_connection_quality_update(int quality);

        void on_tray_activated(QSystemTrayIcon::ActivationReason reason);
        void toggle_fullscreen();
        void show_about_dialog();

    private:
        void setup_ui();
        void setup_styles();
        void setup_navigation();
        void setup_status_bar();
        void setup_tray_icon();
        void setup_shortcuts();
        void setup_toast_system();

        void switch_page(int index);
        void update_window_title();
        void update_status_indicators();
        void collapse_navigation(bool collapsed);

        // Layout components
        QWidget* central_widget_ = nullptr;
        QHBoxLayout* main_layout_ = nullptr;

        NavigationRail* nav_rail_ = nullptr;
        QWidget* content_area_ = nullptr;
        QVBoxLayout* content_layout_ = nullptr;

        StatusBar* status_bar_ = nullptr;
        QStackedWidget* page_stack_ = nullptr;

        // Pages
        HostPage* host_page_ = nullptr;
        ViewerPage* viewer_page_ = nullptr;
        SessionsPage* sessions_page_ = nullptr;
        StatsPage* stats_page_ = nullptr;

        // Toast system
        ToastManager* toast_manager_ = nullptr;

        // System tray
        QSystemTrayIcon* tray_icon_ = nullptr;
        QMenu* tray_menu_ = nullptr;

        // Session
        std::unique_ptr<core::Session> session_;
        AppMode current_mode_ = AppMode::IDLE;

        // State
        bool nav_collapsed_ = false;
        bool close_to_tray_ = true;
        bool is_fullscreen_ = false;
        int current_page_index_ = 0;

        // Metrics
        double current_rtt_ms_ = 0.0;
        double current_glass_ms_ = 0.0;
        int connection_quality_ = 0;
    };

} // namespace rapiddesk::ui