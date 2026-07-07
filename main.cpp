#include "src/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Network Tool");
    app.setApplicationDisplayName("Network Tool");
    app.setOrganizationName("NetworkTool");

    MainWindow window;
    window.show();

    return app.exec();
}
