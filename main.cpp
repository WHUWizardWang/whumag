#include "mainwindow.h"
#include <QLoggingCategory>

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QQmlApplicationEngine>
#include <QApplication>
#include "contourplotter.h"

int main(int argc, char *argv[])
{

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QApplication a(argc, argv);
        // 添加 SSL 库路径
    QDir dir(QCoreApplication::applicationDirPath());
    QCoreApplication::addLibraryPath(dir.absolutePath());

        // QQmlApplicationEngine engine;
        // engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    MainWindow w;
    w.show();
    return a.exec();

}
