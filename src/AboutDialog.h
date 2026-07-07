#pragma once

#include <QDialog>

// "Über" dialog: shows the app name, version, license, and the liability
// disclaimer (same wording as README.md's "Haftungsausschluss" section).
class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
};
