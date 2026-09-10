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
    void load(const QVector<double> &griddate, int width, int height);
    void load(const QVector<double> &griddate, int width, int height,
              const QVector<QVector3D> &colors);

    void setHeight(int height);
    void setWidth(int width);
    void setMask(VVb &mask);
    void setTerrain(VVf &terrain);
    void setMagano(VVf &magano);
    void load();

public slots:
    //    void changeColorMap(const QString &colorMapName);
    //    void inverseColorMap(bool isInverse);


private:
    Ui::OmgGLWidget *ui;
    //    OmgGLCtrl *m_gl_ctrl;
    Terrain *m_terrain_ptr;

    //    void initColorMaps();
    //    void reverseVertor(QVector<QVector3D> &vec);
};

#endif // OMGGLWIDGET_H
