#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QObject>
#include <QSettings>
#include <QTranslator>

namespace {

QTranslator *g_appTranslator = nullptr;
QTranslator *g_qtBaseTranslator = nullptr;

const QVector<QString> &supportedCodes() {
    static const QVector<QString> codes = {
        QStringLiteral("de"), QStringLiteral("en"), QStringLiteral("fr"),
        QStringLiteral("it"), QStringLiteral("es"),
    };
    return codes;
}

// Resolves "system" to one of our supported codes, or an empty string if
// the system locale isn't one we ship a translation for.
QString resolveLocaleCode(const QString &code) {
    if (code != QStringLiteral("system")) {
        return code;
    }
    const QString systemCode = QLocale::system().name().left(2).toLower();
    if (supportedCodes().contains(systemCode)) {
        return systemCode;
    }
    return {};
}

} // namespace

QVector<LanguageManager::Language> LanguageManager::availableLanguages() {
    return {
        {QStringLiteral("system"), QObject::tr("Systemsprache")},
        {QStringLiteral("de"), QStringLiteral("Deutsch")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("fr"), QStringLiteral("Français")},
        {QStringLiteral("it"), QStringLiteral("Italiano")},
        {QStringLiteral("es"), QStringLiteral("Español")},
    };
}

QString LanguageManager::currentLanguageCode() {
    QSettings settings;
    return settings.value(QStringLiteral("language/code"), QStringLiteral("system")).toString();
}

void LanguageManager::setLanguageCode(const QString &code) {
    QSettings settings;
    settings.setValue(QStringLiteral("language/code"), code);
}

void LanguageManager::installTranslators(const QString &code) {
    QCoreApplication *app = QCoreApplication::instance();

    if (g_appTranslator) {
        app->removeTranslator(g_appTranslator);
        delete g_appTranslator;
        g_appTranslator = nullptr;
    }
    if (g_qtBaseTranslator) {
        app->removeTranslator(g_qtBaseTranslator);
        delete g_qtBaseTranslator;
        g_qtBaseTranslator = nullptr;
    }

    const QString locale = resolveLocaleCode(code);
    if (locale.isEmpty()) {
        return;
    }

    g_appTranslator = new QTranslator(app);
    if (g_appTranslator->load(QStringLiteral(":/i18n/NetworkTool_%1.qm").arg(locale))) {
        app->installTranslator(g_appTranslator);
    } else {
        delete g_appTranslator;
        g_appTranslator = nullptr;
    }

    g_qtBaseTranslator = new QTranslator(app);
    if (g_qtBaseTranslator->load(QLocale(locale), QStringLiteral("qtbase"), QStringLiteral("_"),
                                  QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app->installTranslator(g_qtBaseTranslator);
    } else {
        delete g_qtBaseTranslator;
        g_qtBaseTranslator = nullptr;
    }
}
