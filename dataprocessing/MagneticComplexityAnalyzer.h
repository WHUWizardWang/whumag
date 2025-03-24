#ifndef MAGNETICCOMPLEXITYANALYZER_H
#define MAGNETICCOMPLEXITYANALYZER_H
#include <vector>
#include"DataStruct.h"
#include <cmath>
#include <set>
#include <algorithm>
#include <eigen-3.4.0/Eigen/Dense>
#include <numeric>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QAtomicInt>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include "draw/qcustomplot.h"
#include "dataprocessing/accuracy.h"
using namespace Eigen;
namespace Geomagnetic {
// 子区特征参数
struct SubArea
{
    std::vector<double> magneticValues;
    double meanValue;
    double stdDev;
    double gradientMean;
    double kurtosis;
    double slopeStdDev;
    double roughness;
};
// 网格数据结构
struct GridCell {
    std::vector<SinglePoint> points;  // 存储原始数据点
    double meanMagnetic;        // 网格平均磁场值
    double gradLon;              // 经度方向梯度
    double gradLat;              // 纬度方向梯度
    double gradNE;               // 东北方向梯度
    double gradSE;               // 东南方向梯度
    double gradientStdDev;       // 该点的梯度标准差
    bool hasData;             // 是否有数据标志
    double complexity;          // 该点的复杂度
    double surveySpacing;       // 该点对应的测线间距
    GridCell() : meanMagnetic(0), gradLon(0), gradLat(0), gradNE(0), gradSE(0),
        gradientStdDev(0), hasData(false), complexity(0), surveySpacing(0) {}
};
// 网格数据结构
struct GridData {
    std::vector<std::vector<GridCell>> grid;
    double cellSize;
    double lonMin, lonMax, latMin, latMax;
    int rows, cols;

};
struct ComplexityResult {
    GridData gridData;                      // 处理后的网格数据
    std::vector<double> complexityValues;   // 复杂度值
    std::vector<double> spacingValues;      // 测线间距值
    std::vector<int> rowIndices;            // 对应的行索引
    std::vector<int> colIndices;            // 对应的列索引
};
struct ComplexPoint
{
    double lon;         // 真实经度
    double lat;         // 真实纬度
    double complexity;  // 复杂度
    double spacing;     // 测线间距
    double baseMag;     // 基础磁场值
};
// 评估结果结构
struct ComplexityEvalResult {
    std::vector<ComplexPoint> backgroundPoints; // 背景数据点
    std::vector<ComplexPoint> checkLinePoints;  // 检查线对应的背景点
    double rmsForSpacing10;                    // 测线间距为10的点的RMS
    size_t numPointsForRms;                    // 用于计算RMS的点数
};
// 磁场复杂度分析器
class MagneticComplexityAnalyzer {
public:
    MagneticComplexityAnalyzer();
    ~MagneticComplexityAnalyzer();
    // 主处理函数 - 返回复杂度分析结果
    ComplexityResult analyzeComplexity(const Datapoint& data, double gridSize,int jumpSize=15,int stepSize=1);
    ComplexityResult analyzeComplexityChunked(const Datapoint& data, double gridSize, int jumpSize = 15,
                                              int chunkSize = 1000,bool air=false);
    ComplexityResult airanalyzeComplexity(const Datapoint& data, double gridSize,int jumpSize=15,int stepSize=1);
    double computeCheckLineAccuracy(const Geomagnetic::Datapoint & backgroundData,
                                    const Geomagnetic::Datapoint & checkLineData,
                                    double gridSize,
                                    int jumpSize,
                                    int stepSize);
    std::vector<ComplexPoint> convertIndicesToGeo(const ComplexityResult &result);
    // 生成并显示复杂度热图
    void showComplexityMap(const ComplexityResult& result,int jumpSize);
    // 生成并显示测线间距热图
    void showSpacingMap(const ComplexityResult& result,int jumpSize);
    // 将复杂度结果输出到文件
    bool exportToFile(const ComplexityResult& result, const QString& filename);
    // 从结果生成热图 (可供外部使用)
    void plotComplexityMap(QCustomPlot* customPlot, const ComplexityResult& result, int jumpSize, bool showSpacing = false);


    // 主成分分析和特征融合
    std::vector<double> calculateComplexity(const std::vector<SubArea>& subAreas);
    void setSUBAREA_SIZE(int &subarea_size);

    // 主处理函数
    std::vector<double> processData(const Datapoint& data, double gridSize,GridData& gridData);
    void plotComplexityMap(QCustomPlot* customPlot,const GridData& gridData,const std::vector<double>& complexityValues);

private:
    int SUBAREA_SIZE; // 子区大小(个数）
    const int FEATURE_COUNT;   // 6个特征参数
    // 格网化函数
    GridData createGrid(const Datapoint& data, double gridSize);
    void interpolateEmptyGrids(GridData& gridData);
    void calculateGridGradients(GridData& gridData);
    Accuracy acc;


    // 计算子区的特征参数
    SubArea calculateSubAreaFeatures(const std::vector<Geomagnetic::SinglePoint>& points);
    SubArea calculateSubAreaFeaturesFromGrid(const GridData& gridData,
                                             int centerRow, int centerCol);
    std::vector<double> calculateComplexity(const std::vector<SubArea>& subAreas,
                                            GridData& gridData,
                                            const std::vector<int>& rowIndices,
                                            const std::vector<int>& colIndices);

    //辅助函数
    double mapComplexityToSpacing(double complexity);
    double airComplexityToSpacing(double complexity);
    bool isInSubArea(const SinglePoint& center, const SinglePoint& point, double size);
    void interpolateAndCalculateGradients(GridData& gridData);
    void collectSubAreaFeatures(const GridData& gridData, int centerRow, int centerCol, SubArea& outSubArea);
//    void calculateComplexityInPlace(const std::vector<SubArea>& subAreas,GridData& gridData,const std::vector<int>& rowIndices,const std::vector<int>& colIndices,std::vector<double>& outComplexity);

};

}
#endif // MAGNETICCOMPLEXITYANALYZER_H
