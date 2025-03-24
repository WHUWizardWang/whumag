#pragma once

#include <iostream>
#include <QDate>
#include <QFile>
#include <QDebug>
#include "database/databasemanager.h"
#include "wmm/GeomagnetismHeader.h"
#include "MagAno/OmgValidator.h"
#include "wmm/igrf_point.h"


typedef struct
{
    double lon, lat, mag;
} xPoint;

class TimeTongHua
{
public:
	TimeTongHua();
	~TimeTongHua();

    QDate date0;
    QDate date1;

    void ReadData(QString FileName);
    void GetIGRFData(int useGeoid,double height);
    void CalMag(QString FileName,int useGeoid,double height,QDate date0,QDate date1,QString dir);
    void out2file(QString dir);

    QVector<double> data_igrf0;
    QVector<double> data_igrf1;
    QVector<xPoint> data_real;
    QVector<xPoint> data_tonghua;

private:


};


