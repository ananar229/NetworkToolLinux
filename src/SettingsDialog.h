#pragma once

#include <QDialog>

class QComboBox;

// "Einstellungen" dialog: lets the user pick the UI language from a
// dropdown. Only has a Close button — the choice is saved and applied
// immediately (asking to restart, since this app doesn't live-retranslate
// its widgets).
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onLanguageActivated(int index);

private:
    QComboBox *m_languageCombo;
};
