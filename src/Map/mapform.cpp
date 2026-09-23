#include "mapform.h"
#include "ui_mapform.h"

#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QDir>
#include <QUrl>

MapForm::MapForm(QWidget *parent) :
    QWidget(parent),
    qml_polygon_(new OmgQmlPolygon()),
    ui(new Ui::MapForm)
{
    ui->setupUi(this);
    ui->quickWidget->engine()->rootContext()->setContextProperty("qmlPolygon", this->qml_polygon_);
    // Qt.application.dirPath is not a real QML property (it silently
    // evaluates to undefined), which was why the offline map tile directory
    // ("undefined/offline_tiles") never resolved to anything real. Expose
    // the actual executable directory as a context property instead.
    ui->quickWidget->engine()->rootContext()->setContextProperty("appDirPath", QCoreApplication::applicationDirPath());

    // Offline base map: tiles in <exe dir>/offline_tiles/<zoom>/<x>/<y>.png, served to the map as a
    // "custom URL" tile source with a file:// address, so the map never goes online (the built-in
    // online sources need an API key and otherwise return watermarked tiles).  The highest zoom
    // folder present limits how far the map can be zoomed in.
    const QDir tileDir(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("offline_tiles")));
    int maxZoom = -1;
    for (const QString &entry : tileDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool ok = false;
        const int z = entry.toInt(&ok);
        if (ok)
            maxZoom = qMax(maxZoom, z);
    }
    if (maxZoom < 0)
        qWarning() << "No offline map tiles in" << tileDir.absolutePath() << "- the base map stays empty";
    ui->quickWidget->engine()->rootContext()->setContextProperty(
        "offlineTilesUrl", QUrl::fromLocalFile(tileDir.absolutePath()).toString() + QLatin1Char('/'));
    ui->quickWidget->engine()->rootContext()->setContextProperty("offlineMaxZoom", maxZoom);
    initQmlMap();
    ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/Map/main.qml")));


    connect((QObject *)ui->quickWidget->rootObject(), SIGNAL(drawStateChanged(int)), this, SLOT(onDrawStateChanged(int)));

//    test();
}

void MapForm::test(VesselPath vp){
//    VesselPath vp;
//    vp.name="123";
//    vp.color="purple";
//    vp.width = 8;
//    vp.pos.push_back(QGeoCoordinate(20.0,114.0));
//    vp.pos.push_back(QGeoCoordinate(21.0,114.0));
//    vp.pos.push_back(QGeoCoordinate(21.0,113.0));
    this->qml_polygon_->addVesselPath(vp);
}

MapForm::~MapForm()
{
    delete qml_polygon_;
    delete ui;
}


void MapForm::initQmlMap()
{
    qmlRegisterType<OmgQmlPolygon>("OmgGeoObject", 1, 0, "OmgQmlPolygon");
//    qmlRegisterType<OmgQmlPolygon>("OmgGeoObject", 1, 0, "OmgQmlPoint");

//    auto engine = ui->quickWidget->rootContext()

}

void MapForm::onDrawStateChanged(int state)
{
    switch (state)
    {
        case 0:
        case 2:
            ui->quickWidget->setCursor(Qt::ArrowCursor);
            break;
        case 1:
            ui->quickWidget->setCursor(Qt::CrossCursor);
            break;
        default:
            break;
    }
}

