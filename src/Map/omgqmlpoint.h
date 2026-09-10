#ifndef OMGQMLPOINT_H
#define OMGQMLPOINT_H

#include <QObject>
#include <QMetaType>

class OmgQmlPoint: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal lat READ lat WRITE setLat)
    Q_PROPERTY(qreal lon READ lon WRITE setLon)
    Q_PROPERTY(qreal red READ red WRITE setRed)
    Q_PROPERTY(qreal green READ green WRITE setGreen)
    Q_PROPERTY(qreal blue READ blue WRITE setBlue)

public:
    OmgQmlPoint(QObject *parent = nullptr);
    OmgQmlPoint(const OmgQmlPoint &other);
    OmgQmlPoint(qreal pLat, qreal pLon, qreal pRed, qreal pGreen, qreal pBlue);
    ~OmgQmlPoint();

    qreal lat() const;
    qreal lon() const;
    qreal red() const;
    qreal green() const;
    qreal blue() const;

    void setLat(qreal pLat);
    void setLon(qreal pLon);
    void setRed(qreal pRed);
    void setGreen(qreal pGreen);
    void setBlue(qreal pBlue);



private:
    qreal m_lat;
    qreal m_lon;
    qreal m_red;
    qreal m_green;
    qreal m_blue;
};

Q_DECLARE_METATYPE(OmgQmlPoint);

#endif // OMGQMLPOINT_H
