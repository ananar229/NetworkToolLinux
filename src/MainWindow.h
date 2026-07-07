#pragma once

#include <QMainWindow>

class QStackedWidget;

// Top-level window: a macOS-style segmented control across the top
// ("Info", "Netstat", "Ping", ...) that switches between the tool
// pages held in a QStackedWidget, mirroring the classic Network Utility.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenuBar();

    QStackedWidget *m_stack;
};
