#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("TextFoundry");
  app.setOrganizationName("TextFoundry");

  QQmlApplicationEngine engine;

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine.loadFromModule("TextFoundry", "Main");
  return app.exec();
}
