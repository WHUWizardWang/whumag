#ifndef SUBAREA_H
#define SUBAREA_H
#include "DataStruct.h"
#include "ReadData.h"
#include <QVector>
#include <QDebug>
#include "referencemap/GeomagneticModel.h"
#include "MagAno/OmgValidator.h"

using namespace Geomagnetic;
std::pair<int, int> cal_rc(Datapoint &datapoint,double x_step,double y_step);
double calculateStandardDeviation(const QVector<double>& data);
// 极差（Range）, 四分位距（Interquartile Range, IQR）, 方差（Variance） 和 标准差（Standard Deviation）,
// 变异系数（Coefficient of Variation, CV）, 绝对中位差（Median Absolute Deviation, MAD）
void cal_std(Datapoint &datapoint,std::pair<int, int> rc,int window_size,int multiple,
             QVector<QVector<double>> &result,QVector<QVector<Datapoint>> &datapoint_subarea,
             QVector<QVector<Datapoint>> &datapoint_result);

std::pair<double, double> findMinMax(const QVector<QVector<double>>& matrix);
void create_type(const QVector<QVector<double>>& matrix,QVector<QVector<double>>& result);
void subarea_build(Datainfo datainfo,std::pair<int, int> rc,int window_size,int multiple,
           QVector<QVector<double>>& type,QVector<QVector<Datapoint>> &datapoint_subarea,
                   QVector<QVector<Datapoint>> &datapoint_result);
void final_merge(QVector<AnoPoint> &ano_pnts, QVector<QVector<Geomagnetic::Datapoint>> &datapoint_result);


#endif // SUBAREA_H
