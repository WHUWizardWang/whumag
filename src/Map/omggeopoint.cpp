#include "omggeopoint.h"

//OmgGeoPoint::OmgGeoPoint(QObject *parent) : QObject(parent)
//{

//}

OmgGeoPoint::OmgGeoPoint()
{

}

OmgGeoPoint::OmgGeoPoint(float lat, float lon, float value)
{
    m_lat = lat;
    m_lon = lon;
    m_v = value;
}

OmgGeoPoint::OmgGeoPoint(float lat, float lon, float value, float red, float green, float blue)
{
    m_lat = lat;
    m_lon = lon;
    m_v = value;
    m_red = red;
    m_green = green;
    m_blue = blue;
}

void OmgGeoPoint::setParameters(float lat, float lon, float value, float red, float green, float blue)
{
    m_lat = lat;
    m_lon = lon;
    m_v = value;
    m_red = red;
    m_green = green;
    m_blue = blue;
}

float OmgGeoPoint::lat() const
{
    return m_lat;
}
float OmgGeoPoint::lon() const
{
    return m_lon;
}
float OmgGeoPoint::value() const
{
    return m_v;
}
float OmgGeoPoint::red() const
{
    return m_red;
}
float OmgGeoPoint::green() const
{
    return m_green;
}
float OmgGeoPoint::blue() const
{
    return m_blue;
}

void OmgGeoPoint::setLat(float lat)
{
    m_lat = lat;
}
void OmgGeoPoint::setLon(float lon)
{
    m_lon = lon;
}
void OmgGeoPoint::setValue(float value)
{
    m_v = value;
}
void OmgGeoPoint::setRed(float red)
{
    m_red = red;
}
void OmgGeoPoint::setGreen(float green)
{
    m_green = green;
}
void OmgGeoPoint::setBlue(float blue)
{
    m_blue = blue;
}
