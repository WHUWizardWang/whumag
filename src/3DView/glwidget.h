#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include "terrain.h"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    void load();
    void loadNewTerrain(const QVector<double> &pnts, int width, int height, double scale);
    void loadNewTerrain(const QVector<double> &pnts, int width, int height, double scale,
                        const QVector<QVector3D> &colors);
    void setTerrainPointer(Terrain *terrain_ptr);
    void setColorMaps();

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void cleanup();

    void setScale(int scale);
    void toDefaultView();
    void zoomIn();
    void zoomOut();
    void panView(bool isPanning);

    void changeColorMap(const QString &colorMapName);
    void inverseColorMap(bool isInverse);
    void closeShade(bool isCloseShade);
    void closeTexture(bool isCloseTexture);

    void changeDisplayMode(int mode);
    void changeMagColorMap(const QString &colorMapName);
    void inverseMagColorMap(bool isInverse);

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
    void hasTextureChanged(bool);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupVertexAttribs();
    void zoom(GLfloat value);
    void initColorMaps();
    QVector<QVector3D> loadColorMap(const QString &colorMapPath);
    void reverseVertor(QVector<QVector3D> &vec);

    void myUpdate();

    QMap<QString, QVector<QVector3D>> m_colorMapMap;

    QOpenGLShaderProgram *m_program;
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_normalMatrixLoc;
    int m_lightPosLoc;
    int m_colorMapLoc;
    int m_scaleLoc;
    int m_colorNumLoc;
    int m_closeShadeLoc;
    int m_closeTextureLoc;
    QMatrix4x4 m_proj;      // 投影矩阵
    QMatrix4x4 m_camera;    // 相机位置矩阵
    QMatrix4x4 m_world;     // 世界矩阵

    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_terrainVbo;
    bool m_core;
    int m_xRot;             // X轴旋转角
    int m_yRot;             // Y轴旋转角
    int m_zRot;             // Z轴旋转角
    QPoint m_lastPos;       // 上次鼠标位置

    Terrain *m_terrain_ptr; // 点数据
    bool m_isPanning;       // 是否处于平移状态


    GLfloat m_scale;        // 高程缩放因子
    QVector<QVector3D> m_colorMap;  // 色板
    int m_colorNum;         // 颜色数量
    int m_closeShade;       // 关闭阴影
    bool m_isInverse;
    int m_closeTexture;
    bool m_hasTexture;

    QVector<QVector3D> m_magColorMap;
    int m_isMagInverse;
    int m_displayMode;
};

#endif // GLWIDGET_H
