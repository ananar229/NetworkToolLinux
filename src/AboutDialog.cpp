#include "AboutDialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace {
// Legal text is kept in its canonical English form regardless of UI
// language, matching the repository's root LICENSE file.
const char *const kMitLicenseText =
    "MIT License\n"
    "\n"
    "Copyright (c) 2026 ananar229\n"
    "\n"
    "Permission is hereby granted, free of charge, to any person obtaining a copy of this "
    "software and associated documentation files (the \"Software\"), to deal in the Software "
    "without restriction, including without limitation the rights to use, copy, modify, merge, "
    "publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons "
    "to whom the Software is furnished to do so, subject to the following conditions:\n"
    "\n"
    "The above copyright notice and this permission notice shall be included in all copies or "
    "substantial portions of the Software.\n"
    "\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, "
    "INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR "
    "PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE "
    "FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR "
    "OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER "
    "DEALINGS IN THE SOFTWARE.";
}

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Über %1").arg(QApplication::applicationName()));
    setModal(true);
    setFixedWidth(480);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(QStringLiteral("<span style=\"font-size:16px; font-weight:bold;\">%1</span>")
                                       .arg(QApplication::applicationName()));
    layout->addWidget(titleLabel);

    auto *versionLabel = new QLabel(tr("Version %1").arg(QApplication::applicationVersion()));
    layout->addWidget(versionLabel);

    auto *licenseTitle = new QLabel(QStringLiteral("<b>%1</b>").arg(tr("Lizenz")));
    layout->addWidget(licenseTitle);

    auto *licenseText = new QPlainTextEdit();
    licenseText->setReadOnly(true);
    licenseText->setPlainText(QString::fromLatin1(kMitLicenseText));
    // Always-on (not as-needed) scrollbar: the scrollbar only appearing
    // after the first layout pass shifted this widget's effective height,
    // which the surrounding QVBoxLayout didn't fully re-flow, leaving this
    // box overlapping the disclaimer text below it.
    licenseText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    licenseText->setFixedHeight(180);
    layout->addWidget(licenseText);

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
    // Give this label a known width up front: mixing a heightForWidth
    // widget (word-wrapped label) with fixed-size widgets in the same
    // QVBoxLayout otherwise left the layout using a stale/incorrect height
    // for a preceding item, overlapping the license box above.
    disclaimerLabel->setFixedWidth(460);
    layout->addWidget(disclaimerLabel);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttonBox);

    // The word-wrapped disclaimer label's real height (at this dialog's
    // fixed width) is only known once the layout has actually run at that
    // width; forcing one more pass here (rather than leaving it to the
    // first show()) keeps the license box from overlapping the text below.
    layout->activate();
    adjustSize();
}
