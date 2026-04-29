#include <QGuiApplication>
#include <QIcon>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>
#include <QStyleHints>
#include <QQuickStyle>

int main(int argc, char** argv) {
  QLoggingCategory::setFilterRules(QStringLiteral("textfoundry.gui.syntaxhighlighter=true"));

#ifdef Q_OS_WIN
  QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));
  QQuickStyle::setFallbackStyle(QStringLiteral("Fusion"));
#endif

  QGuiApplication app(argc, argv);
#ifdef Q_OS_WIN
  app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif
  app.setApplicationName("TextFoundry");
  app.setApplicationVersion(QStringLiteral(TEXTFOUNDRY_APP_VERSION));
  app.setOrganizationName("TextFoundry");
  app.setDesktopFileName("textfoundry");
  app.setWindowIcon(
      QIcon(QStringLiteral(":/qt/qml/TextFoundry/resource/app/textfoundry.svg")));

  QQmlApplicationEngine engine;
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
