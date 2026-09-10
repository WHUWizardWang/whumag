/**
 * @file GeomangeticModel.cpp
 * Base class for Geomangetic models
 */
#include<cstring>
#include<eigen-3.4.0/Eigen/Dense>
#include<map>
#include<cmath>
#include<set>
#include<algorithm>
#include<iostream>
#include<limits>
#include"GeomagneticModel.h"
#include"DataStruct.h"
#include <random>


namespace Geomagnetic
{
	const double PI = 3.1415926535;
	using namespace Eigen;


	/// <summary>
	/// 会使用到的数学求解函数库
	/// @Copyright 王文钊
	/// 2023.12.8
	/// </summary>
	
	//广义逆矩阵的求解
	static MatrixXd computePseudoinverse(const MatrixXd& matrix)
	{
		BDCSVD<MatrixXd> svd(matrix, ComputeFullU |ComputeFullV);
		const auto& singularValues = svd.singularValues();
		MatrixXd singularValuesInv(matrix.cols(), matrix.rows());
		singularValuesInv.setZero();
		int size = singularValues.size();
		for (unsigned int i = 0; i < size; ++i) {
			if (singularValues(i) > 1e-6) { // tolerance
				singularValuesInv(i, i) = 1 / singularValues(i);
			}
		}

		return svd.matrixV() * singularValuesInv * svd.matrixU().adjoint();
	}
	
    double Computer(SinglePoint& p1, SinglePoint& p2)
    {
        double temp = (p1.X - p2.X) * (p1.X - p2.X) + (p1.Y - p2.Y) * (p1.Y - p2.Y);
        return(temp);

    }
	/// <summary>
	/// 泰勒多项式拟合的基础函数库
	/// @Copyright 王文钊
	/// 2023.10.10
	/// </summary>
    TaylorModel::TaylorModel()
        : xMin(0), xMax(0), yMin(0), yMax(0), tMin(0), tMax(0), centerX(0), centerY(0) {
    }

    TaylorModel::~TaylorModel() {
    }

    double TaylorModel::factorial(int n) {
        if (n <= 1) return 1.0;
        double result = 1.0;
        for (int i = 2; i <= n; ++i) {
            result *= i;
        }
        return result;
    }

    double TaylorModel::binomialCoeff(int n, int k) {
        if (k < 0 || k > n) return 0.0;
        if (k == 0 || k == n) return 1.0;

        double result = 1.0;
        for (int i = 1; i <= k; ++i) {
            result *= (n - (i - 1));
            result /= i;
        }
        return result;
    }

    double TaylorModel::normalizeCoordinate(double value, double minVal, double maxVal) {
        if (std::abs(maxVal - minVal) < 1e-10)
            return 0.5;
        return (value - minVal) / (maxVal - minVal);
    }

    double TaylorModel::denormalizeCoordinate(double value, double minVal, double maxVal) {
        return minVal + value * (maxVal - minVal);
    }

    void TaylorModel::setMinMax(const Datapoint& datapoints) {
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

        // Set default center to the middle of the domain
        centerX = (xMin + xMax) / 2.0;
        centerY = (yMin + yMax) / 2.0;
    }

    void TaylorModel::Normalize(Datapoint& datapoints) {
        for (auto& point : datapoints) {
            point.second.X = normalizeCoordinate(point.second.X, xMin, xMax);
            point.second.Y = normalizeCoordinate(point.second.Y, yMin, yMax);
        }
    }

    void TaylorModel::Denormalize(Datapoint& datapoints) {
        for (auto& point : datapoints) {
            point.second.X = denormalizeCoordinate(point.second.X, xMin, xMax);
            point.second.Y = denormalizeCoordinate(point.second.Y, yMin, yMax);
        }
    }

    void TaylorModel::setCenter(double x, double y) {
        centerX = normalizeCoordinate(x, xMin, xMax);
        centerY = normalizeCoordinate(y, yMin, yMax);
    }

    double TaylorModel::calculateCoefficient(const Datapoint& datapoints, int p, int q) {
        // Implementation of least squares method to find coefficient for x^p * y^q
        // This is simplified for demonstration - a more robust approach would use
        // linear algebra libraries for solving the system of equations

        double sumXpYq = 0.0;
        double sumT = 0.0;
        double sumXpYqT = 0.0;
        double sumXpYqXpYq = 0.0;

        for (const auto& point : datapoints) {
            double x = point.second.X - centerX;
            double y = point.second.Y - centerY;
            double t = point.second.tMagnetic;

            double xpyq = std::pow(x, p) * std::pow(y, q);

            sumXpYq += xpyq;
            sumT += t;
            sumXpYqT += xpyq * t;
            sumXpYqXpYq += xpyq * xpyq;
        }

        double n = datapoints.size();
        if (std::abs(sumXpYqXpYq) < 1e-10) {
            return 0.0;
        }

        // Scale by factorial to match Taylor series formula
        double coeff = sumXpYqT / sumXpYqXpYq;
        coeff /= (factorial(p) * factorial(q));

        return coeff;
    }

    void TaylorModel::computeCoefficients(const Datapoint& trainingData, int cutoff) {
        // Initialize coefficients to zero
        coefficients.clear();
        coefficients.resize(cutoff + 1, std::vector<double>(cutoff + 1, 0.0));

        // Compute coefficients for each term x^p * y^q where p+q <= cutoff
        for (int order = 0; order <= cutoff; ++order) {
            for (int p = 0; p <= order; ++p) {
                int q = order - p;
                coefficients[p][q] = calculateCoefficient(trainingData, p, q);
            }
        }
    }

    double TaylorModel::interpolate(double x, double y) {
        // Normalize inputs
        double x_norm = normalizeCoordinate(x, xMin, xMax);
        double y_norm = normalizeCoordinate(y, yMin, yMax);

        // Relative to center point
        double dx = x_norm - centerX;
        double dy = y_norm - centerY;

        double result = 0.0;

        // Compute Taylor series
        for (size_t p = 0; p < coefficients.size(); ++p) {
            for (size_t q = 0; q < coefficients[p].size(); ++q) {
                if (p + q < coefficients.size()) {
                    result += coefficients[p][q] * std::pow(dx, p) * std::pow(dy, q);
                }
            }
        }

        return result;
    }

    void TaylorModel::applyInterpolation(const Datapoint& trainingData, Datapoint& resultData, int cutoff) {
        // Set min/max values from training data
        setMinMax(trainingData);

        // Compute coefficients using training data
        computeCoefficients(trainingData, cutoff);

        // Apply interpolation to all points in result dataset
        for (auto& point : resultData) {
            point.second.tMagnetic = interpolate(point.second.X, point.second.Y);
        }
    }

	/// <summary>
	/// 多面函数插值的基础函数库
	/// @Copyright 王文钊
	/// 2023.12.7
	/// </summary>
    Polyhedral::Polyhedral() : m(0), n(0), Xi(0), Yi(0), Sigma2(10), kdtree(nullptr) {
    }

    Polyhedral::~Polyhedral() {
        // 清理KD树资源
        if (kdtree) {
            typedef nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<double, PointCloudAdapter>,
                PointCloudAdapter, 2, unsigned int> KDTree;

            KDTree* tree = static_cast<KDTree*>(kdtree);
            delete tree;
            kdtree = nullptr;
        }
    }

    void Polyhedral::init(Datainfo& datainfo, Datapoint& datapoint) {
        auto start = std::chrono::high_resolution_clock::now();

        m = datapoint.size();
        Xi = datainfo.Cx;
        Yi = datainfo.Cy;
        Sigma2 = datainfo.sigma2;
        n = m;
        int size = datapoint.size();
        F.resize(size);
        int i = 0;
        for (const auto& entry : datapoint)
        {
            F(i) = entry.second.tMagnetic;
            ++i;
        }
    }

    void Polyhedral::ComputeQ(Datainfo& datainfo, Datapoint& datapoint) {
        A.resize(m, n);
        if (datainfo.PolyQ == 0)
        {
            int i = 0;
            for (const auto& elem : datapoint)
            {
                int j = 0;
                for (const auto& entry : datapoint)
                {
                    A(i, j) = sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) + (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
                    ++j;
                }
                ++i;
            }
        }
        else if (datainfo.PolyQ == 1)
        {
            int i = 0;
            for (const auto& elem : datapoint)
            {
                int j = 0;
                for (const auto& entry : datapoint)
                {
                    A(i, j) = 1/sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) + (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
                    ++j;
                }
            }
        }
        else std::cerr << "Unknown Kernel function" << endl;
    }

    void Polyhedral::ComputeX(Datainfo& datainfo, Datapoint& datapoint) {

        X.resize(n);
        X = A.ldlt().solve(F);

    }

    void Polyhedral::processChunk(const Datapoint& datapoint, Datapoint& dataresult,
                                  const Datainfo& datainfo, // 添加 datainfo 参数
                                  size_t start, size_t end, const std::vector<int>& indices) {
        // 获取点向量以加快访问速度（避免在循环中查找映射）
        std::vector<std::pair<double, double>> points;
        points.reserve(datapoint.size());
        for (const auto& p : datapoint) {
            points.emplace_back(p.second.X, p.second.Y);
        }

        typedef nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloudAdapter>,
            PointCloudAdapter, 2, unsigned int> KDTree;

        // 处理指定块中的每个结果点
        for (size_t idx = start; idx < end; ++idx) {
            int i = indices[idx];
            auto it = dataresult.find(i);
            if (it == dataresult.end()) continue;

            // 获取当前点坐标
            double x = it->second.X;
            double y = it->second.Y;
            double query_pt[2] = {x, y};

            std::vector<unsigned int> nn_indices(MAX_NEIGHBORS); // 使用unsigned int类型
            std::vector<double> nn_distances(MAX_NEIGHBORS);

            // 查找最近邻
            size_t num_results = static_cast<KDTree*>(kdtree)->knnSearch(
                query_pt, MAX_NEIGHBORS, nn_indices.data(), nn_distances.data());

            // 计算核函数值
            double tMagnetic = 0.0;
            for (size_t j = 0; j < num_results; ++j) {
                double dist = std::sqrt(nn_distances[j] + Sigma2);

                double kernel_val;
                if (datainfo.PolyQ == 0) {
                    kernel_val = dist;
                } else { // datainfo.PolyQ == 1
                    kernel_val = 1.0 / (dist + EPSILON);
                }

                tMagnetic += kernel_val * X(nn_indices[j]);
            }

            // 更新结果
            it->second.tMagnetic = tMagnetic;
        }
    }

    // Result 函数
    void Polyhedral::Result(Datainfo& datainfo, Datapoint& dataresult, Datapoint& datapoint) {
        Ap.resize(n);
        if (datainfo.PolyQ == 0)
        {
            for (auto& elem : dataresult)
            {
                int j = 0;
                for (const auto& entry : datapoint)
                {
                    Ap(0, j) = sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) + (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
                    ++j;
                }
                elem.second.tMagnetic = Ap * X;
            }
        }
        else if (datainfo.PolyQ == 1)
        {
            for (auto& elem : dataresult)
            {
                int j = 0;
                for (const auto& entry : datapoint)
                {
                    Ap(0, j) = 1 / sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) + (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
                    ++j;
                }
                elem.second.tMagnetic = Ap * X;
            }
        }
    }


    void Splinecurve::init(Datainfo& datainfo, Datapoint& datapoint)
    {
        C = datainfo.C;
        E = datainfo.E;
        N = datainfo.DataNum;
        X.resize(N + 3, 1);
        B = MatrixXd::Zero(N + 3, 1);
        int i = 0;
        for (const auto& elem : datapoint)
        {
            B(i, 0) = elem.second.tMagnetic;
            i++;
        }
    }

    void Splinecurve::ComputeX(Datapoint& datapoint)
    {
        MatrixXd A(N,N);
        MatrixXd mat1(N, N + 3);
        MatrixXd mat2(3, N + 3);
        MatrixXd mat3(N+3, N+3);
        MatrixXd mat(N, 3);
        MatrixXd Z = MatrixXd::Zero(3, 3);
        int i = 0;
        for (auto elem = datapoint.begin(); elem != datapoint.end();elem++)
        {
            int j = 0;
            for (auto entry = datapoint.begin(); entry != datapoint.end();entry++) {
                if (i == j)
                {
                    A(i, j) = C / 2;
                    j++;
                    continue;
                }
                //                cout << Computer(elem->second, entry->second) << endl;
                A(i, j) = Computer(elem->second, entry->second)*log(Computer(elem->second, entry->second)+E);
                //                cout << A(i, j) << endl;
                ++j;
            }
            mat(i, 0) = 1;
            mat(i, 1) = elem->second.X;
            mat(i, 2) = elem->second.Y;
            ++i;
            //            cout << i << " " << j << endl;
        }
        //        cout << mat << endl;
        //        cout << A << endl;
        mat1 << A, mat;
        mat2 << mat.transpose(), Z;
        mat3 << mat1
            , mat2;
        //		cout << mat3 << endl;
        Aplus = computePseudoinverse(mat3);
        X.resize(N + 3,1);
        X = Aplus * B;
    }

    void Splinecurve::Result(Datapoint& dataresult, Datapoint& datapoint)
    {
        int j = 0;
        for (auto& entry : dataresult)
        {
            double index=0;
            int i = 0;
            for (auto& elem :datapoint)
            {
                index += X(i) * Computer(entry.second, elem.second) * log(Computer(entry.second, elem.second)+E);
                i++;
            }
            entry.second.tMagnetic = X(N) + X(N + 1) * entry.second.X + X(N + 2) * entry.second.Y + index ;
            j++;
        }
    }

    // 径向基函数 - 三次样条
    double OptimizedCubicInterpolator::rbf(double r) {
        if (r <= 0.0) {
            return 0.0;
        }
    return r * r * std::log(std::max(r, 1e-10)); // 薄板样条
    }

    // 找到距离查询点最近的n个点
    std::vector<size_t> OptimizedCubicInterpolator::findNearestPoints(
        const std::vector<double>& x,
        const std::vector<double>& y,
        double xi,
        double yi,
        int n
        ) {
        using DistanceIndex = std::pair<double, size_t>;

        // 使用优先队列查找最近的n个点
        std::priority_queue<DistanceIndex, std::vector<DistanceIndex>, std::less<DistanceIndex>> pq;

        for (size_t i = 0; i < x.size(); ++i) {
            double dx = x[i] - xi;
            double dy = y[i] - yi;
            double dist_squared = dx*dx + dy*dy;

            if (pq.size() < static_cast<size_t>(n)) {
                pq.push({dist_squared, i});
            } else if (dist_squared < pq.top().first) {
                pq.pop();
                pq.push({dist_squared, i});
            }
        }

        std::vector<size_t> indices;
        indices.reserve(pq.size());

        while (!pq.empty()) {
            indices.push_back(pq.top().second);
            pq.pop();
        }

        return indices;
    }

    // 对单个点进行局部RBF插值
    double OptimizedCubicInterpolator::interpolatePoint(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z,
        double xi,
        double yi,
        int maxNeighbors
        ) {
        // 最少需要16个点进行有效的三次插值
        int n = std::min(maxNeighbors, static_cast<int>(x.size()));
        n = std::max(16, n);

        // 找到最近的n个点
        std::vector<size_t> indices = findNearestPoints(x, y, xi, yi, n);

        // 提取这些点的坐标和值
        std::vector<double> local_x, local_y, local_z;
        local_x.reserve(indices.size());
        local_y.reserve(indices.size());
        local_z.reserve(indices.size());

        for (size_t idx : indices) {
            local_x.push_back(x[idx]);
            local_y.push_back(y[idx]);
            local_z.push_back(z[idx]);
        }

        // 计算权重矩阵A和右侧向量b
        int local_n = local_x.size();
        std::vector<std::vector<double>> A(local_n + 4, std::vector<double>(local_n + 4, 0.0));
        std::vector<double> b(local_n + 4, 0.0);

        // 填充RBF部分
        for (int i = 0; i < local_n; ++i) {
            for (int j = 0; j < local_n; ++j) {
                double dx = local_x[i] - local_x[j];
                double dy = local_y[i] - local_y[j];
                double r = std::sqrt(dx*dx + dy*dy);
                A[i][j] = rbf(r);
            }
            b[i] = local_z[i];
        }

        // 填充多项式部分
        for (int i = 0; i < local_n; ++i) {
            A[i][local_n] = 1.0;
            A[i][local_n+1] = local_x[i];
            A[i][local_n+2] = local_y[i];
            A[i][local_n+3] = local_x[i] * local_y[i];

            A[local_n][i] = 1.0;
            A[local_n+1][i] = local_x[i];
            A[local_n+2][i] = local_y[i];
            A[local_n+3][i] = local_x[i] * local_y[i];
        }
        std::vector<double> weights(local_n + 4, 0.0);
        // 求解线性系统 Ax = b 使用高斯消元法
        try {
            // 创建Eigen矩阵和向量
            Eigen::MatrixXd A_eigen(local_n + 4, local_n + 4);
            Eigen::VectorXd b_eigen(local_n + 4);

            // 填充矩阵和向量
            for (int i = 0; i < local_n + 4; ++i) {
                b_eigen(i) = b[i];
                for (int j = 0; j < local_n + 4; ++j) {
                    A_eigen(i, j) = A[i][j];
                }
            }

            // 使用SVD求解（最稳定的方法，适用于可能病态的矩阵）
            Eigen::VectorXd w_eigen = A_eigen.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b_eigen);

            // 将Eigen向量转换回std::vector
            for (int i = 0; i < local_n + 4; ++i) {
                weights[i] = w_eigen(i);
            }

            // 检查解的合理性
            double relative_error = (A_eigen * w_eigen - b_eigen).norm() / b_eigen.norm();
            if (relative_error > 0.1 || w_eigen.hasNaN()) {
                std::cerr << "警告: 解的相对误差大: " << relative_error << std::endl;
                throw std::runtime_error("解的精度不足");
            }
        }
        catch (const std::exception& e) {
            // 处理失败情况，添加正则化项
            std::cerr << "SVD求解失败，尝试Tikhonov正则化: " << e.what() << std::endl;

            Eigen::MatrixXd A_eigen(local_n + 4, local_n + 4);
            Eigen::VectorXd b_eigen(local_n + 4);

            for (int i = 0; i < local_n + 4; ++i) {
                b_eigen(i) = b[i];
                for (int j = 0; j < local_n + 4; ++j) {
                    A_eigen(i, j) = A[i][j];
                }
            }

            // 添加Tikhonov正则化
            double lambda = 1e-6;
            Eigen::MatrixXd AtA = A_eigen.transpose() * A_eigen;
            Eigen::MatrixXd I = Eigen::MatrixXd::Identity(local_n + 4, local_n + 4);
            Eigen::VectorXd Atb = A_eigen.transpose() * b_eigen;

            Eigen::VectorXd w_eigen = (AtA + lambda * I).ldlt().solve(Atb);

            for (int i = 0; i < local_n + 4; ++i) {
                weights[i] = w_eigen(i);
            }
        }

        // 使用求解出的权重计算插值结果
        double result = weights[local_n] + weights[local_n+1]*xi + weights[local_n+2]*yi + weights[local_n+3]*xi*yi;

        for (int j = 0; j < local_n; ++j) {
            double dx = xi - local_x[j];
            double dy = yi - local_y[j];
            double r = std::sqrt(dx*dx + dy*dy);
            result += weights[j] * rbf(r);
        }

        return result;
    }

    // 块处理插值
    std::vector<double> OptimizedCubicInterpolator::blockInterpolate(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z,
        const std::vector<double>& xi,
        const std::vector<double>& yi,
        int maxPointsPerBlock
        ) {
        size_t numPoints = xi.size();
        std::vector<double> results(numPoints);

        // 计算数据范围
        double x_min = *std::min_element(x.begin(), x.end());
        double x_max = *std::max_element(x.begin(), x.end());
        double y_min = *std::min_element(y.begin(), y.end());
        double y_max = *std::max_element(y.begin(), y.end());

        // 估计合理值范围（用于异常检测）
        std::vector<double> validValues;
        for (size_t i = 0; i < z.size(); ++i) {
            if (std::abs(z[i]) < 1000.0) { // 假设合理值小于1000
                validValues.push_back(z[i]);
            }
        }

        double median = 0.0;
        double mad = 0.0;  // 中位数绝对偏差
        if (!validValues.empty()) {
            size_t n = validValues.size() / 2;
            std::nth_element(validValues.begin(), validValues.begin() + n, validValues.end());
            median = validValues[n];

            std::vector<double> deviations;
            for (double val : validValues) {
                deviations.push_back(std::abs(val - median));
            }
            std::nth_element(deviations.begin(), deviations.begin() + n, deviations.end());
            mad = deviations[n] * 1.4826; // 转换为标准差估计
        }

        // 设置异常值阈值
        double lower_bound = median - 5 * mad;
        double upper_bound = median + 5 * mad;
        if (mad < 1e-10 || validValues.empty()) {
            lower_bound = -300.0;
            upper_bound = 300.0;
        }

        std::cout << "数据值范围估计: [" << lower_bound << ", " << upper_bound << "]" << std::endl;
        std::cout << "中位数: " << median << ", MAD: " << mad << std::endl;

        // 过滤数据，仅保留有效点
        std::vector<double> clean_x, clean_y, clean_z;
        for (size_t i = 0; i < z.size(); ++i) {
            if (z[i] >= lower_bound && z[i] <= upper_bound) {
                clean_x.push_back(x[i]);
                clean_y.push_back(y[i]);
                clean_z.push_back(z[i]);
            }
        }

        std::cout << "过滤后的有效数据点数量: " << clean_x.size() << " / " << x.size() << std::endl;

        if (clean_x.size() < 10) {
            std::cerr << "错误: 过滤后的有效数据点太少，无法进行插值" << std::endl;
            return std::vector<double>(xi.size(), median); // 返回中位数
        }

        // 构建KD树用于高效查找最近点
        std::vector<std::array<double, 2>> points(clean_x.size());
        for (size_t i = 0; i < clean_x.size(); ++i) {
            points[i] = {clean_x[i], clean_y[i]};
        }
        KDTree<double, 2> kdtree(points);

        // 确定块数量
        int numBlocksX = std::ceil(std::sqrt(static_cast<double>(numPoints) / maxPointsPerBlock));
        int numBlocksY = numBlocksX;

        // 创建块索引
        std::vector<std::vector<size_t>> queryIndices(numBlocksX * numBlocksY);

        // 将查询点分配到块
        for (size_t i = 0; i < xi.size(); ++i) {
            int blockX = std::min(numBlocksX - 1, static_cast<int>((xi[i] - x_min) / (x_max - x_min) * numBlocksX));
            int blockY = std::min(numBlocksY - 1, static_cast<int>((yi[i] - y_min) / (y_max - y_min) * numBlocksY));
            int blockIndex = blockY * numBlocksX + blockX;
            queryIndices[blockIndex].push_back(i);
        }

        // 为每个块处理查询点
        std::mutex resultMutex;
        std::atomic<int> processedBlocks(0);
        int totalBlocks = 0;

        for (int blockIndex = 0; blockIndex < numBlocksX * numBlocksY; ++blockIndex) {
            if (queryIndices[blockIndex].empty()) continue;
            totalBlocks++;

            // 找出当前块中所有查询点的范围
            double query_x_min = std::numeric_limits<double>::max();
            double query_x_max = std::numeric_limits<double>::lowest();
            double query_y_min = std::numeric_limits<double>::max();
            double query_y_max = std::numeric_limits<double>::lowest();

            for (size_t idx : queryIndices[blockIndex]) {
                query_x_min = std::min(query_x_min, xi[idx]);
                query_x_max = std::max(query_x_max, xi[idx]);
                query_y_min = std::min(query_y_min, yi[idx]);
                query_y_max = std::max(query_y_max, yi[idx]);
            }

            // 计算块的初始边界，扩大范围以确保覆盖
            double margin_x = (query_x_max - query_x_min) * 1.0; // 100% 边界扩展
            double margin_y = (query_y_max - query_y_min) * 1.0;

            // 确保最小边界大小
            margin_x = std::max(margin_x, (x_max - x_min) * 0.05);
            margin_y = std::max(margin_y, (y_max - y_min) * 0.05);

            double block_x_min = query_x_min - margin_x;
            double block_x_max = query_x_max + margin_x;
            double block_y_min = query_y_min - margin_y;
            double block_y_max = query_y_max + margin_y;

            // 找到与此块相关的数据点
            std::vector<double> block_x, block_y, block_z;
            std::vector<size_t> dataIndices;

            // 初始化数据点集合(使用已过滤的有效点)
            for (size_t i = 0; i < clean_x.size(); ++i) {
                if (clean_x[i] >= block_x_min && clean_x[i] <= block_x_max &&
                    clean_y[i] >= block_y_min && clean_y[i] <= block_y_max) {
                    block_x.push_back(clean_x[i]);
                    block_y.push_back(clean_y[i]);
                    block_z.push_back(clean_z[i]);
                    dataIndices.push_back(i);
                }
            }

            // 确保至少有足够的点
            const int minRequiredPoints = 30;
            if (block_x.size() < minRequiredPoints) {
                // 使用KD树为所有查询点找到足够的最近点
                std::set<size_t> uniqueIndices;

                for (size_t idx : queryIndices[blockIndex]) {
                    std::array<double, 2> query = {xi[idx], yi[idx]};
                    auto nearest = kdtree.nearest(query, 50);

                    for (const auto& [dist, point_idx] : nearest) {
                        if (uniqueIndices.find(point_idx) == uniqueIndices.end()) {
                            uniqueIndices.insert(point_idx);
                        }

                        if (uniqueIndices.size() >= 50) {
                            break;
                        }
                    }
                }

                block_x.clear();
                block_y.clear();
                block_z.clear();
                dataIndices.clear();

                for (size_t idx : uniqueIndices) {
                    block_x.push_back(clean_x[idx]);
                    block_y.push_back(clean_y[idx]);
                    block_z.push_back(clean_z[idx]);
                    dataIndices.push_back(idx);
                }
            }

            // 计算块数据点的凸包
            std::vector<size_t> hull_indices = ConvexHull::compute(block_x, block_y);

            // 为块内每个查询点进行插值
            for (size_t queryIdx : queryIndices[blockIndex]) {
                double interpolated = 0.0;
                bool useNearestNeighbor = false;

                // 检查查询点是否在凸包内
                bool isInHull = ConvexHull::pointInConvexHull(xi[queryIdx], yi[queryIdx], block_x, block_y, hull_indices);

                // 检查是否有足够的点以及是否在凸包内
                if (block_x.size() < 16 || !isInHull) {
                    useNearestNeighbor = true;
                } else {
                    try {
                        // 尝试RBF插值
                        interpolated = interpolatePoint(block_x, block_y, block_z, xi[queryIdx], yi[queryIdx], 30);

                        // 验证插值结果是否在合理范围内
                        if (interpolated < lower_bound || interpolated > upper_bound) {
                            std::cout << "检测到异常插值结果: " << interpolated
                                      << " 在位置 (" << xi[queryIdx] << ", " << yi[queryIdx]
                                      << ")，回退到最近邻插值" << std::endl;
                            useNearestNeighbor = true;
                        }
                    }
                    catch (const std::exception& e) {
                        std::cerr << "插值出错: " << e.what() << ", 位置: ("
                                  << xi[queryIdx] << ", " << yi[queryIdx] << ")" << std::endl;
                        useNearestNeighbor = true;
                    }
                }

                if (useNearestNeighbor) {
                    // 对于不在凸包内的点，找到凸包上最近的点
                    if (!isInHull && !hull_indices.empty()) {
                        auto [dist, nearest_idx] = ConvexHull::nearestPointOnConvexHull(
                            xi[queryIdx], yi[queryIdx], block_x, block_y, hull_indices);

                        // 通过最近的边界点获取z值
                        for (size_t j = 0; j < block_x.size(); ++j) {
                            if (j == nearest_idx) {
                                interpolated = block_z[j];
                                break;
                            }
                        }
                    } else {
                        // 如果在凸包内但RBF插值失败，或者凸包计算失败，回退到最近邻
                        std::array<double, 2> query = {xi[queryIdx], yi[queryIdx]};
                        auto nearest = kdtree.nearest(query, 1);

                        if (!nearest.empty()) {
                            size_t idx = nearest[0].second;
                            interpolated = clean_z[idx];
                        } else {
                            interpolated = median;
                        }
                    }
                }

                // 再次确认结果在合理范围内
                if (interpolated < lower_bound || interpolated > upper_bound) {
                    interpolated = std::max(lower_bound, std::min(upper_bound, interpolated));
                }

                {
                    std::lock_guard<std::mutex> guard(resultMutex);
                    results[queryIdx] = interpolated;
                }
            }

            // 更新进度
            int completed = ++processedBlocks;
            if (completed % 5 == 0 || completed == totalBlocks) {
                std::cout << "已完成 " << completed << " / " << totalBlocks
                          << " 块 (" << (100.0 * completed / totalBlocks) << "%)" << std::endl;
            }
        }

        // 最终检查所有结果
        int outlierCount = 0;
        for (size_t i = 0; i < results.size(); ++i) {
            if (results[i] < lower_bound || results[i] > upper_bound) {
                results[i] = std::max(lower_bound, std::min(upper_bound, results[i]));
                outlierCount++;
            }
        }

        if (outlierCount > 0) {
            std::cout << "最终处理了 " << outlierCount << " 个异常值 ("
                      << (100.0 * outlierCount / results.size()) << "%)" << std::endl;
        }

        std::cout << "插值完成，总共处理 " << results.size() << " 个查询点" << std::endl;
        return results;
    }

    // 主插值函数
    std::vector<double> OptimizedCubicInterpolator::interpolate(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z,
        const std::vector<double>& xi,
        const std::vector<double>& yi
        ) {
        // 检查输入有效性
        if (x.empty() || y.empty() || z.empty() || xi.empty() || yi.empty()) {
            std::cerr << "错误: 输入向量为空" << std::endl;
            return std::vector<double>(xi.size(), 0.0);
        }

        if (x.size() != y.size() || x.size() != z.size()) {
            std::cerr << "错误: 输入数据点向量大小不匹配" << std::endl;
            return std::vector<double>(xi.size(), 0.0);
        }

        if (xi.size() != yi.size()) {
            std::cerr << "错误: 查询点向量大小不匹配" << std::endl;
            return std::vector<double>(xi.size(), 0.0);
        }

        auto start = std::chrono::high_resolution_clock::now();

        // 根据数据规模决定最佳处理策略
        int n = x.size();
        int m = xi.size();

        std::cout << "开始插值: " << n << " 个数据点, " << m << " 个查询点" << std::endl;

        std::vector<double> results;

        try {
            // 估算内存需求并选择适当的处理方法
            size_t estMemoryMB = (n * n * sizeof(double)) / (1024 * 1024);
            std::cout << "估计矩阵内存需求: " << estMemoryMB << " MB" << std::endl;

            if (n < 10000) {
                // 对于小数据集，直接使用标准RBF方法
                std::cout << "使用标准RBF方法(数据集小)" << std::endl;

                // 这里可以添加标准RBF方法的实现
                // 暂时调用块处理方法，设置块大小为数据集大小
                results = blockInterpolate(x, y, z, xi, yi, n);
            }
            else if (n < 20000) {
                // 中等数据集使用块处理，但较大块
                std::cout << "使用中等大小的块处理方法" << std::endl;
                results = blockInterpolate(x, y, z, xi, yi, 5000);
            }
            else {
                // 大数据集使用小块处理
                std::cout << "使用小块处理方法(大数据集)" << std::endl;
                results = blockInterpolate(x, y, z, xi, yi, 2000);
            }
        }
        catch (const std::bad_alloc& e) {
            std::cerr << "内存分配失败: " << e.what() << std::endl;
            std::cerr << "尝试使用更小的块..." << std::endl;

            try {
                // 回退到更小的块大小
                results = blockInterpolate(x, y, z, xi, yi, 500);
            }
            catch (const std::exception& e) {
                std::cerr << "插值失败: " << e.what() << std::endl;

                // 最终回退到最近邻插值
                std::cout << "回退到最近邻插值方法" << std::endl;
                results.resize(xi.size());

#pragma omp parallel for
                for (size_t i = 0; i < xi.size(); ++i) {
                    double min_dist = std::numeric_limits<double>::max();
                    size_t nearest_idx = 0;

                    for (size_t j = 0; j < x.size(); ++j) {
                        double dist = std::pow(x[j] - xi[i], 2) + std::pow(y[j] - yi[i], 2);
                        if (dist < min_dist) {
                            min_dist = dist;
                            nearest_idx = j;
                        }
                    }

                    results[i] = z[nearest_idx];
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "插值完成，耗时: " << elapsed.count() << " 秒" << std::endl;

        return results;
    }

    std::vector<double> OptimizedCubicInterpolator::cubic_interpolate(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z,
        const std::vector<double>& xi,
        const std::vector<double>& yi
        )
    {
        // 检查输入有效性
        if (x.empty() || y.empty() || z.empty() || xi.empty() || yi.empty()) {
            std::cerr << "错误: 输入向量为空" << std::endl;
            return std::vector<double>(xi.size(), 0.0);
        }

        if (x.size() != y.size() || x.size() != z.size()) {
            std::cerr << "错误: 输入数据点向量大小不匹配" << std::endl;
            return std::vector<double>(xi.size(), 0.0);
        }

        if (xi.size() != yi.size()) {
            std::cerr << "错误: 查询点向量大小不匹配" << std::endl;
            return std::vector<double>(xi.size(), 0.0);
        }

        auto start = std::chrono::high_resolution_clock::now();

        // 根据数据规模决定最佳处理策略
        int n = x.size();
        int m = xi.size();

        //        std::cout << "开始插值: " << n << " 个数据点, " << m << " 个查询点" << std::endl;

        std::vector<double> results;

        try {
            // 估算内存需求并选择适当的处理方法
            size_t estMemoryMB = (n * n * sizeof(double)) / (1024 * 1024);
            //            std::cout << "估计矩阵内存需求: " << estMemoryMB << " MB" << std::endl;

            if (n < 100) {
                // 对于小数据集，直接使用标准RBF方法
                //                std::cout << "使用标准RBF方法(数据集小)" << std::endl;

                // 这里可以添加标准RBF方法的实现
                // 暂时调用块处理方法，设置块大小为数据集大小
                results = blockInterpolate(x, y, z, xi, yi, n);
            }
            else if (n < 1000) {
                // 中等数据集使用块处理，但较大块
                //                std::cout << "使用中等大小的块处理方法" << std::endl;
                results = blockInterpolate(x, y, z, xi, yi, 500);
            }
            else {
                // 大数据集使用小块处理
                //                std::cout << "使用小块处理方法(大数据集)" << std::endl;
                results = blockInterpolate(x, y, z, xi, yi, 200);
            }
        }
        catch (const std::bad_alloc& e) {
            std::cerr << "内存分配失败: " << e.what() << std::endl;
            std::cerr << "尝试使用更小的块..." << std::endl;

            try {
                // 回退到更小的块大小
                results = blockInterpolate(x, y, z, xi, yi, 50);
            }
            catch (const std::exception& e) {
                std::cerr << "插值失败: " << e.what() << std::endl;

                // 最终回退到最近邻插值
                //                std::cout << "回退到最近邻插值方法" << std::endl;
                results.resize(xi.size());

#pragma omp parallel for
                for (size_t i = 0; i < xi.size(); ++i) {
                    double min_dist = std::numeric_limits<double>::max();
                    size_t nearest_idx = 0;

                    for (size_t j = 0; j < x.size(); ++j) {
                        double dist = std::pow(x[j] - xi[i], 2) + std::pow(y[j] - yi[i], 2);
                        if (dist < min_dist) {
                            min_dist = dist;
                            nearest_idx = j;
                        }
                    }

                    results[i] = z[nearest_idx];
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        //        std::cout << "插值完成，耗时: " << elapsed.count() << " 秒" << std::endl;

        return results;
    }

    std::vector<double> OptimizedCubicInterpolator::idw_interpolate(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z,
        const std::vector<double>& xi,
        const std::vector<double>& yi
        )
    {
        const double p = 2.0;  // IDW幂次，一般取2
        const double eps = 1e-12; // 防止除0
        size_t n = x.size();
        size_t m = xi.size();

        std::vector<double> result(m, 0.0);

        for (size_t k = 0; k < m; ++k) {
            double sx = xi[k], sy = yi[k];
            double numerator = 0.0;
            double denominator = 0.0;
            bool found_exact = false;
            double exact_value = 0.0;

            for (size_t j = 0; j < n; ++j) {
                double dx = x[j] - sx;
                double dy = y[j] - sy;
                double dist2 = dx * dx + dy * dy;
                if (dist2 < eps) {
                    // 完全重合，直接赋值
                    found_exact = true;
                    exact_value = z[j];
                    break;
                }
                double w = 1.0 / std::pow(dist2, p * 0.5);
                numerator += w * z[j];
                denominator += w;
            }
            if (found_exact) {
                result[k] = exact_value;
            } else if (denominator > 0) {
                result[k] = numerator / denominator;
            } else {
                result[k] = 0.0; // 无法插值
            }
        }
        return result;
    }

    double OptimizedCubicInterpolator::cubicInterpolate(double p0, double p1, double p2, double p3, double t) {
        // 三次插值公式
        double a0 = -0.5*p0 + 1.5*p1 - 1.5*p2 + 0.5*p3;
        double a1 = p0 - 2.5*p1 + 2*p2 - 0.5*p3;
        double a2 = -0.5*p0 + 0.5*p2;
        double a3 = p1;
        return ((a0*t + a1)*t + a2)*t + a3;
    }

    void CubicInterpolator2D::idw_fill_grid(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z,
                           const std::vector<double>& grid_x, const std::vector<double>& grid_y,
                           std::vector<double>& grid_z, int grid_size, double p)
    {
        grid_z.resize(grid_size * grid_size, 0.0);
        for (int i = 0; i < grid_size; ++i) {
            for (int j = 0; j < grid_size; ++j) {
                double gx = grid_x[j];
                double gy = grid_y[i];
                double num = 0, denom = 0;
                bool found_exact = false;
                double exact_val = 0;
                for (size_t k = 0; k < x.size(); ++k) {
                    double dx = gx - x[k], dy = gy - y[k];
                    double dist2 = dx*dx + dy*dy;
                    if (dist2 < 1e-12) { // 点重合，直接赋值
                        found_exact = true;
                        exact_val = z[k];
                        break;
                    }
                    double w = 1.0 / std::pow(dist2, p/2.0);
                    num += w * z[k];
                    denom += w;
                }
                grid_z[i * grid_size + j] = found_exact ? exact_val : (denom > 0 ? num/denom : std::numeric_limits<double>::quiet_NaN());
            }
        }
    }

    CubicInterpolator2D::CubicInterpolator2D(const std::vector<double>& x,
                                             const std::vector<double>& y,
                                             const std::vector<double>& z,
                                             int grid_size)
        : grid_size_(grid_size)
    {
        if (x.size() != y.size() || x.size() != z.size())
            throw std::runtime_error("x,y,z must have same size");

        // 1. 生成规则格网坐标
        double x_min = *std::min_element(x.begin(), x.end());
        double x_max = *std::max_element(x.begin(), x.end());
        double y_min = *std::min_element(y.begin(), y.end());
        double y_max = *std::max_element(y.begin(), y.end());
        grid_x_.resize(grid_size);
        grid_y_.resize(grid_size);
        for (int i = 0; i < grid_size; ++i) {
            grid_x_[i] = x_min + (x_max - x_min) * i / (grid_size - 1);
            grid_y_[i] = y_min + (y_max - y_min) * i / (grid_size - 1);
        }

        // 2. 用IDW把散点填充到规则网格
        std::vector<double> grid_z;
        idw_fill_grid(x, y, z, grid_x_, grid_y_, grid_z, grid_size);

        // 3. 用alglib双三次样条在规则格网上拟合（grid_z 转为 real_2d_array）
        alglib::real_1d_array xg, yg;
        alglib::real_2d_array fg;
        xg.setcontent(grid_size, grid_x_.data());
        yg.setcontent(grid_size, grid_y_.data());
        fg.setlength(grid_size, grid_size);
        for (int i = 0; i < grid_size; ++i)
            for (int j = 0; j < grid_size; ++j)
                fg[i][j] = grid_z[i * grid_size + j];

        alglib::spline2dbuildbicubic(xg, yg, fg, grid_size, grid_size, spline_);


    }

    std::vector<double> CubicInterpolator2D::interpolate(const std::vector<double>& xx,
                                                         const std::vector<double>& yy)
    {
        if (xx.size() != yy.size())
            throw std::runtime_error("xx,yy size mismatch");
        std::vector<double> zz(xx.size());
        for (size_t i = 0; i < xx.size(); ++i) {
            zz[i] = alglib::spline2dcalc(spline_, xx[i], yy[i]);
        }
        return zz;
    }
    }
