#pragma once

#include <QWidget>

class QLineEdit;
class QCheckBox;
class QSpinBox;
class QPushButton;
class CommandRunner;
class OutputConsole;

// "Ping" tab: send ICMP echo requests to a host, optionally limited
// to a fixed number of pings, streaming live results.
class PingTab : public QWidget {
    Q_OBJECT

public:
    explicit PingTab(QWidget *parent = nullptr);

private slots:
    void toggle();
    void onStarted();
    void onFinished(int exitCode);
    void onFailedToStart(const QString &program);

private:
    QLineEdit *m_addressEdit;
    QCheckBox *m_limitCheckBox;
    QSpinBox *m_countSpinBox;
    QPushButton *m_actionButton;
    OutputConsole *m_output;
    CommandRunner *m_runner;
};
