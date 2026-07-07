#include "AboutDialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Über %1").arg(QApplication::applicationName()));
    setModal(true);
    setFixedWidth(420);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(QStringLiteral("<span style=\"font-size:16px; font-weight:bold;\">%1</span>")
                                       .arg(QApplication::applicationName()));
    layout->addWidget(titleLabel);

    auto *versionLabel = new QLabel(tr("Version %1").arg(QApplication::applicationVersion()));
    layout->addWidget(versionLabel);

    auto *licenseLabel = new QLabel(tr("Lizenz: MIT"));
    layout->addWidget(licenseLabel);

    auto *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    auto *disclaimerTitle = new QLabel(QStringLiteral("<b>%1</b>").arg(tr("Haftungsausschluss")));
    layout->addWidget(disclaimerTitle);

    auto *disclaimerLabel = new QLabel(
        tr("Diese Software wird ohne jegliche Garantie oder Gewährleistung "
           "bereitgestellt, weder ausdrücklich noch stillschweigend, "
           "einschließlich, aber nicht beschränkt auf die Gewährleistung der "
           "Marktgängigkeit, der Eignung für einen bestimmten Zweck und der "
           "Nichtverletzung von Rechten Dritter. Die Nutzung erfolgt auf "
           "eigenes Risiko. Der Autor übernimmt keine Haftung für Schäden "
           "oder Folgeschäden, die durch die Nutzung oder Unmöglichkeit der "
           "Nutzung dieser Software entstehen, und lehnt jegliche Haftung ab."));
    disclaimerLabel->setWordWrap(true);
    layout->addWidget(disclaimerLabel);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttonBox);
}
