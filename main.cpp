#include "src/MainWindow.h"
#include "src/common/LanguageManager.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Network Tool");
    app.setApplicationDisplayName("Network Tool");
    app.setOrganizationName("NetworkTool");
    app.setApplicationVersion(QStringLiteral(NETWORKTOOL_VERSION));

    LanguageManager::installTranslators(LanguageManager::currentLanguageCode());

    MainWindow window;
    window.show();

    return app.exec();
}


