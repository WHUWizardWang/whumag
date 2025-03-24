#ifndef WHUMAG_DATAPROCESSING_ACCURACY_H
#define WHUMAG_DATAPROCESSING_ACCURACY_H
#include <string>
#include <vector>
#include <cmath>
#include <limits>
#include <numeric>
#include <iostream>
#include <QFile>
#include <QTextStream>
#include "DataStruct.h"
#include "referencemap/GeomagneticModel.h"
namespace Geomagnetic {
    class ComplexPoint;
}

class Accuracy
{
public:
    //读文件
    void readData(const QString& filename, Geomagnetic::Datapoint& dataPoints);
    bool readDatawithComplexity(const QString& filename, QVector<Geomagnetic::ComplexPoint>& result);
    void dataResult(const Geomagnetic::Datapoint& dataPoints, QString filename);
    double computeCheckLineAccuracy(const QString& backgroundDataFile,
                                    const QString& checkLineDataFile,
                                    const QString& complexityDataFile,
                                    double targetSpacing = 10.0,
                                    double spacingTolerance = 0.1,
                                    double searchRadius = 0.002);
    //计算函数
    void processData(const QString& filename1, const QString& filename2);
    // 趋势线对齐和误差计算
    std::tuple<double, std::vector<double>, double> removeTrendLine(
        const std::vector<double>& v1,
        const std::vector<double>& v2
        );
private:
    // 过滤矩阵函数 (如果需要)
    Geomagnetic::Datapoint filterGeoMagneticData(
        const Geomagnetic::Datapoint& inputData,
        double minLon = 1.070, double maxLon = 2.07,
        double minLat = 0.89, double maxLat = 1.89
        );

public:
    // 移动平均函数
    std::vector<double> movingAverage(const std::vector<double>& data, int windowSize);


    // 计算RMSE
    double calculateRMSE(const std::vector<double>& v1, const std::vector<double>& v2);

};
#endif // ACCURACY_H

