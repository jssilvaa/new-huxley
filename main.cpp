// main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext> 
#include <QCoreApplication>
#include "src/controller/ClientController.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Huxley");
    QCoreApplication::setOrganizationDomain("huxley.chat");
    QCoreApplication::setApplicationName("HuxleyChat");

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    
    ClientController controller; 
    controller.start(); // boot connection
    engine.rootContext()->setContextProperty("Controller", &controller); 

    engine.loadFromModule("chat", "Main"); 
    if (engine.rootObjects().isEmpty()) {
        return -1; 
    }

    return app.exec();
}
