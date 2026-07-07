#include "CommandRunner.h"

CommandRunner::CommandRunner(QObject *parent)
    : QObject(parent), m_process(new QProcess(this)) {
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        emit outputReceived(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    });

    connect(m_process, &QProcess::started, this, &CommandRunner::started);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus) { emit finished(exitCode); });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit failedToStart(m_process->program());
        }
    });
}

void CommandRunner::start(const QString &program, const QStringList &arguments) {
    if (isRunning()) {
        stop();
    }
    m_process->start(program, arguments);
}

void CommandRunner::stop() {
    if (!isRunning()) {
        return;
    }
    m_process->terminate();
    if (!m_process->waitForFinished(300)) {
        m_process->kill();
        m_process->waitForFinished(300);
    }
}

bool CommandRunner::isRunning() const {
    return m_process->state() != QProcess::NotRunning;
}
