#ifndef SUBAREABLOCKS_H
#define SUBAREABLOCKS_H

#include "DataStruct.h"
#include "ReadData.h"
#include <QVector>
#include <QDebug>
#include "referencemap/GeomagneticModel.h"
#include "MagAno/OmgValidator.h"
#include "dataprocessing/mergeform.h"
#include "draw/draw_form.h"
using namespace Geomagnetic;



class subareaBlocks
{
    struct ijBound
    {
        int i_min,i_max;
        int j_min,j_max; // 分区的起始和结束的ij
    };

public:
    subareaBlocks();
    void cal_rc(Datapoint &datapoint);                                      // 读取输入数据的行列数
    double calculateStandardDeviation(const QVector<double>& data);         // 计算标准差
    void createBounds();                                                    // 获取分区的起始和结束的ij
    void dp2anop(Datapoint &datapoint);                                     // datapoint -> anopoint
    void createSubAll(Datapoint &datapoint);                                // 临时：将抽稀前的数据分区
    Datapoint anop2dp(ijBound ij);                                          // anopoint -> datapoint
    void subStd();                                                          // 计算分区的标准差
    QVector<double>  extractSubRange(ijBound ij);                           // 根据分区起始和结束的ij获取分区内的数据
    void subModel(Datainfo datainfo);                                       // 分区内建模
    void create_dp_result();                                                // 构建结果模板
    void submerge(Datapoint &all);                                                        // 分区融合
    void subAccuracy(Datapoint &all);                                       // 各分区的精度评价
    double distanceBetween(SinglePoint p1,AnoPoint p2);                     // 计算两点间的距离
    SinglePoint findNearestPoint(Datapoint input, AnoPoint p);              // 找到距离最近的点
    void out2file(QString filepath);                                        // 输出到文件
    void subarea(Datapoint &all,Datainfo datainfo,Datapoint &datapoint);  // 分区
    void build(Datapoint &all,Datainfo datainfo,Datapoint &datapoint,QString filepath);  // 建模


    double x_step;          // 输入数据的x间隔
    double y_step;          // 输入数据的y间隔
    int row_in;             // 输入数据行数
    int col_in;             // 输入数据列数
    int extractEveryNthRow; // 每n行抽取一行
    int row_overlap;        // 行重叠度
    int col_overlap;        // 列重叠度
    int row_blockCount;     // 分成几行
    int col_blockCount;     // 分成几列
    double rms;             // 总体rms

    QVector<QVector<AnoPoint>> dataInput;           // 输入数据
    QVector<QVector<AnoPoint>> dataInput_all;       // 输入数据all
    QVector<QVector<QVector<AnoPoint>>> subInput;   // 各分区输入数据
    QVector<QVector<ijBound>> ijBounds;             // ij范围
    QVector<QVector<double>> substd;                // 每个区域的标准差
    QVector<QVector<Datapoint>> datapoint_result;   // 各分区结果 Datapoint
    QVector<AnoPoint> result;                       // 结果 AnoPoint
    QVector<QVector<double>> subrms;                // 各分区的rms               // 各分区的std
    QString outstr;                                 // 输出到界面中的文字提示
    int inputPara_subarea(QDialog &dialog,int &data_num, QStringList &data_name_list,int &data_index,QString &dir);  // 分区输入参数
};

#endif // SUBAREABLOCKS_H
