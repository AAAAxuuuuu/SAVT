#include "MarkdownRenderer.h"
#include "savt/ui/AnalysisController.h"

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
  QQuickStyle::setStyle(QStringLiteral("Basic"));
  QGuiApplication app(argc, argv);
  app.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.png")));

  savt::ui::AnalysisController analysisController;
  MarkdownRenderer markdownRenderer;
  if (argc > 1) {
    analysisController.setProjectRootPath(
        QDir::cleanPath(QString::fromLocal8Bit(argv[1])));
  }

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("_analysisController",
                                           &analysisController);
  engine.rootContext()->setContextProperty("analysisController",
                                           &analysisController);
  engine.rootContext()->setContextProperty("markdownRenderer",
                                           &markdownRenderer);
  engine.loadFromModule("SAVT.Desktop", "Main");
  if (engine.rootObjects().isEmpty()) {
    return -1;
  }

  return app.exec();
}
