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
	
	// 帮助函数来计算阶乘
	double factorial(int n) {
		double result = 1;
		for (int i = 1; i <= n; ++i) {
			result *= i;
		}
		return result;
	}

	// 使用公式计算 P_k(Δφ)
	double P_k(int k, double delta_phi) {
		double result = 0;
		// 使用 floor(k / 2) 来决定上限
		for (int m = 0; m <= k / 2; ++m) {
			double coeff = pow(-1, m) * factorial(2 * k - 2 * m) /
				(pow(2, k) * factorial(m) * factorial(k - m) * factorial(k - 2 * m));
			result += coeff * pow(delta_phi, k - 2 * m);
		}
		return result;
	}

	// 使用嵌套循环和公式计算 F_i
    double F_i(int N, double delta_phi, double delta_lambda, const std::vector<std::vector<double>>& a) {
		double result = 0;
		for (int n = 0; n <= N; ++n) {
			for (int k = 0; k <= n; ++k) {
				result += a[n][k] * P_k(k, delta_phi) * P_k(n - k, delta_lambda);
			}
		}
		return result;
	}
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
	
	//勒让德多项式计算（使用迭代法）
	double legendre(int n, double x) 
	{
		
		// 如果 n 为 0，返回 P_0(x) = 1
		if (n == 0)
			return 1.0;

		// 如果 n 为 1，返回 P_1(x) = x
		if (n == 1)
			return x;

		double Pn_minus2 = 1.0; // P_0(x)
		double Pn_minus1 = x;   // P_1(x)
		double Pn;

		// 使用迭代计算 P_n(x)
		for (int i = 2; i <= n; i++) {
			Pn = ((2.0 * i - 1.0) * x * Pn_minus1 - (i - 1) * Pn_minus2) / i;
			Pn_minus2 = Pn_minus1;
			Pn_minus1 = Pn;
		}

		return Pn_minus1;
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
	void TaylorModel::init(Datainfo& datainfo)
	{
		N = datainfo.Cutoff;
		fai0 = datainfo.Clon;
		lambda0 = datainfo.Clat;
		P = datainfo.DataNum;
	}

	TaylorModel::TaylorModel(void)
	{
	}
	inline static double delta_Compute(double a, double b,double i)
	{
		return pow(a - b,i);
	}
	void TaylorModel::CalculateM(Datapoint& datapoint)
	{
		M.resize(P, (N + 1) * (N + 2) / 2);
		int index1 = 0;
		for (const auto& elem:datapoint)
		{
			int index = 0;
			const SinglePoint& entry = elem.second;
			for (int i = 0; i <= N; i++)
			{
				for (int j = i; j >=0 ; j--)
				{
//					cout << delta_Compute(fai0, entry.lon, j) << " " << delta_Compute(lambda0, entry.lat, i - j);
					M(index1, index++) = delta_Compute(fai0, entry.lon,j) * delta_Compute(lambda0, entry.lat,i-j);
//					cout << M(index1, index-1);
				}
			}
			index1++;
		}
		if (DEBUG)
		{
            std::cout << M;
		}
	}

	void TaylorModel::CalculateAQ(Datapoint& datapoint)
	{
		A.resize((N + 1) * (N + 2) / 2, 1);
		F.resize(P, 1);
		int index = 0;
		for (const auto& elem : datapoint)
		{
			F(index++, 0) = elem.second.tMagnetic;
		}
		A = (M.transpose() * M).ldlt().solve(M.transpose() * F);
		if (DEBUG)
		{
            std::cout << A;
		}
	}

	void TaylorModel::Result(Datapoint& dataresult)
	{
		for (auto& elem:dataresult)
		{
				int index = 0;
				SinglePoint& entry = elem.second;
				for (int i = 0; i < N; i++)
				{
					for (int j = i; j >= 0; j--)
					{
						entry.tMagnetic += A(index++)*delta_Compute(fai0, entry.lon, j) * delta_Compute(lambda0, entry.lat, i - j);
					}
				}
		}
	}



	/// <summary>
	/// 勒让德多项式拟合的基础函数库
	/// @Copyright 王文钊
	/// 2023.10.12
	/// </summary>
	void LegendreModel::init(Datainfo& datainfo,Datapoint& datapoint)
	{
		// Initialize min and max values with opposite limits.
        maxLat = std::numeric_limits<double>::lowest();
        minLat = std::numeric_limits<double>::max();
        maxLon = std::numeric_limits<double>::lowest();
        minLon = std::numeric_limits<double>::max();
		N = datainfo.N;
		P = datainfo.DataNum;
		F.resize(P);
		int i = 0;
		for (const auto& entry : datapoint)
		{
			F(i) = entry.second.tMagnetic;
			++i;
		}
	}
	LegendreModel::LegendreModel()
	{
	}
	void LegendreModel::NormalizedCalculation(Datapoint& datapoint ,Datapoint& dataresult)
	{
		int size = datapoint.size();
		int size_result = dataresult.size();
		delta.resize(size, 2);
		delta_result.resize(size_result, 2);
		//遍历Datapoint中的所有SinglePoint对象
		for (const auto& elem : datapoint) {
			const SinglePoint& sp = elem.second;
			// 更新最大纬度和最小纬度
            if (sp.Y > maxLat) maxLat = sp.Y;
            if (sp.Y < minLat) minLat = sp.Y;
			// 更新最大经度和最小经度
            if (sp.X > maxLon) maxLon = sp.X;
            if (sp.X < minLon) minLon = sp.X;
		} 
		double LatAvg = 0.5 * (maxLat + minLat);
		double LonAvg = 0.5 * (maxLon + minLon);
		double LatDif = 0.5 * (maxLat - minLat);
		double LonDif = 0.5 * (maxLon - minLon);

		int i = 0;
		for (const auto& elem : datapoint) {
			const SinglePoint& sp = elem.second;
			delta(i, 0) = (sp.lat - LatAvg) / LatDif;
			delta(i, 1) = (sp.lon - LonAvg) / LonDif;
			++i;
		}
		int j = 0;
		for (const auto& elem : dataresult) {
			const SinglePoint& sp = elem.second;
			delta_result(j, 0) = (sp.lat - LatAvg) / LatDif;
			delta_result(j, 1) = (sp.lon - LonAvg) / LonDif;
			++j;
		}
		 
	}

	void LegendreModel::ComputeLegendreMatrix(Datainfo& datainfo, Datapoint& datapoint)
	{
		B.resize(P, (N + 1) * (N + 2) / 2);
		for (int i = 0; i < P; i++)
		{
			int index = 0;//阶数循环
			for (int n = 0; n <= N; ++n) {
				for (int k = 0; k <= n; ++k) {
					B(i, index++) = P_k(k, delta(i, 0)) * P_k(n - k, delta(i,1));
				}
			}
		}
        qDebug()<<B<<endl;
		X.resize((N + 1) * (N + 2) / 2, 1);
        qDebug()<<F<<endl;
		X =  B.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(F);;
        qDebug()<<X<<endl;
	}

	void LegendreModel::Result(Datainfo& datainfo, Datapoint& dataresult)
	{
		int index1 = 0;
		for (auto& elem : dataresult)
		{
			double Fi = 0.0;
			int index = 0;
			// 对于所有n (从0到N)
			for (int n = 0; n <= N; ++n) {
				// 对于所有k (从0到n)
				for (int k = 0; k <= n; ++k) {
					// 在矩阵X中检索a_{nk}
					double a_nk = X(index++);
					// 计算公式的每一项
					Fi += a_nk * P_k(k, delta_result(index1,0)) * P_k(n - k, delta_result(index1,1));
				}
			}
			elem.second.tMagnetic = Fi;
				++index1;
		}
	}
	 

	/// <summary>
	/// 多面函数插值的基础函数库
	/// @Copyright 王文钊
	/// 2023.12.7
	/// </summary>
	void Polyhedral::init(Datainfo& datainfo, Datapoint& datapoint)
	{
        datainfo.DataNum = datapoint.size();
		m = datainfo.DataNum;
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
	Polyhedral::Polyhedral()
	{
	}
	void Polyhedral::ComputeQ(Datainfo& datainfo, Datapoint& datapoint)
	{
        // A.resize(m, n);
        // if (datainfo.PolyQ == 0)
        // {
        // 	int i = 0;
        // 	for (const auto& elem : datapoint)
        // 	{
        // 		int j = 0;
        // 		for (const auto& entry : datapoint)
        // 		{
        // 			A(i, j) = sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) + (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
        // 			++j;
        // 		}
        // 		++i;
        // 	}
        // }
        // else if (datainfo.PolyQ == 1)
        // {
        // 	int i = 0;
        // 	for (const auto& elem : datapoint)
        // 	{
        // 		int j = 0;
        // 		for (const auto& entry : datapoint)
        // 		{
        // 			A(i, j) = 1/sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) + (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
        // 			++j;
        // 		}
        // 	}
        // }
  //       else std::cerr << "Unknown Kernel function" << endl;
        int dataSize = datapoint.size();
        // 使用稀疏矩阵
        typedef Eigen::SparseMatrix<double> SpMat;
        SpMat sparseA(dataSize, dataSize);

        // 创建三元组列表用于填充稀疏矩阵
        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(dataSize * dataSize);  // 预留空间

        if (datainfo.PolyQ == 0)
        {
            int i = 0;
            for (const auto& elem : datapoint)
            {
                int j = 0;
                for (const auto& entry : datapoint)
                {
                    double value = sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) +
                                        (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
                    tripletList.push_back(T(i, j, value));
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
                    double denom = sqrt((elem.second.X - entry.second.X) * (elem.second.X - entry.second.X) +
                                        (elem.second.Y - entry.second.Y) * (elem.second.Y - entry.second.Y) + Sigma2);
                    double value = 1.0 / (denom > 1e-10 ? denom : 1e-10); // 避免除零
                    tripletList.push_back(T(i, j, value));
                    ++j;
                }
                ++i;
            }
        }
        else std::cerr << "Unknown Kernel function" << std::endl;

        // 构建稀疏矩阵
        sparseA.setFromTriplets(tripletList.begin(), tripletList.end());

        // 将稀疏矩阵转为稠密矩阵（如果你的求解器需要）
        // 或者修改ComputeX以使用稀疏求解器
        A = Eigen::MatrixXd(sparseA);
	}

	void Polyhedral::ComputeX(Datainfo& datainfo, Datapoint& datapoint)
	{
        // X.resize(n);
        // X = A.ldlt().solve(F);
        // 检查矩阵尺寸
        if (A.rows() == 0 || A.cols() == 0) {
            std::cerr << "Matrix A has invalid dimensions" << std::endl;
            return;
        }

        // 确保F向量已正确初始化
        if (F.size() != A.rows()) {
            std::cerr << "Vector F size doesn't match matrix A rows" << std::endl;
            return;
        }

        try {
            X.resize(A.cols());
            X = A.ldlt().solve(F);
        }
        catch (const std::bad_alloc& e) {
            std::cerr << "Memory allocation failed in ComputeX: " << e.what() << std::endl;
            // 尝试备用解决方案
        }

	}

	void Polyhedral::Result(Datainfo& datainfo, Datapoint& dataresult,Datapoint& datapoint)
	{
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
//		cout << X << endl;
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
        return r * r * r; // φ(r) = r³
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

        // 计算数据范围以优化空间划分
        double x_min = *std::min_element(x.begin(), x.end());
        double x_max = *std::max_element(x.begin(), x.end());
        double y_min = *std::min_element(y.begin(), y.end());
        double y_max = *std::max_element(y.begin(), y.end());

        // 确定块数量
        int numBlocksX = std::ceil(std::sqrt(static_cast<double>(numPoints) / maxPointsPerBlock));
        int numBlocksY = numBlocksX;

        // 创建块索引
        std::vector<std::vector<size_t>> blockIndices(numBlocksX * numBlocksY);
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

            // 为当前块确定数据点范围
            double block_x_min = x_min + (blockIndex % numBlocksX) * (x_max - x_min) / numBlocksX - (x_max - x_min) / (2 * numBlocksX);
            double block_x_max = x_min + ((blockIndex % numBlocksX) + 1) * (x_max - x_min) / numBlocksX + (x_max - x_min) / (2 * numBlocksX);
            double block_y_min = y_min + (blockIndex / numBlocksX) * (y_max - y_min) / numBlocksY - (y_max - y_min) / (2 * numBlocksY);
            double block_y_max = y_min + ((blockIndex / numBlocksX) + 1) * (y_max - y_min) / numBlocksY + (y_max - y_min) / (2 * numBlocksY);

            // 找到与此块相关的数据点
            std::vector<double> block_x, block_y, block_z;
            std::vector<size_t> dataIndices;

            for (size_t i = 0; i < x.size(); ++i) {
                if (x[i] >= block_x_min && x[i] <= block_x_max &&
                    y[i] >= block_y_min && y[i] <= block_y_max) {
                    block_x.push_back(x[i]);
                    block_y.push_back(y[i]);
                    block_z.push_back(z[i]);
                    dataIndices.push_back(i);
                }
            }

            // 对该块中的每个查询点进行局部插值
            for (size_t queryIdx : queryIndices[blockIndex]) {
                // 使用这个块的数据点进行插值
                double interpolated = 0.0;

                try {
                    // 如果数据点太少，扩大搜索范围
                    if (block_x.size() < 20) {
                        interpolated = interpolatePoint(x, y, z, xi[queryIdx], yi[queryIdx], 50);
                    } else {
                        interpolated = interpolatePoint(block_x, block_y, block_z, xi[queryIdx], yi[queryIdx], 30);
                    }

                    {
                        std::lock_guard<std::mutex> guard(resultMutex);
                        results[queryIdx] = interpolated;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "插值出错: " << e.what() << std::endl;

                    // 回退到最近邻插值
                    double min_dist = std::numeric_limits<double>::max();
                    size_t nearest_idx = 0;

                    for (size_t j = 0; j < block_x.size(); ++j) {
                        double dist = std::pow(block_x[j] - xi[queryIdx], 2) +
                                      std::pow(block_y[j] - yi[queryIdx], 2);
                        if (dist < min_dist) {
                            min_dist = dist;
                            nearest_idx = j;
                        }
                    }

                    {
                        std::lock_guard<std::mutex> guard(resultMutex);
                        results[queryIdx] = block_z[nearest_idx];
                    }
                }
            }

            // 更新进度
            int completed = ++processedBlocks;
            if (completed % 5 == 0 || completed == totalBlocks) {
                std::cout << "已完成 " << completed << " / " << totalBlocks
                          << " 块 (" << (100.0 * completed / totalBlocks) << "%)" << std::endl;
            }
        }

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

            if (n < 100) {
                // 对于小数据集，直接使用标准RBF方法
                std::cout << "使用标准RBF方法(数据集小)" << std::endl;

                // 这里可以添加标准RBF方法的实现
                // 暂时调用块处理方法，设置块大小为数据集大小
                results = blockInterpolate(x, y, z, xi, yi, n);
            }
            else if (n < 1000) {
                // 中等数据集使用块处理，但较大块
                std::cout << "使用中等大小的块处理方法" << std::endl;
                results = blockInterpolate(x, y, z, xi, yi, 500);
            }
            else {
                // 大数据集使用小块处理
                std::cout << "使用小块处理方法(大数据集)" << std::endl;
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
}
