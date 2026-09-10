#ifndef OMGRASTER_H
#define OMGRASTER_H

//#include <QObject>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QVector3D>
#include <QImage>
#include <QImageReader>
#include "omggeopoint.h"
#include "omgpolygon.h"

class OmgRaster //: public QObject
{
    //    Q_OBJECT
public:
    //    explicit OmgRaster(QObject *parent = nullptr);
    OmgRaster();

    float latResolution() const;
    float lonResolution() const;

    bool loadFromImage(const QString &filePath);
    bool loadFromPoints(const QString &filePath);
    bool getColorMap(const QString &colorMapPath);

    void setLeftTopLatLon(float lat, float lon);
    void setLatLonResolution(float lat_rsl, float lon_rsl);

    QVector<OmgGeoPoint> getInternalPoints(QVector<Vec2d> &POL, float radio = 1.0f);

    void setResImgPath(const QString &path);

private:
    bool valid(float value);
    bool invalid(float value);
    int getColorIndex(float value);


    float m_left_top_lat = 0.0f;
    float m_left_top_lon = 0.0f;
    float m_lat_resolution = 0.0f;
    float m_lon_resolution = 0.0f;
    float m_height = 0.0f;
    float m_width = 0.0f;
    float m_maxValue = 0.0f;
    float m_minValue = 0.0f;
    QVector<float> m_data;
    QVector<QVector3D> m_colorMap;

    QString m_img_path;
};

#endif // OMGRASTER_H
