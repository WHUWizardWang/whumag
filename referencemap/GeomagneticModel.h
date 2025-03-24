/**
 * @file GeomangeticModel.hpp
 * Base class for Geomangetic models
 */
#ifndef _GEOMANGETICMODEL_H_
#define _GEOMANGETICMODEL_H_
#include<cmath>
#include<vector>
#include <limits>
#include <chrono>
#include <algorithm>
#include <queue>
#include <utility>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include<QDebug>
#include<eigen-3.4.0/Eigen/Dense>
#include <eigen-3.4.0/Eigen/Sparse>
#include"DataStruct.h"
namespace Geomagnetic {
	using namespace Eigen;
	class GeomangeticModel
	{
	public:
		//Distructor
		virtual ~GeomangeticModel(){}

		//Return validity of model
		bool isValid(void)
		{
			return valid;
		}

	protected:
		bool valid;

	};
	///The class of Taylor polynomial;
	///When using this class, it needs to be initialized with the init function,
	///and the input and output data structure is the Datapoint class
	///(see the Datastruct.h file for a specific definition).	  
	class TaylorModel : public GeomangeticModel
	{
	public:
		void init(Datainfo& datainfo);
		TaylorModel(void);
		void CalculateM(Datapoint& datapoint);
		void CalculateAQ(Datapoint& datapoint);
		void Result(Datapoint& dataresult);
		virtual ~TaylorModel() {};

	protected:
		int N = 15;
		double fai0 = 0;
		double lambda0 = 0;
		int P = 0;
		MatrixXd A;
		MatrixXd Q;
		MatrixXd F;
		MatrixXd M;
	};

	class LegendreModel : public GeomangeticModel
	{
	public:
		void init(Datainfo& datainfo,Datapoint& datapoint);
		LegendreModel();
		void NormalizedCalculation(Datapoint& datapoint,Datapoint& dataresult);
		void ComputeLegendreMatrix(Datainfo& datainfo, Datapoint& datapoint);
		void Result(Datainfo& datainfo, Datapoint& dataresult);
		virtual ~LegendreModel() {};
	protected:
		int N = 6;
		int P = 0;
		double maxLat;
		double minLat;
		double maxLon;
		double minLon;
		MatrixXd delta;
		MatrixXd delta_result;
		MatrixXd B;
		MatrixXd X;
		VectorXd F;
	};

	class Polyhedral : public GeomangeticModel
	{
	public:
		void init(Datainfo& datainfo, Datapoint& datapoint);
		Polyhedral();
		void ComputeQ(Datainfo& datainfo ,Datapoint& datapoint);
		void ComputeX(Datainfo& datainfo, Datapoint& datapoint);
		void Result(Datainfo& datainfo, Datapoint& dataresult, Datapoint& datapoint);
		virtual ~Polyhedral() {};
	protected:
		double m;
		double n;
		double Xi;
		double Yi;
		MatrixXd A;
		RowVectorXd Ap;
		VectorXd F;
		VectorXd X;
		double Sigma2;

	};

	class Splinecurve : public GeomangeticModel
	{
	public:
		void init(Datainfo& datainfo, Datapoint& datapoint);
		void ComputeX(Datapoint& datapoint);
		void Result(Datapoint& dataresult, Datapoint& datapoint);
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

    private:
        // 计算径向基函数值
        static double rbf(double r);

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
	
}


#endif
