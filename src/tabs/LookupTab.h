#pragma once

#include <QWidget>

class QLineEdit;
class QComboBox;
class QPushButton;
class CommandRunner;
class OutputConsole;

// "Lookup" tab: DNS record lookups via `host -t <type> <address>`.
class LookupTab : public QWidget {
    Q_OBJECT

public:
    explicit LookupTab(QWidget *parent = nullptr);

private slots:
    void run();
    void onFinished(int exitCode);
    void onFailedToStart(const QString &program);

private:
    QLineEdit *m_addressEdit;
    QComboBox *m_typeCombo;
    QPushButton *m_runButton;
    OutputConsole *m_output;
    CommandRunner *m_runner;
};
