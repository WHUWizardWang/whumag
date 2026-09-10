#ifndef ICCP_H
#define ICCP_H
#pragma once

#include <algorithm>
#include <complex>
#include <QtGlobal>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QList>
#include <QLineF>
#include <eigen-3.4.0/Eigen/Dense>
#include "qcustomplot.h"
#include "function.h"
// 重载 << 运算符以支持 Eigen::Matrix3f 与 QDebug 的输出
QDebug operator<<(QDebug dbg, const Eigen::Matrix2d& matrix);

struct CountLine
{
    QVector<QLineF> line;
    double value;
};

struct magPoint
{
    QPointF point;
    double value;
};

class ICCP
{
public:
    ICCP() = default;
    ICCP(double x_step, double y_step)
        : dx(x_step), dy(y_step) {}
    ~ICCP() = default;

    // ===== 数据 I/O =====
    QVector<QVector<double>> ReadFile(const QString &fileName);                       // 读取散点数据
    QVector<QVector<double>> ReadBackground(const QString &filePath);                 // 读取背景网格
    QVector<magPoint>        ReadINS(const QString &filePath);                       // 读取 INS 数据
    void                     totxt(const QVector<QPointF> X, QString file);          // 导出结果到文本
    void                     setMinMax(double xmin, double xmax, double ymin, double ymax); // 设置网格范围
    void                     setDxDy(double x_step, double y_step);            // 设置网格分辨率

    // ===== 等值线格网与提取 =====
    QVector<QVector<bool>>   isoCellGrid(QVector<QVector<double>> data, double isoValue);               // 单元格角赋值
    int                      getCellBit(bool v1,bool v2,bool v3, bool v4);                               // 判断格型
    QVector<QVector<int>>    getCellShift(QVector<QVector<bool>> bit);                                    // 构建格型矩阵
    void                     getLines(CountLine &result, QVector<QVector<int>> matrix,double isoValue, QVector<QVector<double>> data); // 提取线段
    void                     draw_lines(CountLine lines,double m,double n);                               // 绘制线段

    // ===== 核心 ICP 接口 =====
    QVector<QPointF>         iccp(QVector<QVector<double>> data,const QVector<magPoint>& insP,double threshold);       // 执行 ICP
    QVector<QPointF>         cal(QString data,QString insP,QString real,double thr);                             // 简易调用1
    QVector<QPointF>         cal(QString data,QString insP,QString tercomResult,QString real,double thr);         // 简易调用2
    void                     outResult(QVector<QPointF> X);                                                         // 输出结果
    void                     drawResult(QVector<QPointF> X,QVector<QVector<double>> matrix,QVector<QPointF> Real,QVector<magPoint> insP); // 绘图

    // ===== 公用工具函数 =====
    QVector<QPointF>         eigenMatrixToQVector(const Eigen::MatrixXd& matrix);   // Eigen 转 QVector
    double                   azimuth(double dx, double dy);                        // 方位角计算

protected:
    // ===== 旋转与变换计算 =====
    QVector<QPointF>         computeRotationMatrix_2(QVector<QPointF> points1,QVector<QPointF> points2);
    QVector<QPointF>         computeRotationMatrix_3(QVector<QPointF> points1,QVector<QPointF> points2);
    QVector<QPointF>         computeRotationMatrix_4(QVector<QPointF> points1,QVector<QPointF> points2);
    void                     computeRotationMatrix(const QVector<QPointF>& insP, const QVector<QPointF>& PP,Eigen::Matrix2d &R,Eigen::MatrixXd &T);
    QVector<QPointF>         computeMatrixNew(const QVector<QPointF>& insP,Eigen::Matrix2d &R,Eigen::MatrixXd &T);
    double                   calculateDifferences(const QVector<QPointF>& points1, const QVector<QPointF>& points2);

    // ===== 最近点与质心计算 =====
    QPointF                  findNearestPointOnLine(const QLineF line, const QPointF p);
    QPointF                  findNearestPointOnLines(const CountLine result, const QPointF p);
    QVector<QPointF>         get_PP(QVector<QPointF> insP,const CountLine result);
    QPointF                  computeCentroid(const QVector<QPointF>& points);

private:
    // ===== 类成员变量 =====
    double                   xmax;       // 数据最大经度
    double                   xmin;       // 数据最小经度
    double                   ymax;       // 数据最大纬度
    double                   ymin;       // 数据最小纬度
    double                   dx;         // 网格经度分辨率
    double                   dy;         // 网格纬度分辨率
    int                      xSize;      // 网格列数
    int                      ySize;      // 网格行数
    QVector<double>          x_bg,y_bg,z_bg; // 背景网格数据
    double                   finalRMS;   // 最终均方根误差

public:
    QCustomPlot*             customPlot; // 绘图控件指针
};
#endif // ICCP_H
