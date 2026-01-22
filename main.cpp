#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QTimer>
#include <QObject>
#include <QtQml>
#include "src/controller/ClientController.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Huxley");
    QCoreApplication::setOrganizationDomain("huxley.chat");
    QCoreApplication::setApplicationName("HuxleyChat");

    QGuiApplication app(argc, argv);

    ClientController controller;
    QTimer::singleShot(0, &controller, &ClientController::start);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller, &ClientController::shutdown);

    qmlRegisterSingletonInstance("chat", 1, 0, "Controller", &controller);

    QQmlApplicationEngine engine;

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/chat/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
