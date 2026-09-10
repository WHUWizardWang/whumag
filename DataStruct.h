#pragma once
#include<cstring>
#include<eigen-3.4.0/Eigen/Dense>
#include<map>
#include<vector>
#include"eigenqdebug.h"

namespace Geomagnetic
{
constexpr auto DEBUG = 0;

	using namespace Eigen;

	//The structure of the single point
	class SinglePoint
	{
	public:
		void setMagnetic(double tm)
		{
			tMagnetic = tm;
		}
	public:

		double gpsWeek;
		double gpsSeconds;

		double xMagnetic=0;
		double yMagnetic=0;
		double zMagnetic=0;
		double tMagnetic=0;

		double lat;
		double lon;
		double height;

		double X;
		double Y;
		double Z=0;

        int index = 0;

	}; 
	//The structure of the read data
    typedef std::map<int, SinglePoint> Datapoint;
	//The structure of the data info
    struct Datainfo
	{
		double DataNum;//读入数据数量
		double Cx;//数据中心点X坐标
		double Cy;//数据中心点Y坐标
		double Clon;//数据中心点经度
		double Clat;//数据中心点纬度
		int Cutoff = 5;//泰勒多项式截止阶数
		int N = 6;//勒让德截止阶数
		short PolyQ = 0;//默认0为正双曲函数，1为反双曲函数
		double sigma2 = 10;//多面函数法的平滑因子
		double E = 0.01;//样条曲线的曲率
		int C = 1000;//样条曲线的弹性稀疏
	} ;
	//经度，纬度，总磁场强度
//	typedef map<double, map<double, double>> DataResult;

}
