#pragma once

#include <QString>
#include <QVector>

// Persists the user's language choice (QSettings) and installs the matching
// Qt translators. "system" defers to QLocale::system(); Qt just falls back
// to the untranslated source text for any string a catalog doesn't cover.
namespace LanguageManager {

struct Language {
    QString code; // "system", "de", "en", "fr", "it", "es"
    QString nativeName;
};

QVector<Language> availableLanguages();

QString currentLanguageCode();
void setLanguageCode(const QString &code);

// Installs (or re-installs) the translators for the given language code.
// Safe to call again after the language changes; removes any previously
// installed translators first.
void installTranslators(const QString &code);

} // namespace LanguageManager
