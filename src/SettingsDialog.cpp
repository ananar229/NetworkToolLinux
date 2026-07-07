#include "SettingsDialog.h"

#include "common/LanguageManager.h"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Einstellungen"));
    setModal(true);
    setFixedWidth(300);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    layout->addWidget(new QLabel(QStringLiteral("<b>%1</b>").arg(tr("Sprache"))));

    m_languageCombo = new QComboBox();
    const QString currentCode = LanguageManager::currentLanguageCode();
    const QVector<LanguageManager::Language> languages = LanguageManager::availableLanguages();
    for (int i = 0; i < languages.size(); ++i) {
        m_languageCombo->addItem(languages[i].nativeName, languages[i].code);
        if (languages[i].code == currentCode) {
            m_languageCombo->setCurrentIndex(i);
        }
    }
    layout->addWidget(m_languageCombo);
    // activated() only fires on user interaction, not on the setCurrentIndex() above.
    connect(m_languageCombo, &QComboBox::activated, this, &SettingsDialog::onLanguageActivated);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

void SettingsDialog::onLanguageActivated(int index) {
    const QString code = m_languageCombo->itemData(index).toString();
    if (code.isEmpty() || code == LanguageManager::currentLanguageCode()) {
        return;
    }
    LanguageManager::setLanguageCode(code);

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Neustart erforderlich"),
        tr("Die neue Sprache wird nach einem Neustart der Anwendung übernommen. Jetzt neu starten?"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QProcess::startDetached(QApplication::applicationFilePath(), QApplication::arguments());
        QApplication::quit();
    }
}
