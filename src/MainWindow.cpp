#include "MainWindow.h"

#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "tabs/FingerTab.h"
#include "tabs/InfoTab.h"
#include "tabs/LookupTab.h"
#include "tabs/NetstatTab.h"
#include "tabs/PingTab.h"
#include "tabs/PortScanTab.h"
#include "tabs/SpeedTestTab.h"
#include "tabs/TracerouteTab.h"
#include "tabs/WhoisTab.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QStyle>
#include <QTextCursor>
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

enum class EditOp { Undo, Redo, Cut, Copy, Paste, Delete, SelectAll };

// Edit-menu actions apply to whichever text field currently has focus,
// since the app has no single central document to edit.
void invokeEditOp(EditOp op) {
    QWidget *focused = QApplication::focusWidget();
    if (auto *lineEdit = qobject_cast<QLineEdit *>(focused)) {
        switch (op) {
            case EditOp::Undo: lineEdit->undo(); break;
            case EditOp::Redo: lineEdit->redo(); break;
            case EditOp::Cut: lineEdit->cut(); break;
            case EditOp::Copy: lineEdit->copy(); break;
            case EditOp::Paste: lineEdit->paste(); break;
            case EditOp::Delete: lineEdit->del(); break;
            case EditOp::SelectAll: lineEdit->selectAll(); break;
        }
    } else if (auto *textEdit = qobject_cast<QPlainTextEdit *>(focused)) {
        switch (op) {
            case EditOp::Undo: textEdit->undo(); break;
            case EditOp::Redo: textEdit->redo(); break;
            case EditOp::Cut: textEdit->cut(); break;
            case EditOp::Copy: textEdit->copy(); break;
            case EditOp::Paste: textEdit->paste(); break;
            case EditOp::Delete: {
                QTextCursor cursor = textEdit->textCursor();
                cursor.removeSelectedText();
                textEdit->setTextCursor(cursor);
                break;
            }
            case EditOp::SelectAll: textEdit->selectAll(); break;
        }
    }
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("Network Tool"));
    resize(720, 560);

    setupMenuBar();

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

void MainWindow::setupMenuBar() {
    auto *fileMenu = menuBar()->addMenu(tr("Ablage"));

    QAction *newWindowAction = fileMenu->addAction(tr("Neues Fenster"));
    newWindowAction->setShortcut(QKeySequence::New);
    connect(newWindowAction, &QAction::triggered, this, [] {
        auto *window = new MainWindow();
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    });

    QAction *closeAction = fileMenu->addAction(tr("Schliessen"));
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu(tr("Bearbeiten"));

    QAction *undoAction = editMenu->addAction(tr("Widerrufen"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::Undo); });

    QAction *redoAction = editMenu->addAction(tr("Wiederholen"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::Redo); });

    editMenu->addSeparator();

    QAction *cutAction = editMenu->addAction(tr("Ausschneiden"));
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::Cut); });

    QAction *copyAction = editMenu->addAction(tr("Kopieren"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::Copy); });

    QAction *pasteAction = editMenu->addAction(tr("Einfügen"));
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::Paste); });

    QAction *deleteAction = editMenu->addAction(tr("Löschen"));
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::Delete); });

    QAction *selectAllAction = editMenu->addAction(tr("Alles Auswählen"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, [] { invokeEditOp(EditOp::SelectAll); });

    auto *viewMenu = menuBar()->addMenu(tr("Darstellung"));

    QAction *fullscreenAction = viewMenu->addAction(tr("Vollbildmodus"));
    fullscreenAction->setCheckable(true);
    fullscreenAction->setShortcut(QKeySequence::FullScreen);
    connect(fullscreenAction, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            showFullScreen();
        } else {
            showNormal();
        }
    });

    auto *helpMenu = menuBar()->addMenu(tr("Hilfe"));

    QAction *settingsAction = helpMenu->addAction(tr("Einstellungen"));
    connect(settingsAction, &QAction::triggered, this, [this] {
        SettingsDialog dialog(this);
        dialog.exec();
    });

    helpMenu->addSeparator();

    QAction *helpAction = helpMenu->addAction(tr("Hilfe"));
    helpAction->setShortcut(QKeySequence::HelpContents);
    connect(helpAction, &QAction::triggered, this, [this] {
        QMessageBox::information(
            this, tr("Hilfe"),
            tr("Wähle oben in der Leiste ein Werkzeug aus (Info, Netstat, Ping, Lookup, "
               "Traceroute, Whois, Finger, Port Scan, Speed), gib die benötigten Angaben "
               "ein und starte die Aktion über den jeweiligen Knopf. Die Ausgabe erscheint "
               "im Textfeld darunter."));
    });

    QAction *aboutAction = helpMenu->addAction(tr("Über"));
    connect(aboutAction, &QAction::triggered, this, [this] {
        AboutDialog dialog(this);
        dialog.exec();
    });
}
