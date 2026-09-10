#ifndef AUTONAV_H
#define AUTONAV_H

#include <vector>
#include <QString>
#include <QVector>
#include <QPointF>
#include "navigation/iccp.h"
#include "navigation/sitan.h"
#include "navigation/tercom.h"
#include "navigation/function.h"
#include "qcustomplot.h"

namespace Geomagnetic {

class AUTONAV {
public:
    AUTONAV() = default;
    ~AUTONAV() { delete customPlot; }

    /**
     * @brief runAll   一次性运行 TERCOM、ICCP、SITAN 以及 TERCOM+ICCP 四种匹配算法
     * @param backgroundFile   背景场文件路径 (文本文件，每行三列：x y 磁强度)
     * @param insFile          INS 观测文件路径 (CSV，每行至少三列：x,y,磁测量值)
     * @param truePathFile     真实轨迹文件路径 (每行两列：x y)
     */
    void runAll(const QString &backgroundFile,
                const QString &insFile,
                const QString &truePathFile);

    /**
     * @brief drawResult 将所有匹配结果绘制到同一张图中
     * @param iccpResult          ICCP（原始 INS）匹配结果
     * @param sitanResult         SITAN 匹配结果
     * @param tercomResult        TERCOM 匹配结果（Datapoint）
     * @param tercomIccpResult    TERCOM+ICCP 匹配结果
     */
    void drawResult(const QVector<QPointF> &iccpResult,
                    const QVector<QPointF> &sitanResult,
                    const Datapoint &tercomResult,
                    const QVector<QPointF> &tercomIccpResult);

private:
    // —— 三种算法的实例 ——
    ICCP            cp;  ///< ICCP 算法实例
    TercomMatching  tm;  ///< TERCOM 算法实例
    SitanMatching   sm;  ///< SITAN 算法实例

    // —— 原始数据载体 ——
    std::vector<MapData>    base;       ///< 背景场点集 (x, y, 磁强度)
    std::vector<INSData>    insData;    ///< INS 观测点集 (x, y, heading, 磁测量值)
    std::vector<TruePath>   truePath;   ///< 真实轨迹点集 (x, y)

    // —— 内部辅助接口 ——

    /**
     * @brief readBackgroundFile  读取背景场到 base
     * @param filePath
     */
    void readBackgroundFile(const QString &filePath);

    /**
     * @brief readINSFile         读取 INS 观测到 insData
     * @param filePath
     */
    void readINSFile(const QString &filePath);

    /**
     * @brief readTruePathFile    读取真实轨迹到 truePath
     * @param filePath
     */
    void readTruePathFile(const QString &filePath);

    /**
     * @brief convertBaseToGrid   将 base（vector<MapData>）转换为网格矩阵 QVector<QVector<double>>
     * @return
     */
    QVector<QVector<double>> convertBaseToGrid() const;

    /**
     * @brief convertInsToMagPts  将 insData 转换为 QVector<magPoint>
     * @return
     */
    QVector<magPoint> convertInsToMagPoints() const;

    /**
     * @brief convertDatapointToMagPoints  将 TERCOM 输出的 Datapoint 转换为 QVector<magPoint>
     * @param dp
     * @return
     */
    QVector<magPoint> convertDatapointToMagPoints(const Datapoint &dp) const;

public:
    // —— 绘图控件 ——
    QCustomPlot *customPlot = nullptr;
    double x_step = 0.5; ///< 网格经度分辨率
    double y_step = 0.5; ///< 网格纬度分辨率

};

} // namespace Geomagnetic

#endif // AUTONAV_H
