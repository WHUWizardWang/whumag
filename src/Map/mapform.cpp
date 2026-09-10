#include "mapform.h"
#include "ui_mapform.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

MapForm::MapForm(QWidget *parent) :
    QWidget(parent),
    qml_polygon_(new OmgQmlPolygon()),
    ui(new Ui::MapForm)
{
    ui->setupUi(this);
    ui->quickWidget->engine()->rootContext()->setContextProperty("qmlPolygon", this->qml_polygon_);
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

