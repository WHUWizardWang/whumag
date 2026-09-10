#ifndef MAPFORM_H
#define MAPFORM_H

#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QQmlContext>

#include "omgqmlpolygon.h"

namespace Ui
{
    class MapForm;
}

class MapForm : public QWidget
{
    Q_OBJECT

public:
    explicit MapForm(QWidget *parent = nullptr);
    ~MapForm();

    OmgQmlPolygon *qml_polygon_;
    void test(VesselPath vp);
public slots:
    void onDrawStateChanged(int state);


private:
    void initQmlMap();
    void loadJson(const QString& file_path);
    void checkTileResources();



    Ui::MapForm *ui;
};

#endif // MAPFORM_H
