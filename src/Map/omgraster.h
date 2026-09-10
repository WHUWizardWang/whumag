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
    OmgRaster(const OmgRaster &other);

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


    float m_left_top_lat;
    float m_left_top_lon;
    float m_lat_resolution;
    float m_lon_resolution;
    float m_height;
    float m_width;
    float m_maxValue;
    float m_minValue;
    QVector<float> m_data;
    QVector<QVector3D> m_colorMap;

    QString m_img_path;
};

#endif // OMGRASTER_H
