#include "omgqmlpoint.h"

OmgQmlPoint::OmgQmlPoint(QObject *parent): m_lat(0.0f), m_lon(0.0f), m_red(0.0f), m_green(0.0f), m_blue(0.0f)
{

}

OmgQmlPoint::OmgQmlPoint(const OmgQmlPoint &other)
    : m_lat(other.lat())
    , m_lon(other.lon())
    , m_red(other.red())
    , m_green(other.green())
    , m_blue(other.blue())
{

}

OmgQmlPoint::OmgQmlPoint(qreal pLat, qreal pLon, qreal pRed, qreal pGreen, qreal pBlue)
    : m_lat(pLat)
    , m_lon(pLon)
    , m_red(pRed)
    , m_green(pGreen)
    , m_blue(pBlue)
{

}

OmgQmlPoint::~OmgQmlPoint()
{

}

qreal OmgQmlPoint::lat() const
{
    return m_lat;
}
qreal OmgQmlPoint::lon() const
{
    return m_lon;
}
qreal OmgQmlPoint::red() const
{
    return m_red;
}
qreal OmgQmlPoint::green() const
{
    return m_green;
}
qreal OmgQmlPoint::blue() const
{
    return m_blue;
}

void OmgQmlPoint::setLat(qreal pLat)
{
    m_lat = pLat;
}
void OmgQmlPoint::setLon(qreal pLon)
{
    m_lon = pLon;
}
void OmgQmlPoint::setRed(qreal pRed)
{
    m_red = pRed;
}
void OmgQmlPoint::setGreen(qreal pGreen)
{
    m_green = pGreen;
}
void OmgQmlPoint::setBlue(qreal pBlue)
{
    m_blue = pBlue;
}
