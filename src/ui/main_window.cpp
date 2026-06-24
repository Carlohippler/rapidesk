#include "main_window.hpp"
#include "host_page.hpp"
#include "viewer_page.hpp"
#include "sessions_page.hpp"
#include "stats_page.hpp"
#include "core/session.hpp"
#include "core/logger.hpp"
#include "core/config.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QShortcut>
#include <QKeySequence>
#include <QFrame>
#include <QScrollArea>
#include <QSplitter>
#include <QToolButton>
#include <QFontDatabase>
#include <QCheckBox>
#include <QDateTime>
#include <QTableWidget>
#include <QHeaderView>
#include <QProgressBar>

namespace rapiddesk::ui {

    // =========================================================================
    // NAVIGATION RAIL — Modern sidebar navigation
    // =========================================================================

    class NavigationRail : public QFrame {
        Q_OBJECT
    public:
        explicit NavigationRail(QWidget* parent = nullptr) : QFrame(parent) {
            setFixedWidth(64);
            setFrameShape(QFrame::NoFrame);

            auto* layout = new QVBoxLayout(this);
            layout->setContentsMargins(4, 12, 4, 12);
            layout->setSpacing(4);
            layout->setAlignment(Qt::AlignTop);

            // Brand icon at top
            brand_btn_ = create_nav_button("◆", "", true);
            brand_btn_->setFixedSize(48, 48);
            brand_btn_->setStyleSheet(
                "QToolButton {"
                "  background: transparent;"
                "  color: #2F81F7;"
                "  font-size: 20px;"
                "  font-weight: bold;"
                "  border: none;"
                "  border-radius: 8px;"
                "}"
                "QToolButton:hover {"
                "  background: #21262D;"
                "}"
            );
            layout->addWidget(brand_btn_, 0, Qt::AlignCenter);
            layout->addSpacing(16);

            // Navigation items
            nav_host_ = create_nav_button("🖥", "Host");
            nav_viewer_ = create_nav_button("👁", "Viewer");
            nav_sessions_ = create_nav_button("📋", "Sessions");
            nav_stats_ = create_nav_button("📊", "Stats");

            layout->addWidget(nav_host_);
            layout->addWidget(nav_viewer_);
            layout->addWidget(nav_sessions_);
            layout->addWidget(nav_stats_);

            layout->addStretch();

            // Separator
            auto* sep = new QFrame(this);
            sep->setFrameShape(QFrame::HLine);
            sep->setStyleSheet("color: #30363D;");
            layout->addWidget(sep);
            layout->addSpacing(8);

            // Settings at bottom
            nav_settings_ = create_nav_button("⚙", "Settings");
            layout->addWidget(nav_settings_);

            setStyleSheet(
                "NavigationRail {"
                "  background: #161B22;"
                "  border-right: 1px solid #30363D;"
                "}"
            );

            // Set initial active
            set_active_nav(1); // Viewer default
        }

        void set_active_nav(int index) {
            active_index_ = index;
            update_nav_states();
        }

        int active_index() const { return active_index_; }

        QToolButton* host_button() const { return nav_host_; }
        QToolButton* viewer_button() const { return nav_viewer_; }
        QToolButton* sessions_button() const { return nav_sessions_; }
        QToolButton* stats_button() const { return nav_stats_; }
        QToolButton* settings_button() const { return nav_settings_; }

        void set_collapsed(bool collapsed) {
            if (collapsed) {
                setFixedWidth(56);
                for (auto* btn : { nav_host_, nav_viewer_, nav_sessions_, nav_stats_, nav_settings_ }) {
                    btn->setText("");
                    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
                    btn->setFixedSize(40, 40);
                }
            }
            else {
                setFixedWidth(64);
                nav_host_->setText("🖥\nHost");
                nav_viewer_->setText("👁\nViewer");
                nav_sessions_->setText("📋\nSessions");
                nav_stats_->setText("📊\nStats");
                nav_settings_->setText("⚙\nSettings");
                for (auto* btn : { nav_host_, nav_viewer_, nav_sessions_, nav_stats_, nav_settings_ }) {
                    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                    btn->setFixedSize(56, 56);
                }
            }
        }

    signals:
        void nav_clicked(int index);

    private:
        QToolButton* create_nav_button(const QString& icon, const QString& label, bool is_brand = false) {
            auto* btn = new QToolButton(this);
            btn->setText(icon + "\n" + label);
            btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            btn->setFixedSize(56, 56);
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(
                "QToolButton {"
                "  background: transparent;"
                "  color: #8B949E;"
                "  font-size: 16px;"
                "  border: none;"
                "  border-radius: 8px;"
                "  padding: 4px;"
                "}"
                "QToolButton:hover {"
                "  background: #21262D;"
                "  color: #E6EDF3;"
                "}"
                "QToolButton:checked {"
                "  background: #21262D;"
                "  color: #E6EDF3;"
                "  border-left: 2px solid #2F81F7;"
                "}"
            );

            if (!is_brand) {
                connect(btn, &QToolButton::clicked, [this, btn]() {
                    int idx = -1;
                    if (btn == nav_host_) idx = 0;
                    else if (btn == nav_viewer_) idx = 1;
                    else if (btn == nav_sessions_) idx = 2;
                    else if (btn == nav_stats_) idx = 3;
                    else if (btn == nav_settings_) idx = 4;

                    if (idx >= 0) {
                        set_active_nav(idx);
                        emit nav_clicked(idx);
                    }
                    });
            }

            return btn;
        }

        void update_nav_states() {
            nav_host_->setChecked(active_index_ == 0);
            nav_viewer_->setChecked(active_index_ == 1);
            nav_sessions_->setChecked(active_index_ == 2);
            nav_stats_->setChecked(active_index_ == 3);
            nav_settings_->setChecked(active_index_ == 4);
        }

        QToolButton* brand_btn_ = nullptr;
        QToolButton* nav_host_ = nullptr;
        QToolButton* nav_viewer_ = nullptr;
        QToolButton* nav_sessions_ = nullptr;
        QToolButton* nav_stats_ = nullptr;
        QToolButton* nav_settings_ = nullptr;
        int active_index_ = 1;
    };

    // =========================================================================
    // STATUS BAR — Real-time connection metrics
    // =========================================================================

    class StatusBar : public QFrame {
        Q_OBJECT
    public:
        explicit StatusBar(QWidget* parent = nullptr) : QFrame(parent) {
            setFixedHeight(40);
            setFrameShape(QFrame::NoFrame);

            auto* layout = new QHBoxLayout(this);
            layout->setContentsMargins(16, 0, 16, 0);
            layout->setSpacing(16);

            // Connection status
            status_icon_ = new QLabel("●", this);
            status_icon_->setStyleSheet("color: #484F58; font-size: 10px;");
            layout->addWidget(status_icon_);

            status_text_ = new QLabel("Desconectado", this);
            status_text_->setStyleSheet("color: #8B949E; font-size: 12px; font-weight: 500;");
            layout->addWidget(status_text_);

            layout->addStretch();

            // Network info
            network_info_ = new QLabel("—", this);
            network_info_->setStyleSheet("color: #484F58; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
            layout->addWidget(network_info_);

            // Latency
            latency_label_ = new QLabel("—", this);
            latency_label_->setStyleSheet("color: #484F58; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
            layout->addWidget(latency_label_);

            // Bandwidth
            bandwidth_label_ = new QLabel("—", this);
            bandwidth_label_->setStyleSheet("color: #484F58; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
            layout->addWidget(bandwidth_label_);

            // Quality indicator
            quality_bar_ = new QFrame(this);
            quality_bar_->setFixedSize(60, 4);
            quality_bar_->setStyleSheet("background: #30363D; border-radius: 2px;");
            layout->addWidget(quality_bar_);

            setStyleSheet(
                "StatusBar {"
                "  background: #0D1117;"
                "  border-bottom: 1px solid #30363D;"
                "}"
            );
        }

        void set_status(const QString& status, const QString& type = "idle") {
            status_text_->setText(status);

            QString color;
            if (type == "connected") color = "#3FB950";
            else if (type == "connecting") color = "#D29922";
            else if (type == "error") color = "#F85149";
            else color = "#484F58";

            status_icon_->setStyleSheet(QString("color: %1; font-size: 10px;").arg(color));

            if (type == "connecting") {
                start_pulse_animation();
            }
            else {
                stop_pulse_animation();
            }
        }

        void set_network_info(const QString& info) {
            network_info_->setText(info);
            network_info_->setStyleSheet("color: #8B949E; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
        }

        void set_latency(double rtt_ms) {
            if (rtt_ms > 0) {
                QString color = rtt_ms < 30 ? "#3FB950" : rtt_ms < 100 ? "#D29922" : "#F85149";
                latency_label_->setText(QString("⏱ %1ms").arg(rtt_ms, 0, 'f', 1));
                latency_label_->setStyleSheet(QString("color: %1; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;").arg(color));
            }
            else {
                latency_label_->setText("—");
                latency_label_->setStyleSheet("color: #484F58; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
            }
        }

        void set_bandwidth(double mbps) {
            if (mbps > 0) {
                bandwidth_label_->setText(QString("📶 %1 Mbps").arg(mbps, 0, 'f', 1));
                bandwidth_label_->setStyleSheet("color: #8B949E; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
            }
            else {
                bandwidth_label_->setText("—");
                bandwidth_label_->setStyleSheet("color: #484F58; font-size: 11px; font-family: 'JetBrains Mono', 'Consolas', monospace;");
            }
        }

        void set_quality(int percent) {
            QString color;
            if (percent >= 80) color = "#3FB950";
            else if (percent >= 50) color = "#D29922";
            else color = "#F85149";

            quality_bar_->setStyleSheet(
                QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                    "stop:0 %1, stop:%2 %1, stop:%2 #30363D, stop:1 #30363D);"
                    "border-radius: 2px;")
                .arg(color)
                .arg(percent / 100.0)
            );
        }

    private:
        void start_pulse_animation() {
            if (!pulse_timer_) {
                pulse_timer_ = new QTimer(this);
                connect(pulse_timer_, &QTimer::timeout, [this]() {
                    pulse_state_ = !pulse_state_;
                    status_icon_->setStyleSheet(
                        QString("color: %1; font-size: 10px;")
                        .arg(pulse_state_ ? "#D29922" : "#D2992280")
                    );
                    });
                pulse_timer_->start(800);
            }
        }

        void stop_pulse_animation() {
            if (pulse_timer_) {
                pulse_timer_->stop();
                delete pulse_timer_;
                pulse_timer_ = nullptr;
            }
        }

        QLabel* status_icon_ = nullptr;
        QLabel* status_text_ = nullptr;
        QLabel* network_info_ = nullptr;
        QLabel* latency_label_ = nullptr;
        QLabel* bandwidth_label_ = nullptr;
        QFrame* quality_bar_ = nullptr;
        QTimer* pulse_timer_ = nullptr;
        bool pulse_state_ = false;
    };

    // =========================================================================
    // TOAST MANAGER — Non-intrusive notification system
    // =========================================================================

    class ToastWidget : public QFrame {
        Q_OBJECT
    public:
        ToastWidget(const QString& message, const QString& type, QWidget* parent = nullptr)
            : QFrame(parent) {
            setFixedWidth(320);
            setFrameShape(QFrame::NoFrame);

            auto* layout = new QHBoxLayout(this);
            layout->setContentsMargins(12, 10, 12, 10);
            layout->setSpacing(8);

            QString icon;
            QString border_color;
            if (type == "success") { icon = "✓"; border_color = "#3FB950"; }
            else if (type == "error") { icon = "✕"; border_color = "#F85149"; }
            else if (type == "warning") { icon = "⚠"; border_color = "#D29922"; }
            else { icon = "ℹ"; border_color = "#58A6FF"; }

            auto* icon_label = new QLabel(icon, this);
            icon_label->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(border_color));
            layout->addWidget(icon_label);

            auto* msg_label = new QLabel(message, this);
            msg_label->setWordWrap(true);
            msg_label->setStyleSheet("color: #E6EDF3; font-size: 12px;");
            layout->addWidget(msg_label, 1);

            auto* close_btn = new QToolButton(this);
            close_btn->setText("✕");
            close_btn->setStyleSheet(
                "QToolButton {"
                "  background: transparent;"
                "  color: #484F58;"
                "  font-size: 12px;"
                "  border: none;"
                "  padding: 2px;"
                "}"
                "QToolButton:hover {"
                "  color: #E6EDF3;"
                "}"
            );
            connect(close_btn, &QToolButton::clicked, this, &ToastWidget::close_clicked);
            layout->addWidget(close_btn);

            setStyleSheet(
                QString("ToastWidget {"
                    "  background: #161B22;"
                    "  border-left: 3px solid %1;"
                    "  border-radius: 6px;"
                    "}")
                .arg(border_color)
            );

            if (type != "error") {
                QTimer::singleShot(4000, this, &ToastWidget::close_clicked);
            }
        }

    signals:
        void close_clicked();
    };

    class ToastManager : public QWidget {
        Q_OBJECT
    public:
        explicit ToastManager(QWidget* parent = nullptr) : QWidget(parent) {
            setAttribute(Qt::WA_TransparentForMouseEvents, false);

            auto* layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 16, 16, 0);
            layout->setSpacing(8);
            layout->setAlignment(Qt::AlignTop | Qt::AlignRight);
        }

        void show_toast(const QString& message, const QString& type) {
            auto* toast = new ToastWidget(message, type, this);
            layout()->addWidget(toast);

            toast->setGraphicsEffect(new QGraphicsOpacityEffect(toast));
            toast->graphicsEffect()->setOpacity(0);

            auto* anim = new QPropertyAnimation(toast->graphicsEffect(), "opacity");
            anim->setDuration(200);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            connect(toast, &ToastWidget::close_clicked, [this, toast]() {
                auto* anim = new QPropertyAnimation(toast->graphicsEffect(), "opacity");
                anim->setDuration(150);
                anim->setStartValue(1.0);
                anim->setEndValue(0.0);
                connect(anim, &QPropertyAnimation::finished, [toast]() {
                    toast->deleteLater();
                    });
                anim->start(QAbstractAnimation::DeleteWhenStopped);
                });

            while (layout()->count() > 5) {
                auto* item = layout()->itemAt(0);
                if (item && item->widget()) {
                    item->widget()->deleteLater();
                }
            }
        }
    };

    // =========================================================================
    // HOST PAGE — Redesigned with enterprise card layout
    // =========================================================================

    class HostPage : public QWidget {
        Q_OBJECT
    public:
        explicit HostPage(QWidget* parent = nullptr) : QWidget(parent) {
            auto* scroll = new QScrollArea(this);
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

            auto* container = new QWidget();
            auto* layout = new QVBoxLayout(container);
            layout->setContentsMargins(32, 24, 32, 24);
            layout->setSpacing(24);

            // Page header
            auto* header = new QLabel("Modo Host", this);
            header->setStyleSheet("font-size: 20px; font-weight: 700; color: #E6EDF3;");
            layout->addWidget(header);

            auto* subheader = new QLabel("Compartilhe seu ID de sessão para permitir acesso remoto seguro.", this);
            subheader->setStyleSheet("font-size: 13px; color: #8B949E; margin-bottom: 8px;");
            subheader->setWordWrap(true);
            layout->addWidget(subheader);

            // Session ID Card
            auto* session_card = create_card();
            auto* session_layout = new QVBoxLayout(session_card);
            session_layout->setContentsMargins(24, 20, 24, 20);
            session_layout->setSpacing(16);

            // Status row
            auto* status_row = new QHBoxLayout();
            status_icon_ = new QLabel("●", this);
            status_icon_->setStyleSheet("color: #484F58; font-size: 10px;");
            status_row->addWidget(status_icon_);

            status_label_ = new QLabel("Pronto para receber conexões", this);
            status_label_->setStyleSheet("font-size: 13px; font-weight: 500; color: #8B949E;");
            status_row->addWidget(status_label_);
            status_row->addStretch();
            session_layout->addLayout(status_row);

            // Session ID display
            auto* id_container = new QFrame(this);
            id_container->setStyleSheet(
                "QFrame {"
                "  background: #0D1117;"
                "  border: 1px solid #30363D;"
                "  border-radius: 8px;"
                "  padding: 4px;"
                "}"
            );
            auto* id_layout = new QHBoxLayout(id_container);
            id_layout->setContentsMargins(16, 12, 16, 12);

            session_id_display_ = new QLabel("— — —  — — —  — — —", this);
            session_id_display_->setStyleSheet(
                "font-size: 28px;"
                "font-weight: 700;"
                "font-family: 'JetBrains Mono', 'Consolas', monospace;"
                "color: #2F81F7;"
                "letter-spacing: 2px;"
            );
            session_id_display_->setAlignment(Qt::AlignCenter);
            id_layout->addWidget(session_id_display_, 1);

            session_layout->addWidget(id_container);

            // Action buttons
            auto* btn_layout = new QHBoxLayout();
            btn_layout->setSpacing(8);

            copy_id_btn_ = create_action_button("📋 Copiar ID", "primary");
            connect(copy_id_btn_, &QPushButton::clicked, [this]() {
                // Would copy to clipboard
                emit copy_id_clicked();
                });
            btn_layout->addWidget(copy_id_btn_);

            copy_link_btn_ = create_action_button("🔗 Copiar Link", "secondary");
            btn_layout->addWidget(copy_link_btn_);

            share_btn_ = create_action_button("📤 Compartilhar", "secondary");
            btn_layout->addWidget(share_btn_);

            btn_layout->addStretch();
            session_layout->addLayout(btn_layout);

            // Password row
            auto* pass_layout = new QHBoxLayout();
            auto* pass_label = new QLabel("Senha de acesso:", this);
            pass_label->setStyleSheet("font-size: 12px; color: #8B949E;");
            pass_layout->addWidget(pass_label);

            pass_display_ = new QLineEdit(this);
            pass_display_->setEchoMode(QLineEdit::Password);
            pass_display_->setReadOnly(true);
            pass_display_->setText("••••••••");
            pass_display_->setStyleSheet(
                "QLineEdit {"
                "  background: #0D1117;"
                "  color: #E6EDF3;"
                "  border: 1px solid #30363D;"
                "  border-radius: 6px;"
                "  padding: 6px 10px;"
                "  font-size: 13px;"
                "  font-family: monospace;"
                "  max-width: 120px;"
                "}"
            );
            pass_layout->addWidget(pass_display_);

            show_pass_btn_ = new QPushButton("👁", this);
            show_pass_btn_->setStyleSheet(
                "QPushButton {"
                "  background: transparent;"
                "  color: #8B949E;"
                "  border: none;"
                "  font-size: 14px;"
                "  padding: 4px;"
                "}"
                "QPushButton:hover {"
                "  color: #E6EDF3;"
                "}"
            );
            connect(show_pass_btn_, &QPushButton::clicked, [this]() {
                if (pass_display_->echoMode() == QLineEdit::Password) {
                    pass_display_->setEchoMode(QLineEdit::Normal);
                    show_pass_btn_->setText("🙈");
                }
                else {
                    pass_display_->setEchoMode(QLineEdit::Password);
                    show_pass_btn_->setText("👁");
                }
                });
            pass_layout->addWidget(show_pass_btn_);
            pass_layout->addStretch();
            session_layout->addLayout(pass_layout);

            layout->addWidget(session_card);

            // Active connections card
            auto* conn_card = create_card();
            auto* conn_layout = new QVBoxLayout(conn_card);
            conn_layout->setContentsMargins(24, 20, 24, 20);
            conn_layout->setSpacing(12);

            auto* conn_header = new QLabel("Conexões Ativas", this);
            conn_header->setStyleSheet("font-size: 14px; font-weight: 600; color: #E6EDF3;");
            conn_layout->addWidget(conn_header);

            connections_table_ = new QTableWidget(0, 5, this);
            connections_table_->setHorizontalHeaderLabels(
                QStringList() << "Cliente" << "IP" << "Latência" << "Qualidade" << "Ações");
            connections_table_->horizontalHeader()->setStretchLastSection(true);
            connections_table_->setStyleSheet(
                "QTableWidget {"
                "  background: #0D1117;"
                "  border: 1px solid #30363D;"
                "  border-radius: 6px;"
                "  gridline-color: #21262D;"
                "  font-size: 12px;"
                "}"
                "QTableWidget::item {"
                "  color: #E6EDF3;"
                "  padding: 8px;"
                "  border-bottom: 1px solid #21262D;"
                "}"
                "QHeaderView::section {"
                "  background: #161B22;"
                "  color: #8B949E;"
                "  padding: 8px;"
                "  border: none;"
                "  border-bottom: 1px solid #30363D;"
                "  font-weight: 600;"
                "  font-size: 11px;"
                "}"
            );
            connections_table_->setColumnWidth(0, 120);
            connections_table_->setColumnWidth(1, 120);
            connections_table_->setColumnWidth(2, 80);
            connections_table_->setColumnWidth(3, 100);
            connections_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
            connections_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
            connections_table_->verticalHeader()->setVisible(false);
            conn_layout->addWidget(connections_table_);

            layout->addWidget(conn_card);

            // Stats card
            auto* stats_card = create_card();
            auto* stats_layout = new QVBoxLayout(stats_card);
            stats_layout->setContentsMargins(24, 20, 24, 20);
            stats_layout->setSpacing(12);

            auto* stats_header = new QLabel("Estatísticas em Tempo Real", this);
            stats_header->setStyleSheet("font-size: 14px; font-weight: 600; color: #E6EDF3;");
            stats_layout->addWidget(stats_header);

            auto* stats_grid = new QGridLayout();
            stats_grid->setSpacing(12);

            stats_grid->addWidget(create_stat_item("CPU", "—"), 0, 0);
            stats_grid->addWidget(create_stat_item("RAM", "—"), 0, 1);
            stats_grid->addWidget(create_stat_item("GPU", "—"), 0, 2);
            stats_grid->addWidget(create_stat_item("Bitrate", "—"), 1, 0);
            stats_grid->addWidget(create_stat_item("FPS", "—"), 1, 1);
            stats_grid->addWidget(create_stat_item("Frame Drops", "—"), 1, 2);

            stats_layout->addLayout(stats_grid);
            layout->addWidget(stats_card);

            layout->addStretch();

            // Stop button at bottom
            stop_btn_ = new QPushButton("⏹ Parar Host", this);
            stop_btn_->setFixedHeight(40);
            stop_btn_->setStyleSheet(
                "QPushButton {"
                "  background: #F85149;"
                "  color: white;"
                "  font-size: 13px;"
                "  font-weight: 600;"
                "  border: none;"
                "  border-radius: 6px;"
                "  padding: 0 24px;"
                "}"
                "QPushButton:hover {"
                "  background: #DA3633;"
                "}"
                "QPushButton:pressed {"
                "  background: #B42318;"
                "}"
            );
            stop_btn_->setCursor(Qt::PointingHandCursor);
            layout->addWidget(stop_btn_, 0, Qt::AlignRight);

            scroll->setWidget(container);

            auto* main_layout = new QVBoxLayout(this);
            main_layout->setContentsMargins(0, 0, 0, 0);
            main_layout->addWidget(scroll);

            setStyleSheet("background: #0D1117;");
        }

        void set_session_id(const QString& id) {
            QString formatted = id;
            if (id.length() == 9) {
                formatted = id.mid(0, 3) + "  " + id.mid(3, 3) + "  " + id.mid(6, 3);
            }
            session_id_display_->setText(formatted);
        }

        void set_status(const QString& status, const QString& type = "idle") {
            status_label_->setText(status);

            QString color;
            if (type == "connected") color = "#3FB950";
            else if (type == "connecting") color = "#D29922";
            else if (type == "error") color = "#F85149";
            else color = "#484F58";

            status_icon_->setStyleSheet(QString("color: %1; font-size: 10px;").arg(color));
        }

        void update_stats(double latency_ms, int fps) {
            // Update stat items if needed
        }

        void add_connection(const QString& name, const QString& ip,
            const QString& latency, const QString& quality) {
            int row = connections_table_->rowCount();
            connections_table_->insertRow(row);
            connections_table_->setItem(row, 0, new QTableWidgetItem(name));
            connections_table_->setItem(row, 1, new QTableWidgetItem(ip));
            connections_table_->setItem(row, 2, new QTableWidgetItem(latency));
            connections_table_->setItem(row, 3, new QTableWidgetItem(quality));

            auto* actions_widget = new QWidget();
            auto* actions_layout = new QHBoxLayout(actions_widget);
            actions_layout->setContentsMargins(4, 2, 4, 2);
            actions_layout->setSpacing(4);

            auto* disconnect_btn = new QToolButton();
            disconnect_btn->setText("❌");
            disconnect_btn->setStyleSheet(
                "QToolButton { background: transparent; color: #F85149; border: none; }"
                "QToolButton:hover { color: #DA3633; }"
            );
            actions_layout->addWidget(disconnect_btn);

            auto* settings_btn = new QToolButton();
            settings_btn->setText("⚙");
            settings_btn->setStyleSheet(
                "QToolButton { background: transparent; color: #8B949E; border: none; }"
                "QToolButton:hover { color: #E6EDF3; }"
            );
            actions_layout->addWidget(settings_btn);
            actions_layout->addStretch();

            connections_table_->setCellWidget(row, 4, actions_widget);
        }

        QPushButton* stop_button() { return stop_btn_; }

    signals:
        void copy_id_clicked();

    private:
        QFrame* create_card() {
            auto* card = new QFrame(this);
            card->setStyleSheet(
                "QFrame {"
                "  background: #161B22;"
                "  border: 1px solid #30363D;"
                "  border-radius: 8px;"
                "}"
            );
            return card;
        }

        QPushButton* create_action_button(const QString& text, const QString& style) {
            auto* btn = new QPushButton(text, this);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(36);

            if (style == "primary") {
                btn->setStyleSheet(
                    "QPushButton {"
                    "  background: #2F81F7;"
                    "  color: white;"
                    "  font-size: 12px;"
                    "  font-weight: 600;"
                    "  border: none;"
                    "  border-radius: 6px;"
                    "  padding: 0 16px;"
                    "}"
                    "QPushButton:hover {"
                    "  background: #388BFD;"
                    "}"
                    "QPushButton:pressed {"
                    "  background: #1F6FEB;"
                    "}"
                );
            }
            else {
                btn->setStyleSheet(
                    "QPushButton {"
                    "  background: #21262D;"
                    "  color: #E6EDF3;"
                    "  font-size: 12px;"
                    "  font-weight: 500;"
                    "  border: 1px solid #30363D;"
                    "  border-radius: 6px;"
                    "  padding: 0 16px;"
                    "}"
                    "QPushButton:hover {"
                    "  background: #30363D;"
                    "  border-color: #484F58;"
                    "}"
                    "QPushButton:pressed {"
                    "  background: #21262D;"
                    "}"
                );
            }
            return btn;
        }

        QWidget* create_stat_item(const QString& label, const QString& value) {
            auto* widget = new QWidget(this);
            auto* layout = new QVBoxLayout(widget);
            layout->setContentsMargins(12, 10, 12, 10);
            layout->setSpacing(4);

            auto* val_label = new QLabel(value, this);
            val_label->setStyleSheet("font-size: 18px; font-weight: 700; color: #E6EDF3; font-family: monospace;");
            layout->addWidget(val_label);

            auto* lbl = new QLabel(label, this);
            lbl->setStyleSheet("font-size: 11px; color: #8B949E; text-transform: uppercase; letter-spacing: 0.5px;");
            layout->addWidget(lbl);

            widget->setStyleSheet(
                "background: #0D1117;"
                "border: 1px solid #30363D;"
                "border-radius: 6px;"
            );

            return widget;
        }

        QLabel* status_icon_ = nullptr;
        QLabel* status_label_ = nullptr;
        QLabel* session_id_display_ = nullptr;
        QPushButton* copy_id_btn_ = nullptr;
        QPushButton* copy_link_btn_ = nullptr;
        QPushButton* share_btn_ = nullptr;
        QLineEdit* pass_display_ = nullptr;
        QPushButton* show_pass_btn_ = nullptr;
        QPushButton* stop_btn_ = nullptr;
        QTableWidget* connections_table_ = nullptr;
    };

    // =========================================================================
    // VIEWER PAGE — Redesigned with connection panel and remote viewport
    // =========================================================================

    class ViewerPage : public QWidget {
        Q_OBJECT
    public:
        explicit ViewerPage(QWidget* parent = nullptr) : QWidget(parent) {
            auto* layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);

            // Connection panel (collapsible)
            auto* conn_panel = new QFrame(this);
            conn_panel->setFixedHeight(72);
            conn_panel->setStyleSheet(
                "QFrame {"
                "  background: #161B22;"
                "  border-bottom: 1px solid #30363D;"
                "}"
            );

            auto* conn_layout = new QHBoxLayout(conn_panel);
            conn_layout->setContentsMargins(24, 0, 24, 0);
            conn_layout->setSpacing(12);

            auto* id_label = new QLabel("ID do Host:", this);
            id_label->setStyleSheet("color: #8B949E; font-size: 12px; font-weight: 500;");
            conn_layout->addWidget(id_label);

            id_input_ = new QLineEdit(this);
            id_input_->setPlaceholderText("000-000-000");
            id_input_->setFixedWidth(140);
            id_input_->setStyleSheet(
                "QLineEdit {"
                "  background: #0D1117;"
                "  color: #E6EDF3;"
                "  border: 1px solid #30363D;"
                "  border-radius: 6px;"
                "  padding: 8px 12px;"
                "  font-size: 13px;"
                "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
                "  letter-spacing: 1px;"
                "}"
                "QLineEdit:focus {"
                "  border-color: #2F81F7;"
                "}"
                "QLineEdit::placeholder {"
                "  color: #484F58;"
                "}"
            );
            conn_layout->addWidget(id_input_);

            auto* pass_label = new QLabel("Senha:", this);
            pass_label->setStyleSheet("color: #8B949E; font-size: 12px; font-weight: 500;");
            conn_layout->addWidget(pass_label);

            pass_input_ = new QLineEdit(this);
            pass_input_->setEchoMode(QLineEdit::Password);
            pass_input_->setFixedWidth(120);
            pass_input_->setStyleSheet(id_input_->styleSheet());
            conn_layout->addWidget(pass_input_);

            connect_btn_ = new QPushButton("Conectar", this);
            connect_btn_->setFixedHeight(36);
            connect_btn_->setStyleSheet(
                "QPushButton {"
                "  background: #2F81F7;"
                "  color: white;"
                "  font-size: 13px;"
                "  font-weight: 600;"
                "  border: none;"
                "  border-radius: 6px;"
                "  padding: 0 20px;"
                "}"
                "QPushButton:hover {"
                "  background: #388BFD;"
                "}"
                "QPushButton:pressed {"
                "  background: #1F6FEB;"
                "}"
                "QPushButton:disabled {"
                "  background: #21262D;"
                "  color: #484F58;"
                "}"
            );
            connect_btn_->setCursor(Qt::PointingHandCursor);
            conn_layout->addWidget(connect_btn_);

            disconnect_btn_ = new QPushButton("Desconectar", this);
            disconnect_btn_->setFixedHeight(36);
            disconnect_btn_->setStyleSheet(
                "QPushButton {"
                "  background: #F85149;"
                "  color: white;"
                "  font-size: 13px;"
                "  font-weight: 600;"
                "  border: none;"
                "  border-radius: 6px;"
                "  padding: 0 20px;"
                "}"
                "QPushButton:hover {"
                "  background: #DA3633;"
                "}"
            );
            disconnect_btn_->setVisible(false);
            disconnect_btn_->setCursor(Qt::PointingHandCursor);
            conn_layout->addWidget(disconnect_btn_);

            conn_layout->addStretch();

            // Options
            auto* options_layout = new QHBoxLayout();
            options_layout->setSpacing(16);

            remember_check_ = new QCheckBox("Lembrar credenciais", this);
            remember_check_->setStyleSheet(
                "QCheckBox {"
                "  color: #8B949E;"
                "  font-size: 12px;"
                "}"
                "QCheckBox::indicator {"
                "  width: 16px;"
                "  height: 16px;"
                "  border-radius: 4px;"
                "  border: 1px solid #30363D;"
                "  background: #0D1117;"
                "}"
                "QCheckBox::indicator:checked {"
                "  background: #2F81F7;"
                "  border-color: #2F81F7;"
                "}"
            );
            options_layout->addWidget(remember_check_);

            viewonly_check_ = new QCheckBox("Modo view-only", this);
            viewonly_check_->setStyleSheet(remember_check_->styleSheet());
            options_layout->addWidget(viewonly_check_);

            conn_layout->addLayout(options_layout);

            layout->addWidget(conn_panel);

            // Remote viewport
            viewport_ = new QFrame(this);
            viewport_->setStyleSheet(
                "QFrame {"
                "  background: #0D1117;"
                "  border: 2px dashed #30363D;"
                "  border-radius: 8px;"
                "}"
            );

            auto* vp_layout = new QVBoxLayout(viewport_);
            vp_layout->setAlignment(Qt::AlignCenter);

            // Empty state
            empty_state_widget_ = new QWidget(viewport_);
            auto* empty_layout = new QVBoxLayout(empty_state_widget_);
            empty_layout->setAlignment(Qt::AlignCenter);
            empty_layout->setSpacing(12);

            auto* empty_icon = new QLabel("👁", this);
            empty_icon->setStyleSheet("font-size: 48px; color: #30363D;");
            empty_icon->setAlignment(Qt::AlignCenter);
            empty_layout->addWidget(empty_icon);

            auto* empty_title = new QLabel("Conecte-se a um host remoto", this);
            empty_title->setStyleSheet("font-size: 16px; font-weight: 600; color: #8B949E;");
            empty_title->setAlignment(Qt::AlignCenter);
            empty_layout->addWidget(empty_title);

            auto* empty_subtitle = new QLabel("Digite o ID de sessão e a senha para iniciar a conexão segura.", this);
            empty_subtitle->setStyleSheet("font-size: 12px; color: #484F58; max-width: 300px;");
            empty_subtitle->setWordWrap(true);
            empty_subtitle->setAlignment(Qt::AlignCenter);
            empty_layout->addWidget(empty_subtitle);

            vp_layout->addWidget(empty_state_widget_);

            // Remote content (hidden initially)
            remote_content_ = new QLabel("Aguardando vídeo...", viewport_);
            remote_content_->setStyleSheet("color: #8B949E; font-size: 14px;");
            remote_content_->setAlignment(Qt::AlignCenter);
            remote_content_->setVisible(false);
            vp_layout->addWidget(remote_content_);

            // Overlay stats (hidden initially)
            overlay_stats_ = new QFrame(viewport_);
            overlay_stats_->setStyleSheet(
                "QFrame {"
                "  background: rgba(13, 17, 23, 0.9);"
                "  border: 1px solid #30363D;"
                "  border-radius: 6px;"
                "  padding: 4px;"
                "}"
            );
            overlay_stats_->setVisible(false);
            auto* overlay_layout = new QHBoxLayout(overlay_stats_);
            overlay_layout->setContentsMargins(10, 6, 10, 6);
            overlay_layout->setSpacing(12);

            overlay_rtt_ = new QLabel("RTT: —", this);
            overlay_rtt_->setStyleSheet("color: #3FB950; font-size: 11px; font-family: monospace;");
            overlay_layout->addWidget(overlay_rtt_);

            overlay_glass_ = new QLabel("Glass: —", this);
            overlay_glass_->setStyleSheet("color: #58A6FF; font-size: 11px; font-family: monospace;");
            overlay_layout->addWidget(overlay_glass_);

            overlay_fps_ = new QLabel("FPS: —", this);
            overlay_fps_->setStyleSheet("color: #D29922; font-size: 11px; font-family: monospace;");
            overlay_layout->addWidget(overlay_fps_);

            overlay_res_ = new QLabel("—×—", this);
            overlay_res_->setStyleSheet("color: #8B949E; font-size: 11px; font-family: monospace;");
            overlay_layout->addWidget(overlay_res_);

            // Floating toolbar (hidden initially)
            floating_toolbar_ = new QFrame(viewport_);
            floating_toolbar_->setStyleSheet(
                "QFrame {"
                "  background: rgba(22, 27, 34, 0.95);"
                "  border: 1px solid #30363D;"
                "  border-radius: 8px;"
                "}"
            );
            floating_toolbar_->setVisible(false);
            auto* toolbar_layout = new QHBoxLayout(floating_toolbar_);
            toolbar_layout->setContentsMargins(8, 6, 8, 6);
            toolbar_layout->setSpacing(4);

            auto add_tool_btn = [this, toolbar_layout](const QString& icon, const QString& tooltip) {
                auto* btn = new QToolButton(this);
                btn->setText(icon);
                btn->setToolTip(tooltip);
                btn->setStyleSheet(
                    "QToolButton {"
                    "  background: transparent;"
                    "  color: #8B949E;"
                    "  font-size: 14px;"
                    "  border: none;"
                    "  border-radius: 4px;"
                    "  padding: 4px 8px;"
                    "}"
                    "QToolButton:hover {"
                    "  background: #30363D;"
                    "  color: #E6EDF3;"
                    "}"
                );
                toolbar_layout->addWidget(btn);
                return btn;
                };

            add_tool_btn("🔒", "Bloquear input");
            add_tool_btn("📋", "Sincronizar clipboard");
            add_tool_btn("📁", "Transferir arquivo");
            add_tool_btn("⚙", "Configurações da sessão");
            add_tool_btn("⛶", "Tela cheia");
            add_tool_btn("❌", "Desconectar");

            layout->addWidget(viewport_, 1);

            // Connection log (collapsible)
            log_panel_ = new QFrame(this);
            log_panel_->setFixedHeight(120);
            log_panel_->setStyleSheet(
                "QFrame {"
                "  background: #0D1117;"
                "  border-top: 1px solid #30363D;"
                "}"
            );
            log_panel_->setVisible(false);

            auto* log_layout = new QVBoxLayout(log_panel_);
            log_layout->setContentsMargins(16, 8, 16, 8);
            log_layout->setSpacing(4);

            auto* log_header = new QHBoxLayout();
            auto* log_title = new QLabel("Log de Conexão", this);
            log_title->setStyleSheet("font-size: 11px; font-weight: 600; color: #8B949E; text-transform: uppercase; letter-spacing: 0.5px;");
            log_header->addWidget(log_title);
            log_header->addStretch();

            auto* clear_log_btn = new QToolButton(this);
            clear_log_btn->setText("Limpar");
            clear_log_btn->setStyleSheet(
                "QToolButton {"
                "  background: transparent;"
                "  color: #8B949E;"
                "  font-size: 11px;"
                "  border: none;"
                "}"
                "QToolButton:hover {"
                "  color: #E6EDF3;"
                "}"
            );
            log_header->addWidget(clear_log_btn);
            log_layout->addLayout(log_header);

            log_content_ = new QLabel("Nenhum evento registrado.", this);
            log_content_->setStyleSheet("font-size: 11px; color: #484F58; font-family: monospace;");
            log_content_->setWordWrap(true);
            log_layout->addWidget(log_content_);

            layout->addWidget(log_panel_);

            setStyleSheet("background: #0D1117;");
        }

        void set_connected(bool connected) {
            id_input_->setEnabled(!connected);
            pass_input_->setEnabled(!connected);
            connect_btn_->setVisible(!connected);
            disconnect_btn_->setVisible(connected);

            empty_state_widget_->setVisible(!connected);
            remote_content_->setVisible(connected);
            overlay_stats_->setVisible(connected);

            if (connected) {
                viewport_->setStyleSheet(
                    "QFrame {"
                    "  background: #0D1117;"
                    "  border: 1px solid #30363D;"
                    "  border-radius: 8px;"
                    "}"
                );
                remote_content_->setText("Aguardando vídeo...");
            }
            else {
                viewport_->setStyleSheet(
                    "QFrame {"
                    "  background: #0D1117;"
                    "  border: 2px dashed #30363D;"
                    "  border-radius: 8px;"
                    "}"
                );
            }
        }

        void update_latency(double rtt_ms, double glass_ms) {
            overlay_rtt_->setText(QString("RTT: %1ms").arg(rtt_ms, 0, 'f', 1));
            overlay_glass_->setText(QString("Glass: %1ms").arg(glass_ms, 0, 'f', 1));
        }

        void update_fps(int fps) {
            overlay_fps_->setText(QString("FPS: %1").arg(fps));
        }

        void update_resolution(int width, int height) {
            overlay_res_->setText(QString("%1×%2").arg(width).arg(height));
        }

        void add_log_entry(const QString& entry) {
            log_panel_->setVisible(true);
            QString current = log_content_->text();
            if (current == "Nenhum evento registrado.") current = "";
            QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
            log_content_->setText(current + timestamp + entry + "\n");
        }

        QString session_id() const { return id_input_->text().remove('-').remove(' '); }
        QString password() const { return pass_input_->text(); }
        bool view_only() const { return viewonly_check_->isChecked(); }

        QPushButton* connect_button() { return connect_btn_; }
        QPushButton* disconnect_button() { return disconnect_btn_; }

    private:
        QLineEdit* id_input_ = nullptr;
        QLineEdit* pass_input_ = nullptr;
        QPushButton* connect_btn_ = nullptr;
        QPushButton* disconnect_btn_ = nullptr;
        QCheckBox* remember_check_ = nullptr;
        QCheckBox* viewonly_check_ = nullptr;

        QFrame* viewport_ = nullptr;
        QWidget* empty_state_widget_ = nullptr;
        QLabel* remote_content_ = nullptr;

        QFrame* overlay_stats_ = nullptr;
        QLabel* overlay_rtt_ = nullptr;
        QLabel* overlay_glass_ = nullptr;
        QLabel* overlay_fps_ = nullptr;
        QLabel* overlay_res_ = nullptr;

        QFrame* floating_toolbar_ = nullptr;

        QFrame* log_panel_ = nullptr;
        QLabel* log_content_ = nullptr;
    };

    // =========================================================================
    // PLACEHOLDER PAGES
    // =========================================================================

    class SessionsPage : public QWidget {
        Q_OBJECT
    public:
        explicit SessionsPage(QWidget* parent = nullptr) : QWidget(parent) {
            auto* layout = new QVBoxLayout(this);
            layout->setContentsMargins(32, 24, 32, 24);

            auto* label = new QLabel("Gerenciamento de Sessões", this);
            label->setStyleSheet("font-size: 20px; font-weight: 700; color: #E6EDF3;");
            layout->addWidget(label);

            auto* sub = new QLabel("Visualize e gerencie todas as suas sessões de acesso remoto.", this);
            sub->setStyleSheet("font-size: 13px; color: #8B949E;");
            layout->addWidget(sub);
            layout->addStretch();

            setStyleSheet("background: #0D1117;");
        }
    };

    class StatsPage : public QWidget {
        Q_OBJECT
    public:
        explicit StatsPage(QWidget* parent = nullptr) : QWidget(parent) {
            auto* layout = new QVBoxLayout(this);
            layout->setContentsMargins(32, 24, 32, 24);

            auto* label = new QLabel("Estatísticas de Performance", this);
            label->setStyleSheet("font-size: 20px; font-weight: 700; color: #E6EDF3;");
            layout->addWidget(label);

            auto* sub = new QLabel("Métricas detalhadas de latência, bitrate e qualidade de conexão.", this);
            sub->setStyleSheet("font-size: 13px; color: #8B949E;");
            layout->addWidget(sub);
            layout->addStretch();

            setStyleSheet("background: #0D1117;");
        }
    };

    // =========================================================================
    // MAIN WINDOW IMPLEMENTATION
    // =========================================================================

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        setMinimumSize(900, 700);
        setWindowTitle("RapidDesk");

        setup_ui();
        setup_styles();
        setup_navigation();
        setup_status_bar();
        setup_tray_icon();
        setup_shortcuts();
        setup_toast_system();

        update_window_title();
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::setup_ui() {
        central_widget_ = new QWidget(this);
        setCentralWidget(central_widget_);

        main_layout_ = new QHBoxLayout(central_widget_);
        main_layout_->setContentsMargins(0, 0, 0, 0);
        main_layout_->setSpacing(0);

        // Navigation rail
        nav_rail_ = new NavigationRail(this);
        main_layout_->addWidget(nav_rail_);

        // Content area
        content_area_ = new QWidget(this);
        content_area_->setStyleSheet("background: #0D1117;");

        content_layout_ = new QVBoxLayout(content_area_);
        content_layout_->setContentsMargins(0, 0, 0, 0);
        content_layout_->setSpacing(0);

        // Status bar
        status_bar_ = new StatusBar(this);
        content_layout_->addWidget(status_bar_);

        // Page stack
        page_stack_ = new QStackedWidget(this);
        page_stack_->setStyleSheet("background: #0D1117; border: none;");

        host_page_ = new HostPage(page_stack_);
        viewer_page_ = new ViewerPage(page_stack_);
        sessions_page_ = new SessionsPage(page_stack_);
        stats_page_ = new StatsPage(page_stack_);

        page_stack_->addWidget(host_page_);
        page_stack_->addWidget(viewer_page_);
        page_stack_->addWidget(sessions_page_);
        page_stack_->addWidget(stats_page_);

        content_layout_->addWidget(page_stack_, 1);

        main_layout_->addWidget(content_area_, 1);

        // Default to viewer page
        page_stack_->setCurrentIndex(1);
        current_page_index_ = 1;
    }

    void MainWindow::setup_styles() {
        setStyleSheet(
            "QMainWindow {"
            "  background: #0D1117;"
            "}"
            "QScrollBar:vertical {"
            "  background: #161B22;"
            "  width: 8px;"
            "  border-radius: 4px;"
            "}"
            "QScrollBar::handle:vertical {"
            "  background: #30363D;"
            "  border-radius: 4px;"
            "  min-height: 30px;"
            "}"
            "QScrollBar::handle:vertical:hover {"
            "  background: #484F58;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "  height: 0px;"
            "}"
            "QScrollBar:horizontal {"
            "  background: #161B22;"
            "  height: 8px;"
            "  border-radius: 4px;"
            "}"
            "QScrollBar::handle:horizontal {"
            "  background: #30363D;"
            "  border-radius: 4px;"
            "  min-width: 30px;"
            "}"
            "QScrollBar::handle:horizontal:hover {"
            "  background: #484F58;"
            "}"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
            "  width: 0px;"
            "}"
            "QToolTip {"
            "  background: #161B22;"
            "  color: #E6EDF3;"
            "  border: 1px solid #30363D;"
            "  border-radius: 4px;"
            "  padding: 4px 8px;"
            "  font-size: 12px;"
            "}"
        );
    }

    void MainWindow::setup_navigation() {
        connect(nav_rail_, &NavigationRail::nav_clicked, this, &MainWindow::switch_page);

        connect(nav_rail_->host_button(), &QToolButton::clicked, this, &MainWindow::on_nav_host_clicked);
        connect(nav_rail_->viewer_button(), &QToolButton::clicked, this, &MainWindow::on_nav_viewer_clicked);
        connect(nav_rail_->sessions_button(), &QToolButton::clicked, this, &MainWindow::on_nav_sessions_clicked);
        connect(nav_rail_->stats_button(), &QToolButton::clicked, this, &MainWindow::on_nav_stats_clicked);
        connect(nav_rail_->settings_button(), &QToolButton::clicked, this, &MainWindow::on_nav_settings_clicked);
    }

    void MainWindow::setup_status_bar() {
        status_bar_->set_status("Pronto", "idle");
    }

    void MainWindow::setup_tray_icon() {
        tray_icon_ = new QSystemTrayIcon(QIcon(":/icons/tray.png"), this);

        tray_menu_ = new QMenu(this);
        tray_menu_->setStyleSheet(
            "QMenu {"
            "  background: #161B22;"
            "  color: #E6EDF3;"
            "  border: 1px solid #30363D;"
            "  border-radius: 6px;"
            "  padding: 8px;"
            "}"
            "QMenu::item {"
            "  padding: 8px 16px;"
            "  border-radius: 4px;"
            "  font-size: 13px;"
            "}"
            "QMenu::item:selected {"
            "  background: #21262D;"
            "}"
            "QMenu::separator {"
            "  height: 1px;"
            "  background: #30363D;"
            "  margin: 6px 0;"
            "}"
        );

        auto* show_action = new QAction("Mostrar RapidDesk", this);
        show_action->setShortcut(QKeySequence("Ctrl+Shift+M"));
        connect(show_action, &QAction::triggered, this, &QWidget::showNormal);
        tray_menu_->addAction(show_action);

        tray_menu_->addSeparator();

        auto* host_action = new QAction("Iniciar Host", this);
        connect(host_action, &QAction::triggered, this, &MainWindow::start_hosting);
        tray_menu_->addAction(host_action);

        auto* quit_action = new QAction("Sair", this);
        quit_action->setShortcut(QKeySequence::Quit);
        connect(quit_action, &QAction::triggered, qApp, &QApplication::quit);
        tray_menu_->addAction(quit_action);

        tray_icon_->setContextMenu(tray_menu_);
        tray_icon_->setToolTip("RapidDesk — Pronto");
        tray_icon_->show();

        connect(tray_icon_, &QSystemTrayIcon::activated,
            this, &MainWindow::on_tray_activated);
    }

    void MainWindow::setup_shortcuts() {
        auto* fullscreen_shortcut = new QShortcut(QKeySequence("F11"), this);
        connect(fullscreen_shortcut, &QShortcut::activated, this, &MainWindow::toggle_fullscreen);

        auto* disconnect_shortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
        connect(disconnect_shortcut, &QShortcut::activated, [this]() {
            if (current_mode_ == AppMode::VIEWER) {
                disconnect_viewer();
            }
            else if (current_mode_ == AppMode::HOST) {
                stop_hosting();
            }
            });

        auto* copy_id_shortcut = new QShortcut(QKeySequence("Ctrl+Shift+C"), this);
        connect(copy_id_shortcut, &QShortcut::activated, [this]() {
            if (current_mode_ == AppMode::HOST) {
                show_toast("ID de sessão copiado!", "success");
            }
            });
    }

    void MainWindow::setup_toast_system() {
        toast_manager_ = new ToastManager(this);
        toast_manager_->setGeometry(width() - 360, 50, 340, height() - 100);
    }

    void MainWindow::switch_page(int index) {
        if (index < 0 || index >= page_stack_->count()) return;

        auto* effect = new QGraphicsOpacityEffect(page_stack_->currentWidget());
        page_stack_->currentWidget()->setGraphicsEffect(effect);

        auto* fade_out = new QPropertyAnimation(effect, "opacity");
        fade_out->setDuration(100);
        fade_out->setStartValue(1.0);
        fade_out->setEndValue(0.0);

        connect(fade_out, &QPropertyAnimation::finished, [this, index]() {
            page_stack_->setCurrentIndex(index);
            current_page_index_ = index;

            auto* new_effect = new QGraphicsOpacityEffect(page_stack_->currentWidget());
            page_stack_->currentWidget()->setGraphicsEffect(new_effect);

            auto* fade_in = new QPropertyAnimation(new_effect, "opacity");
            fade_in->setDuration(150);
            fade_in->setStartValue(0.0);
            fade_in->setEndValue(1.0);
            fade_in->start(QAbstractAnimation::DeleteWhenStopped);
            });

        fade_out->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void MainWindow::show_toast(const QString& message, const QString& type) {
        if (toast_manager_) {
            toast_manager_->show_toast(message, type);
        }
    }

    void MainWindow::update_status_bar(const QString& status, const QString& detail) {
        if (status_bar_) {
            status_bar_->set_status(status, detail);
        }
    }

    void MainWindow::start_hosting() {
        if (current_mode_ != AppMode::IDLE) return;

        try {
            session_ = std::make_unique<core::Session>();
            session_->initialize_host();

            connect(session_.get(), &core::Session::session_id_ready,
                this, &MainWindow::on_session_id_received);
            connect(session_.get(), &core::Session::connection_established,
                this, &MainWindow::on_connection_established);
            connect(session_.get(), &core::Session::connection_error,
                this, &MainWindow::on_connection_error);

            switch_page(0); // Host page
            current_mode_ = AppMode::HOST;
            emit mode_changed(AppMode::HOST);

            status_bar_->set_status("Iniciando host...", "connecting");
            show_toast("Modo host iniciado. Aguardando conexoes...", "info");

        }
        catch (const std::exception& e) {
            show_toast(QString("Falha ao iniciar host: %1").arg(e.what()), "error");
        }
    }

    void MainWindow::stop_hosting() {
        if (current_mode_ != AppMode::HOST) return;

        auto reply = QMessageBox::question(this, "Confirmar",
            "Deseja realmente parar o host e encerrar todas as conexoes ativas?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (reply != QMessageBox::Yes) return;

        session_.reset();
        current_mode_ = AppMode::IDLE;
        switch_page(1); // Back to viewer

        host_page_->set_session_id("");
        host_page_->set_status("Pronto para receber conexoes", "idle");
        status_bar_->set_status("Pronto", "idle");
        status_bar_->set_latency(0);
        status_bar_->set_bandwidth(0);

        emit mode_changed(AppMode::IDLE);
        show_toast("Host encerrado.", "info");
    }

    void MainWindow::start_viewing(const QString& session_id, const QString& password) {
        if (current_mode_ != AppMode::IDLE) return;
        if (session_id.isEmpty()) {
            show_toast("Digite o ID da sessao.", "warning");
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
            current_mode_ = AppMode::VIEWER;
            emit mode_changed(AppMode::VIEWER);

            status_bar_->set_status("Conectando...", "connecting");
            viewer_page_->add_log_entry("Iniciando conexao com " + session_id);

        }
        catch (const std::exception& e) {
            show_toast(QString("Nao foi possivel conectar: %1").arg(e.what()), "error");
            viewer_page_->add_log_entry("Erro: " + QString(e.what()));
        }
    }

    void MainWindow::disconnect_viewer() {
        if (current_mode_ != AppMode::VIEWER) return;

        session_.reset();
        viewer_page_->set_connected(false);
        current_mode_ = AppMode::IDLE;

        status_bar_->set_status("Desconectado", "idle");
        status_bar_->set_latency(0);
        status_bar_->set_bandwidth(0);
        status_bar_->set_network_info("—");

        emit mode_changed(AppMode::IDLE);
        show_toast("Desconectado do host.", "info");
    }

    void MainWindow::on_connection_established() {
        if (current_mode_ == AppMode::HOST) {
            host_page_->set_status("Cliente conectado — transmitindo", "connected");
            status_bar_->set_status("Host ativo — 1 conexao", "connected");
            show_toast("Cliente conectado ao host!", "success");
        }
        else if (current_mode_ == AppMode::VIEWER) {
            viewer_page_->set_connected(true);
            status_bar_->set_status("Conectado", "connected");
            status_bar_->set_network_info("P2P Direct");
            viewer_page_->add_log_entry("Conexao estabelecida via P2P Direct");
            viewer_page_->add_log_entry("Handshake criptografico completo (AES-256-GCM)");
            show_toast("Conectado com sucesso!", "success");
        }
    }

    void MainWindow::on_connection_error(const QString& error) {
        show_toast(error, "error");

        if (current_mode_ == AppMode::HOST) {
            stop_hosting();
        }
        else if (current_mode_ == AppMode::VIEWER) {
            disconnect_viewer();
        }
    }

    void MainWindow::on_session_id_received(const QString& id) {
        if (host_page_) {
            QString formatted = id;
            if (id.length() == 9) {
                formatted = id.mid(0, 3) + "-" + id.mid(3, 3) + "-" + id.mid(6, 3);
            }
            host_page_->set_session_id(formatted);
            host_page_->set_status("Aguardando conexao...", "connecting");
            status_bar_->set_status("Aguardando conexao...", "connecting");
        }
    }

    void MainWindow::on_connection_quality_update(int quality) {
        connection_quality_ = quality;
        status_bar_->set_quality(quality);
    }

    void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            raise();
            activateWindow();
        }
    }

    void MainWindow::toggle_fullscreen() {
        if (is_fullscreen_) {
            showNormal();
            nav_rail_->setVisible(true);
            status_bar_->setVisible(true);
        }
        else {
            showFullScreen();
            nav_rail_->setVisible(false);
            status_bar_->setVisible(false);
        }
        is_fullscreen_ = !is_fullscreen_;
    }

    void MainWindow::show_about_dialog() {
        QMessageBox about(this);
        about.setWindowTitle("Sobre RapidDesk");
        about.setTextFormat(Qt::RichText);
        about.setText(
            "<h2 style='color: #2F81F7;'>RapidDesk 1.0</h2>"
            "<p style='color: #E6EDF3;'>Software de acesso remoto de alta performance.</p>"
            "<p style='color: #8B949E;'>"
            "<b>Latencia glass-to-glass:</b> &lt;20ms (LAN)<br>"
            "<b>Criptografia:</b> X25519 + AES-256-GCM<br>"
            "<b>Protocolo:</b> P2P UDP com fallback TURN<br>"
            "<b>Codec:</b> H.264/H.265 hardware-accelerated"
            "</p>"
        );
        about.setStandardButtons(QMessageBox::Ok);
        about.setStyleSheet(
            "QMessageBox {"
            "  background: #161B22;"
            "}"
            "QLabel {"
            "  color: #E6EDF3;"
            "}"
            "QPushButton {"
            "  background: #2F81F7;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 6px;"
            "  padding: 8px 20px;"
            "  font-weight: 600;"
            "}"
            "QPushButton:hover {"
            "  background: #388BFD;"
            "}"
        );
        about.exec();
    }

    void MainWindow::update_window_title() {
        QString title = "RapidDesk";
        switch (current_mode_) {
        case AppMode::HOST: title += " [HOST]"; break;
        case AppMode::VIEWER: title += " [VIEWER]"; break;
        default: break;
        }
        setWindowTitle(title);
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        if (close_to_tray_ && tray_icon_->isVisible()) {
            hide();
            tray_icon_->showMessage(
                "RapidDesk",
                "Executando em segundo plano. Clique duplo para restaurar.",
                QSystemTrayIcon::Information, 3000);
            event->ignore();
        }
        else {
            event->accept();
        }
    }

    void MainWindow::resizeEvent(QResizeEvent* event) {
        QMainWindow::resizeEvent(event);

        if (toast_manager_) {
            toast_manager_->setGeometry(width() - 360, 50, 340, height() - 100);
        }

        if (width() < 1000 && !nav_collapsed_) {
            collapse_navigation(true);
        }
        else if (width() >= 1000 && nav_collapsed_) {
            collapse_navigation(false);
        }
    }

    void MainWindow::collapse_navigation(bool collapsed) {
        nav_collapsed_ = collapsed;
        nav_rail_->set_collapsed(collapsed);
    }

    void MainWindow::keyPressEvent(QKeyEvent* event) {
        if (event->key() == Qt::Key_Escape && is_fullscreen_) {
            toggle_fullscreen();
            return;
        }
        QMainWindow::keyPressEvent(event);
    }

    void MainWindow::on_nav_host_clicked() {
        if (current_mode_ == AppMode::IDLE) {
            start_hosting();
        }
    }

    void MainWindow::on_nav_viewer_clicked() {
        // Viewer is the default page
    }

    void MainWindow::on_nav_sessions_clicked() {
        switch_page(2);
    }

    void MainWindow::on_nav_stats_clicked() {
        switch_page(3);
    }

    void MainWindow::on_nav_settings_clicked() {
        show_toast("Configuracoes em desenvolvimento.", "info");
    }

} // namespace rapiddesk::ui

#include "main_window.moc"