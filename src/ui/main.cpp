#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QDir>

#include "main_window.hpp"
#include "core/logger.hpp"
#include "core/config.hpp"

using namespace rapiddesk;

int main(int argc, char* argv[])
{
    // High DPI support
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QApplication app(argc, argv);
    app.setApplicationName("RapidDesk");
    app.setOrganizationName("RapidDesk");
    app.setApplicationDisplayName("RapidDesk — Acesso Remoto");

    // Initialize logging
    auto& logger = core::Logger::instance();
    logger.initialize("rapiddesk", core::LogLevel::Debug);

    // Load config
    auto& config = core::Config::instance();
    config.load();

    // Apply dark theme
    QFile styleFile(":/styles/dark.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }
    else {
        // Fallback dark palette
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);
        app.setPalette(darkPalette);
    }

    // Set modern font
    QFont font("Segoe UI", 10);
    font.setStyleHint(QFont::SansSerif);
    app.setFont(font);

    ui::MainWindow window;
    window.show();

    return app.exec();
}