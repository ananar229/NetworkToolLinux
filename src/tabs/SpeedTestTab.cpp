#include "SpeedTestTab.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

// Cloudflare's public, unauthenticated speed-test endpoints (the same
// ones speed.cloudflare.com itself uses): __down streams N random bytes,
// __up accepts an arbitrary body and discards it.

SpeedTestTab::SpeedTestTab(QWidget *parent) : QWidget(parent) {
    m_manager = new QNetworkAccessManager(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    layout->addWidget(new QLabel(tr("Measure the latency, download and upload speed of your network connection.")));

    auto *statsRow = new QHBoxLayout();
    statsRow->setSpacing(12);

    auto *pingCard = new QGroupBox(tr("Ping"));
    m_pingValue = makeStatWidget(pingCard, tr("-- ms"));

    auto *downloadCard = new QGroupBox(tr("Download"));
    m_downloadValue = makeStatWidget(downloadCard, tr("-- Mbps"));

    auto *uploadCard = new QGroupBox(tr("Upload"));
    m_uploadValue = makeStatWidget(uploadCard, tr("-- Mbps"));

    statsRow->addWidget(pingCard);
    statsRow->addWidget(downloadCard);
    statsRow->addWidget(uploadCard);
    layout->addLayout(statsRow);

    m_statusLabel = new QLabel(tr("Ready."));
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    auto *buttonRow = new QHBoxLayout();
    m_actionButton = new QPushButton(tr("Start Test"));
    m_actionButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_actionButton);
    layout->addLayout(buttonRow);

    layout->addStretch(1);

    connect(m_actionButton, &QPushButton::clicked, this, &SpeedTestTab::toggle);
}

QLabel *SpeedTestTab::makeStatWidget(QWidget *card, const QString &initialText) {
    auto *cardLayout = new QVBoxLayout(card);
    auto *value = new QLabel(initialText);
    value->setAlignment(Qt::AlignCenter);
    value->setStyleSheet("font-size: 22px; font-weight: 600;");
    cardLayout->addWidget(value);
    return value;
}

void SpeedTestTab::toggle() {
    if (m_testing) {
        cancelTest(tr("Test cancelled."));
    } else {
        startTest();
    }
}

void SpeedTestTab::startTest() {
    m_testing = true;
    m_phase = Phase::Latency;
    m_latencySamplesDone = 0;
    m_latencyTotalMs = 0;

    m_pingValue->setText(tr("-- ms"));
    m_downloadValue->setText(tr("-- Mbps"));
    m_uploadValue->setText(tr("-- Mbps"));

    m_actionButton->setText(tr("Stop"));
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Measuring latency..."));

    measureLatencyStep();
}

void SpeedTestTab::measureLatencyStep() {
    if (m_latencySamplesDone >= kLatencySamples) {
        const double averageMs = double(m_latencyTotalMs) / kLatencySamples;
        m_pingValue->setText(tr("%1 ms").arg(qRound(averageMs)));
        startDownloadTest();
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://speed.cloudflare.com/__down?bytes=0")));
    m_timer.start();
    m_currentReply = m_manager->get(request);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &SpeedTestTab::onNetworkError);
    connect(m_currentReply, &QNetworkReply::finished, this, [this] {
        if (!m_currentReply) {
            return;
        }
        const bool ok = m_currentReply->error() == QNetworkReply::NoError;
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        if (!ok || !m_testing) {
            return;
        }
        m_latencyTotalMs += m_timer.elapsed();
        m_latencySamplesDone++;
        m_progressBar->setValue(m_latencySamplesDone * 100 / kLatencySamples);
        measureLatencyStep();
    });
}

void SpeedTestTab::startDownloadTest() {
    m_phase = Phase::Download;
    m_statusLabel->setText(tr("Testing download speed..."));
    m_progressBar->setValue(0);

    QNetworkRequest request(QUrl(QStringLiteral("https://speed.cloudflare.com/__down?bytes=%1").arg(kDownloadBytes)));
    m_timer.start();
    m_currentReply = m_manager->get(request);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &SpeedTestTab::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished, this, &SpeedTestTab::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &SpeedTestTab::onNetworkError);
}

void SpeedTestTab::onDownloadProgress(qint64 received, qint64 total) {
    const double seconds = m_timer.elapsed() / 1000.0;
    if (received <= 0 || seconds <= 0) {
        return;
    }
    const double mbps = (received * 8.0) / seconds / 1'000'000.0;
    m_downloadValue->setText(tr("%1 Mbps").arg(mbps, 0, 'f', 1));
    if (total > 0) {
        m_progressBar->setValue(static_cast<int>(received * 100 / total));
    }
}

void SpeedTestTab::onDownloadFinished() {
    if (!m_currentReply) {
        return;
    }
    const bool ok = m_currentReply->error() == QNetworkReply::NoError;
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    if (!ok || !m_testing) {
        return;
    }
    startUploadTest();
}

void SpeedTestTab::startUploadTest() {
    m_phase = Phase::Upload;
    m_statusLabel->setText(tr("Testing upload speed..."));
    m_progressBar->setValue(0);

    if (m_uploadPayload.size() != kUploadBytes) {
        m_uploadPayload = QByteArray(static_cast<int>(kUploadBytes), 'x');
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://speed.cloudflare.com/__up")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));
    m_timer.start();
    m_currentReply = m_manager->post(request, m_uploadPayload);
    connect(m_currentReply, &QNetworkReply::uploadProgress, this, &SpeedTestTab::onUploadProgress);
    connect(m_currentReply, &QNetworkReply::finished, this, &SpeedTestTab::onUploadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &SpeedTestTab::onNetworkError);
}

void SpeedTestTab::onUploadProgress(qint64 sent, qint64 total) {
    const double seconds = m_timer.elapsed() / 1000.0;
    if (sent <= 0 || seconds <= 0) {
        return;
    }
    const double mbps = (sent * 8.0) / seconds / 1'000'000.0;
    m_uploadValue->setText(tr("%1 Mbps").arg(mbps, 0, 'f', 1));
    if (total > 0) {
        m_progressBar->setValue(static_cast<int>(sent * 100 / total));
    }
}

void SpeedTestTab::onUploadFinished() {
    if (!m_currentReply) {
        return;
    }
    const bool ok = m_currentReply->error() == QNetworkReply::NoError;
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    if (!ok || !m_testing) {
        return;
    }
    finishTest();
}

void SpeedTestTab::onNetworkError() {
    if (!m_currentReply) {
        return;
    }
    cancelTest(tr("Network error: %1").arg(m_currentReply->errorString()));
}

void SpeedTestTab::cancelTest(const QString &statusMessage) {
    m_testing = false;
    m_phase = Phase::Idle;
    if (m_currentReply) {
        m_currentReply->disconnect(this);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    m_actionButton->setText(tr("Start Test"));
    m_statusLabel->setText(statusMessage);
    m_progressBar->setValue(0);
}

void SpeedTestTab::finishTest() {
    m_testing = false;
    m_phase = Phase::Idle;
    m_actionButton->setText(tr("Start Test"));
    m_progressBar->setValue(100);
    m_statusLabel->setText(tr("Test complete."));
}
