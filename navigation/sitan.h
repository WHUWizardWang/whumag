#pragma once
#ifndef _SITAN_H_
#define _SITAN_H_

#include <iostream>
#include "DataStruct.h"
#include "mainwindow.h"
#include "tercom.h"
#include "function.h"

namespace Geomagnetic {

/// 卡尔曼滤波器：用于将磁测量值与背景场梯度模型结合，修正惯导漂移
class KalmanFilter
{
public:
    /// 构造：传入背景场格网
    KalmanFilter() = default;
    explicit KalmanFilter(const QVector<QVector<double>>& backgroundField);

    /// 初始化状态向量 x（例如位置误差和偏航角误差等）
    void init(const VectorXd& initialState);

    /// 预测步骤：根据 controlInput（控制输入，例如上一次位置）
    void predict(const VectorXd& controlInput);

    /// 更新步骤：输入最新测量值 measurement 与控制输入，返回更新后的状态
    VectorXd update(double measurement, const VectorXd& controlInput);

private:
    MatrixXd       P;  ///< 状态协方差矩阵
    VectorXd       x;  ///< 状态向量
    MatrixXd       F;  ///< 状态转移矩阵
    RowVector2d    H;  ///< 观测矩阵，将状态映射到测量值
    MatrixXd       R;  ///< 测量噪声协方差
    MatrixXd       Q;  ///< 过程噪声协方差

    QVector<QVector<double>> background;
    /// 在当前位置 currentCoordinates 周围做局部拟合，计算背景场基准值 c 及梯度 a、b
    void getGeomagneticIntensity(const Vector2d& currentCoordinates,
                                 const QVector<QVector<double>>& backgroundField,
                                 double& a, double& b, double& c);
};

/// SITAN 匹配导航主类
class SitanMatching
{
public:
    /// 默认构造
    SitanMatching();

    /// 从背景场文件构造，同时初始化卡尔曼滤波器
//    explicit SitanMatching(const QString& backgroundFilePath);
    SitanMatching(const QString &filePath, QWidget *parent)
        : background(ReadBackground(filePath)),
        kf(background) // 这里直接初始化kf
    {
        customPlot = new QCustomPlot(parent); // parent可以是MainWindow或其它
    }

    ~SitanMatching() {
        delete customPlot;
    }

    /// 读取背景场格网：返回二维矩阵 [i][j] 对应 (x_i, y_j) 位置的磁场值
    QVector<QVector<double>> ReadBackground(const QString& filePath);

    /// 读取 INS 观测数据，返回 INSData 列表
    QVector<INSData> ReadINS(const QString& filePath);

    /// 将点集 p 写入文本文件 file，格式：x,y
    void totxt(const QVector<QPointF>& p, const QString& file);

    /// 将计算结果可视化：绘制背景热力图、真实轨迹、SITAN 匹配轨迹及 INS 轨迹
    void drawResult(const QVector<QPointF>& X,
                    const QVector<QVector<double>>& matrix,
                    const QVector<QPointF>& Real,
                    const QString& backgroundFile,
                    const QVector<INSData>& insdata);

    /// 主流程 1：给定背景文件、INS 文件、真实轨迹文件，执行 SITAN 算法
    void SITANAlgorithm(const QString& backgroundFile,
                        const QString& insFile,
                        const QString& realFile);

    /// 主流程 2：给定背景文件、TERCOM 结果文件、INS 文件、真实轨迹文件，执行 SITAN 算法
    void SITANAlgorithm(const QString& backgroundFile,
                        const QString& tercomResultFile,
                        const QString& insFile,
                        const QString& realFile);

    /// 主流程 3：直接传入背景矩阵和 INS 数据，返回匹配后的坐标序列
    QVector<QPointF> SITANAlgorithm(const QVector<QVector<double>>& background,
                                    const QVector<INSData>& insdata);

    QCustomPlot* customPlot = nullptr;  ///< 绘图库句柄，drawResult 中创建
    QVector<QVector<double>>   background;  ///< 原始背景场格网
private:
    KalmanFilter               kf;          ///< 内部卡尔曼滤波器
    double                     xmin = 0;    ///< 背景场 X 方向最小值
    double                     xmax = 0;    ///< 背景场 X 方向最大值
    double                     ymin = 0;    ///< 背景场 Y 方向最小值
    double                     ymax = 0;    ///< 背景场 Y 方向最大值
    QVector<double>            x_bg, y_bg, z_bg;  ///< 原始背景场采样点，用于备用

    // 你可以在此处继续添加私有辅助方法和成员
};

}
#endif // !_SITAN_H_
