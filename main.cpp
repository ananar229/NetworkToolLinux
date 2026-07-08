#include "src/MainWindow.h"
#include "src/common/LanguageManager.h"

#include <QApplication>
#include <QIcon>
#include <QSize>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Network Tool");
    app.setApplicationDisplayName("Network Tool");
    app.setOrganizationName("NetworkTool");
    app.setApplicationVersion(QStringLiteral(NETWORKTOOL_VERSION));

    QIcon appIcon;
    for (int size : {16, 32, 48, 64, 128, 256, 512}) {
        appIcon.addFile(QStringLiteral(":/icons/networktool_%1.png").arg(size), QSize(size, size));
    }
    app.setWindowIcon(appIcon);

    LanguageManager::installTranslators(LanguageManager::currentLanguageCode());

    MainWindow window;
    window.show();

    return app.exec();
}


