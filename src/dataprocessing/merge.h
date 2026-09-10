#pragma once

#include <iostream>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <eigen-3.4.0/Eigen/Dense>
#include <algorithm>
#include <math.h>
#include <numeric>
#include <QStringList>
#include <QVector>
#include <QDebug>

using namespace Eigen;

struct Point
{
	double L;
	double B;
	double T;
	double m;
};

//const double M_PI = 3.1415926535;

double d2r(double deg);
void TongJi(std::vector<double>dt, std::vector<double>down);

class rongHe
{
public:
	rongHe();
	~rongHe();

	double calculate_p1(Point a, Point b, double delt_phi0, double delt_lamda0);
	double calculate_p2(double m);
    void readfile();
    void allPoints(std::vector<std::vector<Point>>a, std::vector<Point>&all_points);
    std::vector<Point>calModel(double delt_phi0, double delt_lamda0);
    void evaluatePrecision(std::vector<Point>a, std::vector<Point>b);
    void rongHe_run2(double delt_phi0, double delt_lamda0,
                             double min_B, double min_L, double max_B, double max_L,double Bint,double Lint);
    void rongHe_run3(double delt_phi0, double delt_lamda0,
                             double min_B, double min_L, double max_B, double max_L,double Bint,double Lint);
    void createMap(double min_B, double min_L, double max_B, double max_L, double Bint, double Lint); // ����ģ������
    void createMap2(double min_B, double min_L, double max_B, double max_L,double Bint,double Lint);
    void outResult(QString filepath);

    std::vector<std::string>doc;
    std::vector<double>doc_m;
    std::vector<Point>doc_P;
    std::vector<Point>point0;
    std::vector<std::vector<Point>>doc_points;
    std::vector<Point>data;  //���ڴ�Ų�ֵ��Ĵ��쳣��Ϣ
    std::vector<Point>all_point;

	double p1;
	double p2;

    std::vector<double>T1;
    std::vector<double>T2;

private:
    void buildGrid(double min_B, double min_L, double max_B, double max_L, double Bint, double Lint, double B1, double B2, double L1, double L2);

};

