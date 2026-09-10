#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cmath>
#include <QFile>
#include "DataStruct.h"


#define p_0 206264.8062470963551564

namespace Geomagnetic {

struct linePoints
{
    //原始数据文件内容
    int docID;          //数据文件ID
    int year;           //日期 年
    int month;          //日期 月
    int day;            //日期 日
    int hour;           //时间 时
    int min;            //时间 分
    double sec;         //时间 秒
    double Tm;          //地磁测量值
    double G;           //信号强度
    double depth;       //探头深度
    double L;           //GNSS天线经度
    double B;           //GNSS天线纬度
    double V;           //航速
    double heading;     //航向
    double T1;          //正常场
    double v;           //日变值
    double T2;          //船磁
    double adjust;      //调差值
    double T3;          //异常场

    //统一数据管理（datapoints）
    int gpsWeek;
    double gpsSeconds;
    double xMagnetic;
    double yMagnetic;
    double zMagnetic;
    double tMagnetic;
    double lat;
    double lon;
    double height;
    double X;
    double Y;
    double Z;
    int cluster;
};

//wgs84参考椭球
const double e = 0.00669438002290;
const double e1 = 0.00673949677548;
const double b = 6356752.3141;
const double a = 6378137.0;

    class TimeTrans {
    public:
        // Convert calendar date to Julian Date
        double calendarDateToJulianDate(int year, int month, int day, int hour, int minute, int second);
        // Convert Julian Date to GPS week and seconds of the week
        void JulianDateToGPS(double JD, int& gpsWeek, double& gpsSeconds);
    };
    class CoordTrans
    {
    public:
        void BLH2XYZ(Datapoint& datapoint);
        void XYZ2BLH(Datapoint& datapoint);
        void BL2XY(Datapoint& datapoint);
    };
    class ReadData
    {
    public:
        bool readGridFromFile(const std::string& filename, Datapoint& datapoints);
        void DataSet(Datapoint& datapoiint, Datainfo& datainfo);
        void selectRandomData(const Datapoint& allData, Datapoint& data_sparse, int n=10);
        void selectLineData(const Datapoint& allData, Datapoint& train, Datapoint& test,int n);
        void selectLineData(const Datapoint& allData, Datapoint& train, int n);
        void resultOut(const Datapoint& dataresult, const std::string& filename);
        void DadiPoint2ProjectPoint(double B, double L,double &x,double &y);
        int ReadLines(QStringList FileList,QString savepath);
        bool createGridData(const Datapoint& datapoints, Datapoint& dataresult, double dx, double dy);
        Datapoint setDataResult(Datapoint& datapoint, double interval);
    public:
        int interval = 10;
        std::vector<linePoints> linepoints;
        std::vector<std::vector<linePoints>> lines;

    };
   

}
