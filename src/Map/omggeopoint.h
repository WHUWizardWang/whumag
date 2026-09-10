#ifndef OMGGEOPOINT_H
#define OMGGEOPOINT_H

//#include <QObject>

class OmgGeoPoint// : public QObject
{
    //    Q_OBJECT
public:
    //    explicit OmgGeoPoint(QObject *parent = nullptr);
    OmgGeoPoint();
    OmgGeoPoint(float lat, float lon, float value, float red, float green, float blue);


public:
    float lat() const;
    float lon() const;
    float value() const;
    float red() const;
    float green() const;
    float blue() const;



private:

    float m_lat, m_lon, m_v, m_red, m_green, m_blue;

};

#endif // OMGGEOPOINT_H
