#include "MainWindow.h"

#include "tabs/FingerTab.h"
#include "tabs/InfoTab.h"
#include "tabs/LookupTab.h"
#include "tabs/NetstatTab.h"
#include "tabs/PingTab.h"
#include "tabs/PortScanTab.h"
#include "tabs/SpeedTestTab.h"
#include "tabs/TracerouteTab.h"
#include "tabs/WhoisTab.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <iterator>

namespace {

struct TabSpec {
    const char *label;
    QStyle::StandardPixmap icon;
};

constexpr TabSpec kTabSpecs[] = {
    {"Info", QStyle::SP_MessageBoxInformation},
    {"Netstat", QStyle::SP_DriveNetIcon},
    {"Ping", QStyle::SP_ArrowForward},
    {"Lookup", QStyle::SP_FileDialogContentsView},
    {"Traceroute", QStyle::SP_ArrowRight},
    {"Whois", QStyle::SP_FileDialogDetailedView},
    {"Finger", QStyle::SP_DialogHelpButton},
    {"Port Scan", QStyle::SP_DriveHDIcon},
    {"Speed", QStyle::SP_MediaSeekForward},
};

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("Network Tool"));
    resize(720, 560);

    auto *central = new QWidget(this);
    auto *outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto *toolbar = new QFrame();
    toolbar->setObjectName("segmentedToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(12, 10, 12, 10);
    toolbarLayout->setSpacing(0);

    auto *segmentGroup = new QButtonGroup(this);
    segmentGroup->setExclusive(true);

    m_stack = new QStackedWidget();

    toolbarLayout->addStretch(1);
    for (int i = 0; i < static_cast<int>(std::size(kTabSpecs)); ++i) {
        const TabSpec &spec = kTabSpecs[i];
        auto *button = new QToolButton();
        button->setText(tr(spec.label));
        button->setIcon(style()->standardIcon(spec.icon));
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setCheckable(true);
        button->setAutoRaise(false);
        button->setProperty("segmentPosition",
                             i == 0 ? "first" : (i == static_cast<int>(std::size(kTabSpecs)) - 1 ? "last" : "middle"));
        segmentGroup->addButton(button, i);
        toolbarLayout->addWidget(button);
    }
    toolbarLayout->addStretch(1);

    outerLayout->addWidget(toolbar);

    m_stack->addWidget(new InfoTab());
    m_stack->addWidget(new NetstatTab());
    m_stack->addWidget(new PingTab());
    m_stack->addWidget(new LookupTab());
    m_stack->addWidget(new TracerouteTab());
    m_stack->addWidget(new WhoisTab());
    m_stack->addWidget(new FingerTab());
    m_stack->addWidget(new PortScanTab());
    m_stack->addWidget(new SpeedTestTab());
    outerLayout->addWidget(m_stack, 1);

    connect(segmentGroup, &QButtonGroup::idClicked, m_stack, &QStackedWidget::setCurrentIndex);
    segmentGroup->button(0)->setChecked(true);

    setCentralWidget(central);

    // Colors are pulled from the current QPalette (palette(...) roles) rather
    // than hardcoded, so the window follows the desktop's light/dark theme
    // and accent color instead of forcing a fixed macOS-light look.
    setStyleSheet(R"(
        QFrame#segmentedToolbar {
            background-color: palette(window);
            border-bottom: 1px solid palette(mid);
        }
        QToolButton {
            background-color: palette(button);
            border: 1px solid palette(mid);
            border-right: none;
            padding: 6px 10px;
            color: palette(button-text);
        }
        QToolButton[segmentPosition="first"] {
            border-top-left-radius: 6px;
            border-bottom-left-radius: 6px;
        }
        QToolButton[segmentPosition="last"] {
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
            border-right: 1px solid palette(mid);
        }
        QToolButton:checked {
            background-color: palette(highlight);
            color: palette(highlighted-text);
        }
        QToolButton:hover:!checked {
            background-color: palette(light);
        }
        QPlainTextEdit#outputConsole {
            background-color: palette(base);
            border: 1px solid palette(mid);
            border-radius: 4px;
        }
        QGroupBox {
            border: 1px solid palette(mid);
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 6px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px;
        }
        QPushButton {
            padding: 5px 18px;
            border-radius: 5px;
            border: 1px solid palette(mid);
            background-color: palette(button);
        }
        QPushButton:default {
            background-color: palette(highlight);
            color: palette(highlighted-text);
            border: 1px solid palette(highlight);
        }
        QPushButton:hover:!default {
            background-color: palette(light);
        }
        QLineEdit, QComboBox, QSpinBox {
            padding: 4px 6px;
            border: 1px solid palette(mid);
            border-radius: 4px;
            background-color: palette(base);
        }
    )");
}
