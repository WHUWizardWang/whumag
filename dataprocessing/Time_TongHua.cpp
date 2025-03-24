#include "Time_TongHua.h"

TimeTongHua::TimeTongHua()
{
}

TimeTongHua::~TimeTongHua()
{
}

void TimeTongHua::ReadData(QString FileName)
{
    QFile file(FileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        qWarning()<<"open file failed!";
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QStringList fields = line.split(QRegExp("\\s+|,"), QString::SkipEmptyParts);
        if (fields.size() != 3)
            continue;
        bool okX, okY, okZ;
        double xx = fields[0].toDouble(&okX);
        double yy = fields[1].toDouble(&okY);
        double zz = fields[2].toDouble(&okZ);
        xPoint temp;
        temp.lon = xx;
        temp.lat = yy;
        temp.mag = zz;
        data_real.append(temp);
    }
    file.close();
}

void TimeTongHua::GetIGRFData(int useGeoid,double height)
{
    MAGtype_CoordGeodetic *CoordGeodeticArr;
    MAGtype_Date *UserDateArr0,*UserDateArr1;
    int length = data_real.size();
    CoordGeodeticArr = new MAGtype_CoordGeodetic[length];
    UserDateArr0 = new MAGtype_Date[length];
    UserDateArr1 = new MAGtype_Date[length];
    //
    double date_temp0 =  date0.year() * 1.0 + date0.dayOfYear() * 1.0 / date0.daysInYear();
    double date_temp1 =  date1.year() * 1.0 + date1.dayOfYear() * 1.0 / date1.daysInYear();
    int index = 0;
    for (auto p : data_real)
    {
        MAGtype_CoordGeodetic coord;
        MAGtype_Date userdate0, userdate1;
        coord.phi = p.lat;
        coord.lambda = p.lon;
        coord.UseGeoid = useGeoid;
        coord.HeightAboveGeoid = height;
        if (!useGeoid)
        {
            coord.HeightAboveEllipsoid = coord.HeightAboveGeoid;
        }
        userdate0.DecimalYear = date_temp0;
        userdate0.Year = date0.year();
        userdate0.Month = date0.month();
        userdate0.Day = date0.day();
        userdate1.DecimalYear = date_temp1;
        userdate1.Year = date1.year();
        userdate1.Month = date1.month();
        userdate1.Day = date1.day();

        CoordGeodeticArr[index] = coord;
        UserDateArr0[index] = userdate0;
        UserDateArr1[index] = userdate1;
        index++;
    }
    MAGtype_GeoMagneticElements *GeoMagneticElementsArr0 = new MAGtype_GeoMagneticElements[length];
    MAGtype_GeoMagneticElements *ErrorsArr0 = new MAGtype_GeoMagneticElements[length];
    MAGtype_GeoMagneticElements *GeoMagneticElementsArr1 = new MAGtype_GeoMagneticElements[length];
    MAGtype_GeoMagneticElements *ErrorsArr1 = new MAGtype_GeoMagneticElements[length];
    omg_igrf(CoordGeodeticArr, UserDateArr0, GeoMagneticElementsArr0, ErrorsArr0, length);
    omg_igrf(CoordGeodeticArr, UserDateArr1, GeoMagneticElementsArr1, ErrorsArr1, length);
    for (int i = 0; i < length; ++i)
    {
        data_igrf0.append(GeoMagneticElementsArr0[i].F);
        data_igrf1.append(GeoMagneticElementsArr1[i].F);
    }
}

void TimeTongHua::CalMag(QString FileName,int useGeoid,double height,QDate date00,QDate date11,QString dir)
{
    date0 = date00;
    date1 = date11;
    ReadData(FileName);
    GetIGRFData(useGeoid,height);
    for (int i = 0; i < data_real.size(); ++i)
    {
        xPoint temp;
        temp.lat = data_real[i].lat;
        temp.lon = data_real[i].lon;
        temp.mag = - data_igrf0[i] + data_igrf1[i] + data_real[i].mag;
        data_tonghua.append(temp);
    }
    out2file(dir);
}

void TimeTongHua::out2file(QString dir)
{
    QFile file(dir);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "无法打开文件进行写入：" << file.errorString();
        }
    QTextStream out(&file);
    for(auto elem:data_tonghua)
    {
        out<<elem.lon<<" "<<elem.lat<<" "<<elem.mag<<endl;
    }
}
