#include "main_window.hpp"
#include "host_page.hpp"
#include "viewer_page.hpp"
#include "core/session.hpp"
#include "core/logger.hpp"
#include "core/config.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QTimer>

namespace rapiddesk::ui {

    // --- HostPage (inline for simplicity, could be separate file) ---

    class HostPage : public QWidget {
        Q_OBJECT
    public:
        explicit HostPage(QWidget* parent = nullptr) : QWidget(parent) {
            auto* layout = new QVBoxLayout(this);
            layout->setSpacing(20);
            layout->setContentsMargins(40, 40, 40, 40);

            auto* title = new QLabel("Modo Host", this);
            title->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
            layout->addWidget(title, 0, Qt::AlignCenter);

            status_label_ = new QLabel("Pronto para receber conexões", this);
            status_label_->setStyleSheet("font-size: 14px; color: #aaa;");
            layout->addWidget(status_label_, 0, Qt::AlignCenter);

            auto* id_layout = new QHBoxLayout();
            id_layout->addStretch();

            auto* id_label = new QLabel("ID da Sessão:", this);
            id_label->setStyleSheet("font-size: 12px; color: #888;");
            id_layout->addWidget(id_label);

            session_id_display_ = new QLineEdit(this);
            session_id_display_->setReadOnly(true);
            session_id_display_->setAlignment(Qt::AlignCenter);
            session_id_display_->setStyleSheet(
                "QLineEdit { font-size: 32px; font-weight: bold; "
                "color: #4CAF50; background: #1e1e1e; border: 2px solid #4CAF50; "
                "border-radius: 8px; padding: 10px; selection-background-color: #4CAF50; }");
            session_id_display_->setFixedWidth(200);
            id_layout->addWidget(session_id_display_);

            auto* copy_btn = new QPushButton("Copiar", this);
            copy_btn->setStyleSheet(
                "QPushButton { background: #333; color: white; border-radius: 4px; "
                "padding: 8px 16px; } QPushButton:hover { background: #444; }");
            connect(copy_btn, &QPushButton::clicked, [this]() {
                session_id_display_->selectAll();
                session_id_display_->copy();
                });
            id_layout->addWidget(copy_btn);
            id_layout->addStretch();

            layout->addLayout(id_layout);

            auto* info = new QLabel(
                "Compartilhe este ID com quem deseja conectar.\n"
                "A conexão é criptografada de ponta a ponta.", this);
            info->setStyleSheet("font-size: 11px; color: #666;");
            info->setAlignment(Qt::AlignCenter);
            layout->addWidget(info);

            stop_btn_ = new QPushButton("Parar Host", this);
            stop_btn_->setStyleSheet(
                "QPushButton { background: #d32f2f; color: white; font-size: 14px; "
                "padding: 12px 32px; border-radius: 6px; } "
                "QPushButton:hover { background: #b71c1c; }");
            layout->addWidget(stop_btn_, 0, Qt::AlignCenter);

            layout->addStretch();

            // Stats
            stats_label_ = new QLabel("Latência: --ms | FPS: --", this);
            stats_label_->setStyleSheet("font-size: 10px; color: #555; font-family: monospace;");
            layout->addWidget(stats_label_, 0, Qt::AlignRight);
        }

        void set_session_id(const QString& id) {
            session_id_display_->setText(id);
        }

        void set_status(const QString& status) {
            status_label_->setText(status);
        }

        void update_stats(double latency_ms, int fps) {
            stats_label_->setText(QString("Latência: %1ms | FPS: %2")
                .arg(latency_ms, 0, 'f', 1).arg(fps));
        }

        QPushButton* stop_button() { return stop_btn_; }

    private:
        QLineEdit* session_id_display_ = nullptr;
        QLabel* status_label_ = nullptr;
        QPushButton* stop_btn_ = nullptr;
        QLabel* stats_label_ = nullptr;
    };

    // --- ViewerPage (inline for simplicity) ---

    class ViewerPage : public QWidget {
        Q_OBJECT
    public:
        explicit ViewerPage(QWidget* parent = nullptr) : QWidget(parent) {
            auto* layout = new QVBoxLayout(this);
            layout->setSpacing(15);
            layout->setContentsMargins(20, 20, 20, 20);

            // Connection bar
            auto* conn_layout = new QHBoxLayout();

            auto* id_label = new QLabel("ID do Host:", this);
            id_label->setStyleSheet("color: #aaa;");
            conn_layout->addWidget(id_label);

            id_input_ = new QLineEdit(this);
            id_input_->setPlaceholderText("000-000-000");
            id_input_->setStyleSheet(
                "QLineEdit { background: #2a2a2a; color: white; border: 1px solid #444; "
                "border-radius: 4px; padding: 8px; font-size: 14px; }");
            id_input_->setFixedWidth(150);
            conn_layout->addWidget(id_input_);

            auto* pass_label = new QLabel("Senha:", this);
            pass_label->setStyleSheet("color: #aaa;");
            conn_layout->addWidget(pass_label);

            pass_input_ = new QLineEdit(this);
            pass_input_->setEchoMode(QLineEdit::Password);
            pass_input_->setStyleSheet(id_input_->styleSheet());
            pass_input_->setFixedWidth(120);
            conn_layout->addWidget(pass_input_);

            connect_btn_ = new QPushButton("Conectar", this);
            connect_btn_->setStyleSheet(
                "QPushButton { background: #4CAF50; color: white; font-weight: bold; "
                "padding: 8px 20px; border-radius: 4px; } "
                "QPushButton:hover { background: #45a049; }");
            conn_layout->addWidget(connect_btn_);

            disconnect_btn_ = new QPushButton("Desconectar", this);
            disconnect_btn_->setStyleSheet(
                "QPushButton { background: #d32f2f; color: white; "
                "padding: 8px 20px; border-radius: 4px; } "
                "QPushButton:hover { background: #b71c1c; }");
            disconnect_btn_->setVisible(false);
            conn_layout->addWidget(disconnect_btn_);

            conn_layout->addStretch();
            layout->addLayout(conn_layout);

            // Viewer widget placeholder
            viewer_widget_ = new QLabel("Conecte a um host para iniciar", this);
            viewer_widget_->setStyleSheet(
                "QLabel { background: #1a1a1a; color: #555; font-size: 16px; "
                "border: 2px dashed #333; border-radius: 8px; }");
            viewer_widget_->setAlignment(Qt::AlignCenter);
            viewer_widget_->setMinimumSize(800, 600);
            layout->addWidget(viewer_widget_, 1);

            // Status bar
            status_label_ = new QLabel("Desconectado", this);
            status_label_->setStyleSheet("color: #888; font-size: 11px;");
            layout->addWidget(status_label_);

            // Latency overlay (top-right corner)
            latency_overlay_ = new QLabel(this);
            latency_overlay_->setStyleSheet(
                "QLabel { background: rgba(0,0,0,180); color: #0f0; "
                "padding: 4px 8px; border-radius: 4px; font-family: monospace; font-size: 11px; }");
            latency_overlay_->setText("RTT: --ms");
            latency_overlay_->setVisible(false);
        }

        void set_connected(bool connected) {
            id_input_->setEnabled(!connected);
            pass_input_->setEnabled(!connected);
            connect_btn_->setVisible(!connected);
            disconnect_btn_->setVisible(connected);
            latency_overlay_->setVisible(connected);

            if (connected) {
                status_label_->setText("Conectado — criptografia ativa (AES-256-GCM)");
                status_label_->setStyleSheet("color: #4CAF50; font-size: 11px;");
                viewer_widget_->setText("Aguardando vídeo...");
            }
            else {
                status_label_->setText("Desconectado");
                status_label_->setStyleSheet("color: #888; font-size: 11px;");
                viewer_widget_->setText("Conecte a um host para iniciar");
                viewer_widget_->setStyleSheet(
                    "QLabel { background: #1a1a1a; color: #555; font-size: 16px; "
                    "border: 2px dashed #333; border-radius: 8px; }");
            }
        }

        void set_viewer_widget(QWidget* widget) {
            // Replace placeholder with actual viewer widget
            // This would be done via layout manipulation
        }

        void update_latency(double rtt_ms, double glass_ms) {
            latency_overlay_->setText(QString("RTT: %1ms | Glass: %2ms")
                .arg(rtt_ms, 0, 'f', 1).arg(glass_ms, 0, 'f', 1));
        }

        QString session_id() const { return id_input_->text().remove('-'); }
        QString password() const { return pass_input_->text(); }

        QPushButton* connect_button() { return connect_btn_; }
        QPushButton* disconnect_button() { return disconnect_btn_; }

    private:
        QLineEdit* id_input_ = nullptr;
        QLineEdit* pass_input_ = nullptr;
        QPushButton* connect_btn_ = nullptr;
        QPushButton* disconnect_btn_ = nullptr;
        QLabel* viewer_widget_ = nullptr;
        QLabel* status_label_ = nullptr;
        QLabel* latency_overlay_ = nullptr;
    };

    // --- MainWindow Implementation ---

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        setup_ui();
        setup_tray();
        create_menu();
        update_window_title();
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::setup_ui() {
        setMinimumSize(900, 700);
        setWindowTitle("RapidDesk");

        auto* central = new QWidget(this);
        setCentralWidget(central);

        auto* main_layout = new QVBoxLayout(central);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);

        // Toolbar
        auto* toolbar = new QHBoxLayout();
        toolbar->setContentsMargins(16, 8, 16, 8);
        toolbar->setSpacing(10);

        auto* logo = new QLabel("◆ RapidDesk", this);
        logo->setStyleSheet("font-size: 18px; font-weight: bold; color: #4CAF50;");
        toolbar->addWidget(logo);

        toolbar->addStretch();

        auto* host_btn = new QPushButton("▶ Host", this);
        host_btn->setStyleSheet(
            "QPushButton { background: #4CAF50; color: white; padding: 6px 16px; "
            "border-radius: 4px; font-weight: bold; }");
        connect(host_btn, &QPushButton::clicked, this, &MainWindow::start_hosting);
        toolbar->addWidget(host_btn);

        auto* viewer_btn = new QPushButton("👁 Viewer", this);
        viewer_btn->setStyleSheet(
            "QPushButton { background: #2196F3; color: white; padding: 6px 16px; "
            "border-radius: 4px; font-weight: bold; }");
        connect(viewer_btn, &QPushButton::clicked, [this]() {
            stack_->setCurrentIndex(1); // Switch to viewer page
            });
        toolbar->addWidget(viewer_btn);

        main_layout->addLayout(toolbar);

        // Separator
        auto* sep = new QFrame(this);
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #333;");
        main_layout->addWidget(sep);

        // Stacked pages
        stack_ = new QStackedWidget(this);
        main_layout->addWidget(stack_, 1);

        host_page_ = new HostPage(stack_);
        viewer_page_ = new ViewerPage(stack_);

        stack_->addWidget(host_page_);
        stack_->addWidget(viewer_page_);

        // Connect host page signals
        connect(host_page_->stop_button(), &QPushButton::clicked,
            this, &MainWindow::stop_hosting);

        // Connect viewer page signals
        connect(viewer_page_->connect_button(), &QPushButton::clicked, [this]() {
            start_viewing(viewer_page_->session_id(), viewer_page_->password());
            });
        connect(viewer_page_->disconnect_button(), &QPushButton::clicked,
            this, &MainWindow::disconnect_viewer);

        // Default to viewer page
        stack_->setCurrentIndex(1);

        // Latency update timer
        auto* stats_timer = new QTimer(this);
        connect(stats_timer, &QTimer::timeout, [this]() {
            // Would query session for actual stats
            emit latency_updated(0.0, 0.0);
            });
        stats_timer->start(1000); // 1s update
    }

    void MainWindow::setup_tray() {
        tray_icon_ = new QSystemTrayIcon(QIcon(":/icons/tray.png"), this);

        auto* tray_menu = new QMenu(this);

        auto* show_action = new QAction("Mostrar", this);
        connect(show_action, &QAction::triggered, this, &QWidget::showNormal);
        tray_menu->addAction(show_action);

        tray_menu->addSeparator();

        auto* quit_action = new QAction("Sair", this);
        connect(quit_action, &QAction::triggered, qApp, &QApplication::quit);
        tray_menu->addAction(quit_action);

        tray_icon_->setContextMenu(tray_menu);
        tray_icon_->setToolTip("RapidDesk — Pronto");
        tray_icon_->show();

        connect(tray_icon_, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick) {
                showNormal();
                raise();
                activateWindow();
            }
            });
    }

    void MainWindow::create_menu() {
        auto* file_menu = menuBar()->addMenu("Arquivo");

        auto* host_action = new QAction("Iniciar Host", this);
        connect(host_action, &QAction::triggered, this, &MainWindow::start_hosting);
        file_menu->addAction(host_action);

        auto* quit_action = new QAction("Sair", this);
        quit_action->setShortcut(QKeySequence::Quit);
        connect(quit_action, &QAction::triggered, qApp, &QApplication::quit);
        file_menu->addAction(quit_action);

        auto* view_menu = menuBar()->addMenu("Visualizar");

        auto* stats_action = new QAction("Mostrar Estatísticas", this);
        stats_action->setCheckable(true);
        stats_action->setChecked(true);
        view_menu->addAction(stats_action);

        auto* help_menu = menuBar()->addMenu("Ajuda");

        auto* about_action = new QAction("Sobre", this);
        connect(about_action, &QAction::triggered, [this]() {
            QMessageBox::about(this, "Sobre RapidDesk",
                "<h2>RapidDesk 1.0</h2>"
                "<p>Software de acesso remoto de alta performance.</p>"
                "<p>Latência glass-to-glass: &lt;20ms (LAN)</p>"
                "<p>Criptografia: X25519 + AES-256-GCM</p>");
            });
        help_menu->addAction(about_action);
    }

    void MainWindow::start_hosting() {
        if (current_mode_ != Mode::IDLE) return;

        try {
            session_ = std::make_unique<core::Session>();
            session_->initialize_host();

            connect(session_.get(), &core::Session::session_id_ready,
                this, &MainWindow::on_session_id_received);
            connect(session_.get(), &core::Session::connection_established,
                this, &MainWindow::on_connection_established);
            connect(session_.get(), &core::Session::connection_error,
                this, &MainWindow::on_connection_error);

            stack_->setCurrentIndex(0); // Host page
            current_mode_ = Mode::HOST;
            emit mode_changed(Mode::HOST);

        }
        catch (const std::exception& e) {
            QMessageBox::critical(this, "Erro",
                QString("Falha ao iniciar host: %1").arg(e.what()));
        }
    }

    void MainWindow::stop_hosting() {
        if (current_mode_ != Mode::HOST) return;

        session_.reset();
        current_mode_ = Mode::IDLE;
        stack_->setCurrentIndex(1); // Back to viewer page
        host_page_->set_session_id("");
        host_page_->set_status("Pronto para receber conexões");
        emit mode_changed(Mode::IDLE);
    }

    void MainWindow::start_viewing(const QString& session_id, const QString& password) {
        if (current_mode_ != Mode::IDLE) return;
        if (session_id.isEmpty()) {
            QMessageBox::warning(this, "Aviso", "Digite o ID da sessão.");
            return;
        }

        try {
            session_ = std::make_unique<core::Session>();
            session_->initialize_viewer(session_id.toStdString(), password.toStdString());

            connect(session_.get(), &core::Session::connection_established,
                this, &MainWindow::on_connection_established);
            connect(session_.get(), &core::Session::connection_error,
                this, &MainWindow::on_connection_error);

            viewer_page_->set_connected(true);
            current_mode_ = Mode::VIEWER;
            emit mode_changed(Mode::VIEWER);

        }
        catch (const std::exception& e) {
            QMessageBox::critical(this, "Erro de Conexão",
                QString("Não foi possível conectar: %1").arg(e.what()));
        }
    }

    void MainWindow::disconnect_viewer() {
        if (current_mode_ != Mode::VIEWER) return;

        session_.reset();
        viewer_page_->set_connected(false);
        current_mode_ = Mode::IDLE;
        emit mode_changed(Mode::IDLE);
    }

    void MainWindow::on_connection_established() {
        if (current_mode_ == Mode::HOST) {
            host_page_->set_status("Cliente conectado — transmitindo");
        }
        else if (current_mode_ == Mode::VIEWER) {
            viewer_page_->set_connected(true);
            // Here we would replace the placeholder with actual ViewerWidget
        }
    }

    void MainWindow::on_connection_error(const QString& error) {
        QMessageBox::critical(this, "Erro de Conexão", error);

        if (current_mode_ == Mode::HOST) {
            stop_hosting();
        }
        else if (current_mode_ == Mode::VIEWER) {
            disconnect_viewer();
        }
    }

    void MainWindow::on_session_id_received(const QString& id) {
        if (host_page_) {
            // Format with dashes for readability: 123-456-789
            QString formatted = id;
            if (id.length() == 9) {
                formatted = id.mid(0, 3) + "-" + id.mid(3, 3) + "-" + id.mid(6, 3);
            }
            host_page_->set_session_id(formatted);
            host_page_->set_status("Aguardando conexão...");
        }
    }

    void MainWindow::update_window_title() {
        QString title = "RapidDesk";
        switch (current_mode_) {
        case Mode::HOST: title += " [HOST]"; break;
        case Mode::VIEWER: title += " [VIEWER]"; break;
        default: break;
        }
        setWindowTitle(title);
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        if (close_to_tray_ && tray_icon_->isVisible()) {
            hide();
            tray_icon_->showMessage("RapidDesk",
                "Executando em segundo plano. Clique duplo para restaurar.",
                QSystemTrayIcon::Information, 3000);
            event->ignore();
        }
        else {
            event->accept();
        }
    }

} // namespace rapiddesk::ui

#include "main_window.moc"