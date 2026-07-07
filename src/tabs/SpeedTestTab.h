#pragma once

#include <QElapsedTimer>
#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;

// "Speed" tab: measures latency, download and upload throughput against
// a public HTTP test endpoint. Not part of the original Network Utility,
// added on top of it.
class SpeedTestTab : public QWidget {
    Q_OBJECT

public:
    explicit SpeedTestTab(QWidget *parent = nullptr);

private slots:
    void toggle();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    void onUploadProgress(qint64 sent, qint64 total);
    void onUploadFinished();
    void onNetworkError();

private:
    enum class Phase { Idle, Latency, Download, Upload };

    void startTest();
    void cancelTest(const QString &statusMessage);
    void measureLatencyStep();
    void startDownloadTest();
    void startUploadTest();
    void finishTest();
    QLabel *makeStatWidget(QWidget *card, const QString &title);

    QNetworkAccessManager *m_manager;
    QNetworkReply *m_currentReply = nullptr;
    QElapsedTimer m_timer;

    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_pingValue;
    QLabel *m_downloadValue;
    QLabel *m_uploadValue;

    Phase m_phase = Phase::Idle;
    bool m_testing = false;
    int m_latencySamplesDone = 0;
    qint64 m_latencyTotalMs = 0;
    QByteArray m_uploadPayload;

    static constexpr int kLatencySamples = 4;
    static constexpr qint64 kDownloadBytes = 25'000'000;
    static constexpr qint64 kUploadBytes = 10'000'000;
};
