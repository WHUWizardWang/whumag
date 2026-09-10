#ifndef OMGQMLPOLYGON_H
#define OMGQMLPOLYGON_H

#include <QObject>
#include <QMetaType>
#include <QVariant>
#include <QVariantList>
#include <QDir>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QGeoCoordinate>
#include <QVector>

#include "omgqmlpoint.h"
#include "omgpolygon.h"
#include "omgraster.h"

typedef struct{
    QString name;
    QString color;
    int width;
    QVector<QGeoCoordinate> pos;
public:
    QString toJsonString() const;
}VesselPath;

class OmgQmlPolygon: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal latRsl READ latRsl WRITE setLatRsl)
    Q_PROPERTY(qreal lonRsl READ lonRsl WRITE setLonRsl)
    Q_PROPERTY(qreal maxLat READ maxLat WRITE setMaxLat)
    Q_PROPERTY(qreal minLon READ minLon WRITE setMinLon)

public:
    OmgQmlPolygon(QObject *parent = nullptr);
    OmgQmlPolygon(const OmgQmlPolygon &polygon) = delete;
    OmgQmlPolygon &operator=(const OmgQmlPolygon &polygon) = delete;
    ~OmgQmlPolygon();

    qreal latRsl() const;
    qreal lonRsl() const;
    qreal maxLat() const;
    qreal minLon() const;

    void setLatRsl(qreal lat_rsl);
    void setLonRsl(qreal lon_rsl);
    void setMaxLat(qreal max_lat);
    void setMinLon(qreal min_lon);

    OmgQmlPoint pointConvertToQml(const OmgGeoPoint &geoPnt);
    void addPoint(const OmgQmlPoint &pnt);

    Q_INVOKABLE void addNode(qreal lat, qreal lon);
    Q_INVOKABLE void clearNodes();

    Q_INVOKABLE void getInertnalPoints(int flag);
    Q_INVOKABLE void setRadio(qreal radio);
    Q_INVOKABLE QString getResImgPath();
    Q_INVOKABLE QString loadChinaBorder();
    void addVesselPath(const VesselPath& vessel_path);
    void removeVesselPath(const QString& vessel_name);

signals:
    void sigAddVesselPath(const QString& json_str);
    void sigRemoveVesselPath(const QString& vessel_name);

private:
    void calibrateLon(QVector<Vec2d> &POL);

    const QString g_emag2_path = QCoreApplication::applicationDirPath() + "/data/EMAG2_V3_SeaLevel_DataTiff.tif";
    const QString g_mamea_path = QCoreApplication::applicationDirPath() + "/data/mamea20.xym";
    const QString g_image_path = QCoreApplication::applicationDirPath() + "/images/region.png";

    qreal m_lat_rsl;
    qreal m_lon_rsl;
    qreal m_max_lat;
    qreal m_min_lon;
    qreal m_radio;
    QVariantList m_points;
    QVector<Vec2d> m_polygon;
    OmgRaster m_raster_emag2;   // tiff
    OmgRaster m_raster_mamea;   // xym
};

//Q_DECLARE_METATYPE(OmgQmlPolygon);

#endif // OMGQMLPOLYGON_H
