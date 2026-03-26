#include "savt/ui/AnalysisController.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    savt::ui::AnalysisController analysisController;
    if (argc > 1) {
        analysisController.setProjectRootPath(QDir::cleanPath(QString::fromLocal8Bit(argv[1])));
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("analysisController", &analysisController);
    engine.loadFromModule("SAVT.Desktop", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
