#ifndef GEOMAGNETISMPROJECT_H
#define GEOMAGNETISMPROJECT_H

#include <QObject>
#include <QVector>
#include <QFile>
#include <QDebug>
#include "project.h"

class GeoMagnetismProject : public Project
{
public:
    explicit GeoMagnetismProject(Project *parent = nullptr);
    ~GeoMagnetismProject();

    QStringList nameList_real;          // 实测文件列表
    QStringList nameList_processed;     // 处理后文件列表
//    QVector<double> vec_dx;             // 经度（X方向）分辨率
//    QVector<double> vec_dy;             // 纬度（Y方向）分辨率

signals:

private:


};

bool copyFile(const QString &sourceFile, const QString &destFile);

#endif // GEOMAGNETISMPROJECT_H
