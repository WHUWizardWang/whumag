/**
 * @file GeomangeticModel.hpp
 * Base class for Geomangetic models
 */
#ifndef _GEOMANGETICMODEL_H_
#define _GEOMANGETICMODEL_H_
#include <cmath>
#include <vector>
#include <limits>
#include <chrono>
#include <algorithm>
#include <queue>
#include <utility>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include <QDebug>
#include <eigen-3.4.0/Eigen/Dense>
#include <eigen-3.4.0/Eigen/Sparse>
#include <omp.h>
#include "referencemap/KDTree.h"
#include "nanoflann.hpp"
#ifdef rad1
#undef rad1
#endif
#include"alglib/interpolation.h"
#include "DataStruct.h"

namespace Geomagnetic {
    using namespace Eigen;

    // Point cloud adapter for nanoflann
    struct PointCloudAdapter {
        const Datapoint& points;

        explicit PointCloudAdapter(const Datapoint& dp) : points(dp) {}

        // Must return the number of points
        inline size_t kdtree_get_point_count() const { return points.size(); }

        // Returns the distance between points (squared Euclidean)
        inline double kdtree_distance(const double* p1, const size_t idx_p2, size_t /*size*/) const {
            auto it = points.begin();
            std::advance(it, idx_p2);
            double dx = p1[0] - it->second.X;
            double dy = p1[1] - it->second.Y;
            return dx*dx + dy*dy;
        }

        // Returns the dim'th component of the idx'th point
        inline double kdtree_get_pt(const size_t idx, int dim) const {
            auto it = points.begin();
            std::advance(it, idx);
            return dim == 0 ? it->second.X : it->second.Y;
        }

        // Optional bounding box computation (not used in this implementation)
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
    };

    // 构建向量适配器 - 替换原来的map适配器
    class PointVectorAdapter {
    public:
        const std::vector<std::pair<double, double>>& points;

        PointVectorAdapter(const std::vector<std::pair<double, double>>& pts) : points(pts) {}

        inline size_t kdtree_get_point_count() const { return points.size(); }

        inline double kdtree_get_pt(const size_t idx, int dim) const {
            return dim == 0 ? points[idx].first : points[idx].second;
        }

        inline double kdtree_distance(const double* p1, const size_t idx_p2, size_t) const {
            double dx = p1[0] - points[idx_p2].first;
            double dy = p1[1] - points[idx_p2].second;
            return dx*dx + dy*dy;
        }

        template <class BBOX>
        bool kdtree_get_bbox(BBOX&) const { return false; }
    };

	class GeomangeticModel
	{
	public:
		//Distructor
		virtual ~GeomangeticModel(){}

        double lon_max;
        double lon_min;
        double lat_max;
        double lat_min;

        // 新增归一化所需的均值和标准差
        double mean_lon, std_lon;
        double mean_lat, std_lat;

		//Return validity of model
		bool isValid(void)
		{
			return valid;
		}
        void setMinMax(Datapoint& datapoint)
        {
            lon_max = std::numeric_limits<double>::min();
            lon_min = std::numeric_limits<double>::max();
            lat_max = std::numeric_limits<double>::min();
            lat_min = std::numeric_limits<double>::max();
            for (auto& entry : datapoint)
            {
                lon_max = std::numeric_limits<double>::lowest();
                lon_min = std::numeric_limits<double>::max();
                lat_max = std::numeric_limits<double>::lowest();
                lat_min = std::numeric_limits<double>::max();
                for (auto& entry : datapoint)
                {
                    if (entry.second.lon > lon_max)
                        lon_max = entry.second.lon;
                    if (entry.second.lon < lon_min)
                        lon_min = entry.second.lon;
                    if (entry.second.lat > lat_max)
                        lat_max = entry.second.lat;
                    if (entry.second.lat < lat_min)
                        lat_min = entry.second.lat;
                }
            }
        }
        //归一化函数
        void Normalize(Datapoint& datapoint)
        {

            int count = datapoint.size();
            if(count == 0)
                return;

            // 计算均值
            double sum_lon = 0.0, sum_lat = 0.0;
            for (auto& entry : datapoint)
            {
                sum_lon += entry.second.lon;
                sum_lat += entry.second.lat;
            }
            mean_lon = sum_lon / count;
            mean_lat = sum_lat / count;

            // 计算标准差
            double sq_sum_lon = 0.0, sq_sum_lat = 0.0;
            for (auto& entry : datapoint)
            {
                double dlon = entry.second.lon - mean_lon;
                double dlat = entry.second.lat - mean_lat;
                sq_sum_lon += dlon * dlon;
                sq_sum_lat += dlat * dlat;
            }
            std_lon = std::sqrt(sq_sum_lon / count);
            std_lat = std::sqrt(sq_sum_lat / count);

            // 为避免标准差过小导致数值不稳定，可以设置一个下限（例如1e-6）
            if (std::abs(std_lon) < 1e-6) std_lon = 1e-6;
            if (std::abs(std_lat) < 1e-6) std_lat = 1e-6;

            // 归一化每个数据点
            for (auto& entry : datapoint)
            {
                entry.second.lon = (entry.second.lon - mean_lon) / std_lon;
                entry.second.lat = (entry.second.lat - mean_lat) / std_lat;
            }
        }
    //反归一化函数
    void DeNormalize(Datapoint& dataresult)
    {
        for (auto& entry : dataresult)
        {
            entry.second.lon = entry.second.lon * std_lon + mean_lon;
            entry.second.lat = entry.second.lat * std_lat + mean_lat;
        }
    }

	protected:
		bool valid;

	};
	///The class of Taylor polynomial;
	///When using this class, it needs to be initialized with the init function,
	///and the input and output data structure is the Datapoint class
    ///(see the Datastruct.h file for a specific definition).
    class TaylorModel {
    private:
        // Minimum and maximum values for normalization
        double xMin, xMax, yMin, yMax, tMin, tMax;

        // Center point for Taylor expansion
        double centerX, centerY;

        // Taylor coefficients array
        // coefficients[i][j] represents the coefficient for (x^i * y^j)
        std::vector<std::vector<double>> coefficients;

        // Calculate factorial
        double factorial(int n);

        // Calculate binomial coefficient C(n,k)
        double binomialCoeff(int n, int k);

        // Normalize input coordinate to [0, 1] range
        double normalizeCoordinate(double value, double minVal, double maxVal);

        // Map [0, 1] back to original coordinate range
        double denormalizeCoordinate(double value, double minVal, double maxVal);

        // Calculate Taylor coefficient for term (x^p * y^q)
        double calculateCoefficient(const Datapoint& datapoints, int p, int q);

    public:
        TaylorModel();
        ~TaylorModel();

        // Set min and max values from data points
        void setMinMax(const Datapoint& datapoints);

        // Normalize data points to [0, 1] range
        void Normalize(Datapoint& datapoints);

        // Denormalize data points back to original range
        void Denormalize(Datapoint& datapoints);

        // Set center point for Taylor expansion
        void setCenter(double x, double y);

        // Compute Taylor coefficients from training data up to order 'cutoff'
        void computeCoefficients(const Datapoint& trainingData, int cutoff);

        // Interpolate magnetic field value at given coordinates
        double interpolate(double x, double y);

        // Apply interpolation to result dataset
        void applyInterpolation(const Datapoint& trainingData, Datapoint& resultData, int cutoff);
    };


    class LegendreModel {
    private:
        double xMin, xMax, yMin, yMax, tMin, tMax;
        std::vector<std::vector<double>> coefficients;
        int N; // Order of Legendre polynomial

        // Calculate Legendre polynomial of order n at point x
        double LegendrePolynomial(int n, double x) {
            if (n == 0) return 1.0;
            if (n == 1) return x;

            double p0 = 1.0;
            double p1 = x;
            double pn = 0.0;

            for (int i = 2; i <= n; i++) {
                pn = ((2.0 * i - 1.0) * x * p1 - (i - 1.0) * p0) / i;
                p0 = p1;
                p1 = pn;
            }

            return pn;
        }

        // Map input coordinate to [-1, 1] range for Legendre polynomials
        double normalizeCoordinate(double value, double minVal, double maxVal) {
            if (std::abs(maxVal - minVal) < 1e-10)
                return 0.0;
            return 2.0 * (value - minVal) / (maxVal - minVal) - 1.0;
        }

        // Map [-1, 1] back to original coordinate range
        double denormalizeCoordinate(double value, double minVal, double maxVal) {
            return minVal + (value + 1.0) * (maxVal - minVal) / 2.0;
        }

    public:
        LegendreModel() : xMin(0), xMax(0), yMin(0), yMax(0), tMin(0), tMax(0), N(0) {}

        void setMinMax(const Datapoint& datapoints) {
            if (datapoints.empty()) return;

            auto it = datapoints.begin();
            xMin = xMax = it->second.X;
            yMin = yMax = it->second.Y;
            tMin = tMax = it->second.tMagnetic;

            for (const auto& point : datapoints) {
                xMin = std::min(xMin, point.second.X);
                xMax = std::max(xMax, point.second.X);
                yMin = std::min(yMin, point.second.Y);
                yMax = std::max(yMax, point.second.Y);
                tMin = std::min(tMin, point.second.tMagnetic);
                tMax = std::max(tMax, point.second.tMagnetic);
            }
        }

        void Normalize(Datapoint& datapoints) {
            for (auto& point : datapoints) {
                point.second.X = normalizeCoordinate(point.second.X, xMin, xMax);
                point.second.Y = normalizeCoordinate(point.second.Y, yMin, yMax);
            }
        }

        void Denormalize(Datapoint& datapoints) {
            for (auto& point : datapoints) {
                point.second.X = denormalizeCoordinate(point.second.X, xMin, xMax);
                point.second.Y = denormalizeCoordinate(point.second.Y, yMin, yMax);
            }
        }

        // Compute Legendre coefficients from training data
        void computeCoefficients(const Datapoint& trainingData, int order) {
            N = order;
            coefficients.clear();
            coefficients.resize((N + 1) * (N + 1));

            // Initialize coefficients to zero
            for (int n = 0; n <= N; n++) {
                for (int m = 0; m <= N; m++) {
                    coefficients[n * (N + 1) + m] = std::vector<double>(1, 0.0);
                }
            }

            // For each data point
            for (const auto& point : trainingData) {
                double x = point.second.X;
                double y = point.second.Y;
                double t = point.second.tMagnetic;

                // Calculate coefficient contribution
                for (int n = 0; n <= N; n++) {
                    double Pn_x = LegendrePolynomial(n, x);

                    for (int m = 0; m <= N; m++) {
                        double Pm_y = LegendrePolynomial(m, y);
                        coefficients[n * (N + 1) + m][0] += t * Pn_x * Pm_y;
                    }
                }
            }

            // Normalize coefficients
            int numPoints = trainingData.size();
            for (int n = 0; n <= N; n++) {
                for (int m = 0; m <= N; m++) {
                    coefficients[n * (N + 1) + m][0] /= numPoints;

                    // Apply orthogonality factor
                    coefficients[n * (N + 1) + m][0] *= (2 * n + 1) * (2 * m + 1) / 4.0;
                }
            }
        }

        // Interpolate magnetic field value at given coordinates
        double interpolate(double x, double y) {
            // Normalize inputs to [-1, 1]
            double x_norm = normalizeCoordinate(x, xMin, xMax);
            double y_norm = normalizeCoordinate(y, yMin, yMax);

            double result = 0.0;

            // Compute interpolation using Legendre polynomials
            for (int n = 0; n <= N; n++) {
                double Pn_x = LegendrePolynomial(n, x_norm);

                for (int m = 0; m <= N; m++) {
                    double Pm_y = LegendrePolynomial(m, y_norm);
                    result += coefficients[n * (N + 1) + m][0] * Pn_x * Pm_y;
                }
            }

            return result;
        }

        // Apply interpolation to result dataset
        void applyInterpolation(const Datapoint& trainingData, Datapoint& resultData, int order) {
            // Set min/max values from training data
            setMinMax(trainingData);

            // Compute coefficients using training data
            computeCoefficients(trainingData, order);

            // Apply interpolation to all points in result dataset
            for (auto& point : resultData) {
                point.second.tMagnetic = interpolate(point.second.X, point.second.Y);
            }
        }
    };

	class Polyhedral : public GeomangeticModel
	{
    private:
        // Number of data points and parameters
        size_t m; // Number of data points
        size_t n; // Number of parameters (same as m for full RBF)
        double Xi, Yi; // Center coordinates
        double Sigma2; // Smoothing factor

        // Use sparse matrices for efficiency
        Eigen::MatrixXd A;
        Eigen::RowVectorXd Ap;
        Eigen::VectorXd F; // Magnetic values
        Eigen::VectorXd X; // Parameters

        // KD-tree for fast nearest neighbor search
        typedef void* KDTreePtr; // 使用不透明指针，避免头文件依赖
        std::unique_ptr<PointCloudAdapter> point_cloud;
        KDTreePtr kdtree;

        // Parameters for optimization
        const size_t MAX_NEIGHBORS = 100; // Maximum number of neighbors to consider
        const size_t BLOCK_SIZE = 10000;   // 每块处理1万个点
        const size_t NUM_THREADS = 16;    // Number of threads for parallel processing
        const double EPSILON = 1e-6;      // Small value to prevent division by zero

        // Helper methods for parallel processing
        void processChunk(const Datapoint& datapoint, Datapoint& dataresult,
                          const Datainfo& datainfo, size_t start, size_t end,
                          const std::vector<int>& indices);

    public:
        Polyhedral();
        ~Polyhedral();

        void init(Datainfo& datainfo, Datapoint& datapoint);
        void ComputeQ(Datainfo& datainfo, Datapoint& datapoint);
        void ComputeX(Datainfo& datainfo, Datapoint& datapoint);
        void Result(Datainfo& datainfo, Datapoint& dataresult, Datapoint& datapoint);
	};

    class Splinecurve : public GeomangeticModel
    {
    public:
        void init(Datainfo& datainfo, Datapoint& datapoint);
        void ComputeX(Datapoint& datapoint);
        void Result(Datapoint& dataresult, Datapoint& datapoint);
        virtual ~Splinecurve() {};
    public:
        int C;
        double E;
        int N;
        MatrixXd X;
        MatrixXd Aplus;
        MatrixXd B;
    };


	class MomentHarmonic : public GeomangeticModel
	{
	public:
		MomentHarmonic();
		void init();
		void CoordTransform(Datapoint& datapoint);
		void ComputeCoef();
		void Compute();
		void GridConstruction(Datapoint& dataresult);
	};


    class OptimizedCubicInterpolator
    {
    public:
        // 执行三次样条插值
        static std::vector<double> interpolate(
            const std::vector<double>& x,   // 输入点x坐标
            const std::vector<double>& y,   // 输入点y坐标
            const std::vector<double>& z,   // 输入点对应的值
            const std::vector<double>& xi,  // 查询点x坐标
            const std::vector<double>& yi   // 查询点y坐标
            );
        static std::vector<double> cubic_interpolate(
            const std::vector<double>& x,   // 输入点x坐标
            const std::vector<double>& y,   // 输入点y坐标
            const std::vector<double>& z,   // 输入点对应的值
            const std::vector<double>& xi,  // 查询点x坐标
            const std::vector<double>& yi   // 查询点y坐标
            );
        static std::vector<double> idw_interpolate(
            const std::vector<double>& x,   // 输入点x坐标
            const std::vector<double>& y,   // 输入点y坐标
            const std::vector<double>& z,   // 输入点对应的值
            const std::vector<double>& xi,  // 查询点x坐标
            const std::vector<double>& yi   // 查询点y坐标
            );
        std::vector<double> interpolateGridWithEigen(
            const std::vector<double>& x,
            const std::vector<double>& y,
            const std::vector<double>& z,
            const std::vector<double>& xx,
            const std::vector<double>& yy);
    private:
        // 计算径向基函数值
        static double rbf(double r);
        static double cubicInterpolate(double p0, double p1, double p2, double p3, double t);

        // 块处理插值
        static std::vector<double> blockInterpolate(
            const std::vector<double>& x,
            const std::vector<double>& y,
            const std::vector<double>& z,
            const std::vector<double>& xi,
            const std::vector<double>& yi,
            int maxPointsPerBlock
            );

        // 局部RBF插值
        static double interpolatePoint(
            const std::vector<double>& x,
            const std::vector<double>& y,
            const std::vector<double>& z,
            double xi,
            double yi,
            int maxNeighbors
            );

        // 寻找最近的N个点
        static std::vector<size_t> findNearestPoints(
            const std::vector<double>& x,
            const std::vector<double>& y,
            double xi,
            double yi,
            int n
            );
    };

    // 二维Cubic插值类
    class CubicInterpolator2D {
    public:
        // 构造函数：输入原始散点x, y, z和规则网格的分辨率
        CubicInterpolator2D(const std::vector<double>& x,
                            const std::vector<double>& y,
                            const std::vector<double>& z,
                            int grid_size = 100);

        // 插值查询点(xx, yy)，返回插值结果zz
        std::vector<double> interpolate(const std::vector<double>& xx,
                                        const std::vector<double>& yy);

        void idw_fill_grid(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z,
                           const std::vector<double>& grid_x, const std::vector<double>& grid_y,
                           std::vector<double>& grid_z, int grid_size, double p = 2.0);

    private:
        // 网格节点
        std::vector<double> grid_x_, grid_y_;
        std::vector<double> grid_z_;
        int grid_size_;

        // 核心插值对象
        alglib::spline2dinterpolant spline_;
    };
}


#endif
