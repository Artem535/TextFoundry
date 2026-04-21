#include <QGuiApplication>
#include <QIcon>
#include <QDir>
#include <QQmlApplicationEngine>
#include <qqml.h>

#include "components/syntax_highlighter.h"

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("TextFoundry");
  app.setApplicationVersion(QStringLiteral(TEXTFOUNDRY_APP_VERSION));
  app.setOrganizationName("TextFoundry");
  app.setDesktopFileName("textfoundry");
  app.setWindowIcon(
      QIcon(QStringLiteral(":/qt/qml/TextFoundry/resource/app/textfoundry.svg")));

  QQmlApplicationEngine engine;
  qmlRegisterType<tf::gui::SyntaxHighlighter>("TextFoundry", 1, 0,
                                              "SyntaxHighlighter");
  const QString app_dir = QCoreApplication::applicationDirPath();
  engine.addImportPath(app_dir + QStringLiteral("/../lib/qt6/qml"));
  engine.addImportPath(app_dir + QStringLiteral("/../lib64/qt6/qml"));
  engine.addImportPath(app_dir + QStringLiteral("/../Resources/qml"));
  engine.addImportPath(QDir(app_dir).absoluteFilePath(QStringLiteral("../qml")));

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine.loadFromModule("TextFoundry", "Main");
  return app.exec();
}
