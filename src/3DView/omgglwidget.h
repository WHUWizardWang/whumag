#ifndef OMGGLWIDGET_H
#define OMGGLWIDGET_H

#include <QWidget>
#include <QVector>
#include "omgglctrl.h"
#include "glwidget.h"
#include "terrain.h"

namespace Ui
{
    class OmgGLWidget;
}

class OmgGLWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OmgGLWidget(QWidget *parent = nullptr);
    ~OmgGLWidget();

    void setHeight(int height);
    void setWidth(int width);
    void setMask(VVb &mask);
    void setTerrain(VVf &terrain);
    void setMagano(VVf &magano);
    void load();

private:
    Ui::OmgGLWidget *ui;
    Terrain *m_terrain_ptr;
};

#endif // OMGGLWIDGET_H
