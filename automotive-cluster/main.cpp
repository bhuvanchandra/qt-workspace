#include "qtquick1applicationviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QtQuick1ApplicationViewer viewer;
    viewer.setOrientation(QtQuick1ApplicationViewer::ScreenOrientationLockLandscape);
    viewer.setMainQmlFile(QLatin1String("qml/automotive-cluster/main.qml"));
    viewer.showFullScreen();

    return app.exec();
}
