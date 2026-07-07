#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

// Wraps QProcess to run a single external command and stream its
// output line-by-line, similar to how the tabs in the original
// Network Utility invoke ping/traceroute/whois/etc.
class CommandRunner : public QObject {
    Q_OBJECT

public:
    explicit CommandRunner(QObject *parent = nullptr);

    void start(const QString &program, const QStringList &arguments);
    void stop();
    bool isRunning() const;

signals:
    void outputReceived(const QString &text);
    void started();
    void finished(int exitCode);
    void failedToStart(const QString &program);

private:
    QProcess *m_process;
};
