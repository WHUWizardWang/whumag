#ifndef TERRAIN_H
#define TERRAIN_H

#include <QVector>
#include <QVector3D>
#include <qopengl.h>
#include <QImage>
#include <QDebug>

using VVf = QVector<QVector<GLfloat>>;
using VVi = QVector<QVector<int>>;
using VVb = QVector<QVector<bool>>;

class Terrain
{
public:
    Terrain();
    const GLfloat *constData() const
    {
        return m_data.constData();
    }
    int count() const
    {
        return m_count;
    }
    int vertexCount() const
    {
        return m_count / 10;
    }
    void load();
    void setScale(double scale_z);

    void setMode(int mode);
    void setHeight(int height);
    void setWidth(int width);
    void setMask(const VVb &mask);
    void setTerrainData(const VVf &terrain);
    void setMaganoData(const VVf &magano);
    void setTerrainColorMap(const QVector<QVector3D> &colorMap);
    void setMaganoColorMap(const QVector<QVector3D> &colorMap);
    void calcuColorIndex();
    void addTerrainData();
    void addMaganoData();


private:
    void quad(const QVector3D &p1, const QVector3D &p2, const QVector3D &p3, const QVector3D &p4);
    void quad(const QVector3D &p1, const QVector3D &p2, const QVector3D &p3, const QVector3D &p4,
              const QVector3D &c1, const QVector3D &c2, const QVector3D &c3, const QVector3D &c4);
    void add(const QVector3D &v, const QVector3D &n);
    void add(const QVector3D &v, const QVector3D &n, const QVector3D &c);
    void normalize(QVector3D &p);
    double normalize(double v, double minV, double maxV);

    bool verifyDataSize(const VVf &data);
    void findMinMax(const VVf &data, GLfloat &minV, GLfloat &maxV);
    GLfloat mapMinMax(GLfloat value, GLfloat minV, GLfloat maxV, GLfloat minZ, GLfloat maxZ);
    void getQuadPoint(int dataType, int i, int j,
                      GLfloat minV, GLfloat maxV, GLfloat minZ, GLfloat maxZ,
                      QVector3D &point, QVector3D &color);

    QVector<GLfloat> m_data;
    int m_count;
    double m_scale;
    GLfloat m_dataType;

    VVb                 m_mask;             // Validity of points
    VVf                 m_terrain;          // Terrain data
    VVf                 m_magano;           // Magano data
    VVi                 m_t_colorIndex;     // Terrain color index
    VVi                 m_m_colorIndex;     // Magano color index
    QVector<QVector3D>  m_t_colorMap;       // Terrain colormap
    QVector<QVector3D>  m_m_colorMap;       // Magano colormap
    int                 m_mode;             // Display Mode: Only terrain(0), Terrain with magano texture(1), Terrain and magano(2).
    int                 m_height;
    int                 m_width;
};

#endif // TERRAIN_H
