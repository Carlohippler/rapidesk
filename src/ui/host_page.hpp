#pragma once

#include <QWidget>

namespace rapiddesk::ui {

    class HostPage : public QWidget {
        Q_OBJECT
    public:
        explicit HostPage(QWidget* parent = nullptr) : QWidget(parent) {}
        ~HostPage() = default;
    };

} // namespace rapiddesk::ui