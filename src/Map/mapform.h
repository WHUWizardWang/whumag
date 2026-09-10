#ifndef MAPFORM_H
#define MAPFORM_H

#include <QWidget>
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



    Ui::MapForm *ui;
};

#endif // MAPFORM_H
