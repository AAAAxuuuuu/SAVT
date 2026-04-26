#include "MarkdownRenderer.h"
#include "savt/ui/AnalysisController.h"

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QQuickStyle>
#include <QWindow>

namespace {

void centerAndShowRootWindow(QQmlApplicationEngine &engine) {
  if (engine.rootObjects().isEmpty()) {
    return;
  }

  auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
  if (window == nullptr) {
    return;
  }

  const QScreen *screen = window->screen() != nullptr
                              ? window->screen()
                              : QGuiApplication::primaryScreen();
  if (screen != nullptr) {
    const QRect availableGeometry = screen->availableGeometry();
    const QSize windowSize = window->size();
    const int x = availableGeometry.x() +
                  qMax(0, (availableGeometry.width() - windowSize.width()) / 2);
    const int y = availableGeometry.y() +
                  qMax(0, (availableGeometry.height() - windowSize.height()) / 2);
    window->setPosition(x, y);
  }

  window->show();
}

}  // namespace

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
  centerAndShowRootWindow(engine);

  return app.exec();
}
