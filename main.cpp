#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QTimer>
#include <QObject>
#include <QtQml>
#include "src/controller/ClientController.h"

int main(int argc, char *argv[])
{
    // app identity for settings
    QCoreApplication::setOrganizationName("Huxley");
    QCoreApplication::setOrganizationDomain("huxley.chat");
    QCoreApplication::setApplicationName("HuxleyChat");

    // gui event loop
    QGuiApplication app(argc, argv);

    ClientController controller;
    QTimer::singleShot(0, &controller, &ClientController::start); // start after event loop
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller, &ClientController::shutdown); // shutdown on quit

    // qml controller singleton
    qmlRegisterSingletonInstance("chat", 1, 0, "Controller", &controller);

    QQmlApplicationEngine engine;

    // load qml root
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/chat/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1; // no root objects

    return app.exec();
}
