#include "OutputConsole.h"

#include <QFont>
#include <QTextCursor>

OutputConsole::OutputConsole(QWidget *parent) : QPlainTextEdit(parent) {
    setReadOnly(true);
    QFont font("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(11);
    setFont(font);
    setObjectName("outputConsole");
}

static void insertAtEnd(QPlainTextEdit *edit, const QString &html, bool isHtml) {
    QTextCursor cursor(edit->document());
    cursor.movePosition(QTextCursor::End);
    if (isHtml) {
        cursor.insertHtml(html);
    } else {
        cursor.insertText(html);
    }
    edit->setTextCursor(cursor);
    edit->ensureCursorVisible();
}

void OutputConsole::appendPlain(const QString &text) {
    insertAtEnd(this, text, false);
}

void OutputConsole::appendNotice(const QString &text) {
    insertAtEnd(this, QStringLiteral("<div style=\"color:#6e6e6e;\">%1</div>").arg(text.toHtmlEscaped()), true);
}

void OutputConsole::appendError(const QString &text) {
    insertAtEnd(this, QStringLiteral("<div style=\"color:#b00020;\">%1</div>").arg(text.toHtmlEscaped()), true);
}
