#pragma once
#include <iostream>
#include <eigen-3.4.0/Eigen/Dense>
#include <fftw3.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <math.h>
#include <numeric>


class readFile
{
public:
	readFile();
	~readFile();
    void read_file(std::vector<double>& X, std::vector<double>& Y, std::vector<double>& T, std::string infile);//读入坐标信息和磁异常值以及格网行列信息
	void setValue_d(double& a, double b);//double变量赋值
	void setValue_i(int& a, int b);//int变量赋值
    void readfile_run(std::string infile);//完成延拓前所有准备工作
    void cal_grid(double x_step,double y_step);

    std::ifstream fin;
    std::ofstream fout;

    double Xi;
	double Yi;
	double Ti;

    std::vector<double>X;
    std::vector<double>Y;
    std::vector<double>T;
    std::string line;

	int gridrow;
	int gridcol;

private:

};

