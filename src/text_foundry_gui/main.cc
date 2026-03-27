#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("TextFoundry");
  app.setOrganizationName("TextFoundry");
  app.setDesktopFileName("textfoundry");
  app.setWindowIcon(
      QIcon(QStringLiteral(":/qt/qml/TextFoundry/resource/app/textfoundry.svg")));

  QQmlApplicationEngine engine;

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine.loadFromModule("TextFoundry", "Main");
  return app.exec();
}
