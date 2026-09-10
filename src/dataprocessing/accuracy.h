#ifndef WHUMAG_DATAPROCESSING_ACCURACY_H
#define WHUMAG_DATAPROCESSING_ACCURACY_H
#include <string>
#include <vector>
#include <iomanip>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <cfloat>
#include <climits>
#include <limits>
#include <numeric>
#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QObject>
#include <atomic>
#include <mutex>
#include <future>
#include "DataStruct.h"
#include "referencemap/GeomagneticModel.h"
namespace Geomagnetic {
struct ComplexPoint;
}


class Accuracy : public QObject
{
    Q_OBJECT
public:
    explicit Accuracy(QObject *parent = nullptr);
    //读文件
    void readData(const QString& filename, Geomagnetic::Datapoint& dataPoints);

    bool readDatawithComplexity(const QString& filename, QVector<Geomagnetic::ComplexPoint>& result);

    void dataResult(const Geomagnetic::Datapoint& dataPoints, QString filename);
    void dataResult_for_autoreferencemap(const Geomagnetic::Datapoint& dataPoints, int height, QString filename);

    double computeCheckLineAccuracy(const QString& backgroundDataFile,
                                    const QString& checkLineDataFile,
                                    const QString& complexityDataFile,
                                    double targetSpacing = 10.0,
                                    double spacingTolerance = 0.1,
                                    double searchRadius = 0.002);

    double computeSparseCheckLineRMSE(const QStringList& checkLineDataFile,
                                      const QString& complexityDataFile,
                                      double searchRadius = 1000);

    //计算函数
    void processData(const QString& filename1, const QString& filename2);
    // 趋势线对齐和误差计算
    std::tuple<double, std::vector<double>, double> removeTrendLine(
        const std::vector<double>& v1,
        const std::vector<double>& v2
        );

    void setMinMaxLonLat(double minLon, double maxLon, double minLat, double maxLat) {
        this->minLon = minLon;
        this->maxLon = maxLon;
        this->minLat = minLat;
        this->maxLat = maxLat;
    }

private:

    double minLon = 111.070; // 默认最小经度
    double maxLon = 112.07;  // 默认最大经度
    double minLat = 17.89;  // 默认最小纬度
    double maxLat = 18.89;  // 默认最大纬度
    // 过滤矩阵函数 (如果需要)

    QVector<Geomagnetic::ComplexPoint> filterGeoMagneticData(
        const QVector<Geomagnetic::ComplexPoint>& inputData,
        double minLon = 111.070, double maxLon = 112.07,
        double minLat = 17.89, double maxLat = 18.89
        );
    Geomagnetic::Datapoint filterGeoMagneticData(
        const Geomagnetic::Datapoint& inputData,
        double minLon = 111.070, double maxLon = 112.07,
        double minLat = 17.89, double maxLat = 18.89
        );

public:
    // 移动平均函数
    std::vector<double> movingAverage(const std::vector<double>& data, int windowSize);

    // 计算RMSE
    double calculateRMSE(const std::vector<double>& v1, const std::vector<double>& v2);

};

class HierarchicalGridIndex {
private:
    struct GridLevel {
        double cell_size;
        int width, height;
        double min_x, min_y;
        std::vector<std::vector<std::vector<size_t>>> cells;

        GridLevel(double size, double minX, double minY, double maxX, double maxY)
            : cell_size(size), min_x(minX), min_y(minY) {
            width = static_cast<int>((maxX - minX) / cell_size) + 1;
            height = static_cast<int>((maxY - minY) / cell_size) + 1;
            cells.resize(height, std::vector<std::vector<size_t>>(width));
        }
    };

    std::vector<GridLevel> levels;
    std::vector<std::pair<double, double>> points; // 存储原始点坐标
    double center_latitude; // 存储中心纬度用于坐标转换

public:
    void buildIndex(const Geomagnetic::Datapoint& sparsableDataPoints, double centerLat) {
        center_latitude = centerLat;

        // 计算边界
        double min_x = DBL_MAX, max_x = -DBL_MAX;
        double min_y = DBL_MAX, max_y = -DBL_MAX;

        points.clear();
        points.reserve(sparsableDataPoints.size());

        // 转换坐标并计算边界
        for (const auto& pair : sparsableDataPoints) {
            double x_m = pair.second.X * 111319 * std::cos(centerLat * M_PI / 180.0);
            double y_m = pair.second.Y * 111319;
            points.emplace_back(x_m, y_m);

            min_x = std::min(min_x, x_m);
            max_x = std::max(max_x, x_m);
            min_y = std::min(min_y, y_m);
            max_y = std::max(max_y, y_m);
        }

        // 扩展边界以确保完整覆盖
        double margin = 1000.0; // 1km边界
        min_x -= margin;
        max_x += margin;
        min_y -= margin;
        max_y += margin;

        // 创建多层网格（从粗到细）
        levels.clear();
        std::vector<double> cell_sizes = {5000.0, 2000.0, 1000.0, 500.0}; // 米

        for (double cell_size : cell_sizes) {
            levels.emplace_back(cell_size, min_x, min_y, max_x, max_y);
        }

        // 填充网格
        for (size_t i = 0; i < points.size(); ++i) {
            for (auto& level : levels) {
                int grid_x = static_cast<int>((points[i].first - level.min_x) / level.cell_size);
                int grid_y = static_cast<int>((points[i].second - level.min_y) / level.cell_size);

                if (grid_x >= 0 && grid_x < level.width &&
                    grid_y >= 0 && grid_y < level.height) {
                    level.cells[grid_y][grid_x].push_back(i);
                }
            }
        }
    }

    bool isInSparsableArea(double check_x, double check_y, double searchRadius) {
        // 转换检查点坐标
        double check_x_m = check_x * 111319 * std::cos(center_latitude * M_PI / 180.0);
        double check_y_m = check_y * 111319;

        // 选择合适的网格层级（选择网格大小接近搜索半径的层级）
        int level_idx = levels.size() - 1; // 默认使用最细的层级
        for (size_t i = 0; i < levels.size(); ++i) {
            if (levels[i].cell_size <= searchRadius * 1.5) {
                level_idx = i;
                break;
            }
        }

        const auto& level = levels[level_idx];
        int grid_x = static_cast<int>((check_x_m - level.min_x) / level.cell_size);
        int grid_y = static_cast<int>((check_y_m - level.min_y) / level.cell_size);

        int search_range = static_cast<int>(searchRadius / level.cell_size) + 1;

        // 搜索邻近网格
        for (int dy = -search_range; dy <= search_range; dy++) {
            for (int dx = -search_range; dx <= search_range; dx++) {
                int nx = grid_x + dx;
                int ny = grid_y + dy;

                if (nx >= 0 && nx < level.width && ny >= 0 && ny < level.height) {
                    const auto& cell_points = level.cells[ny][nx];
                    if (!cell_points.empty()) {
                        // 精确距离检查
                        for (size_t point_idx : cell_points) {
                            double dx_m = check_x_m - points[point_idx].first;
                            double dy_m = check_y_m - points[point_idx].second;
                            double distance = std::sqrt(dx_m * dx_m + dy_m * dy_m);

                            if (distance <= searchRadius) {
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    // 获取统计信息
    void printStatistics() const {
        for (size_t i = 0; i < levels.size(); ++i) {
            const auto& level = levels[i];
            int non_empty_cells = 0;
            int total_points_in_level = 0;

            for (const auto& row : level.cells) {
                for (const auto& cell : row) {
                    if (!cell.empty()) {
                        non_empty_cells++;
                        total_points_in_level += cell.size();
                    }
                }
            }

            std::cout << "Level " << i << " (cell_size=" << level.cell_size << "m): "
                      << non_empty_cells << " non-empty cells, "
                      << total_points_in_level << " total point references" << std::endl;
        }
    }
};

#endif // ACCURACY_H

