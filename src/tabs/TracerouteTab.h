#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class NativeTraceroute;
class OutputConsole;

// "Traceroute" tab: show the route packets take to a host, using an
// unprivileged UDP+ICMP probe technique instead of an external binary.
class TracerouteTab : public QWidget {
    Q_OBJECT

public:
    explicit TracerouteTab(QWidget *parent = nullptr);

private slots:
    void toggle();
    void onFinished();
    void onHopResult(int ttl, const QString &address, int rttMs);
    void onReachedDestination();
    void onFallingBackToTcp();
    void onErrorOccurred(const QString &message);

private:
    void onStarted();

    QLineEdit *m_addressEdit;
    QPushButton *m_actionButton;
    OutputConsole *m_output;
    NativeTraceroute *m_tracer;
};
