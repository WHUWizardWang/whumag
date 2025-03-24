#pragma once

#include <eigen-3.4.0/Eigen/Dense>
//#include "fftw.h"
#include <vector>
#include "fftw3.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <math.h>
#include <numeric>
#include <utility>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
using namespace  Eigen;

class FFTW
{
public:
    FFTW();
    ~FFTW();
    void fftshift(fftw_complex* data, int rows, int cols);//频谱中心化函数
    fftw_complex* fft_2d(const MatrixXd& real_input, int& rows, int& cols);//二维傅里叶正向变换函数
    fftw_complex* ifft_2d(int& rows, int& cols, fftw_complex* in);//二维傅里叶逆变换函数

private:

};

class yanTuo
{
public:
	yanTuo();
	~yanTuo();
	
    void createGrid(std::vector<double>a, int rows, int cols, MatrixXd& A);//格网化函数
	MatrixXd addBorder(MatrixXd A);//扩边函数，满足傅里叶变换对于行数列数的要求
	fftw_complex* calculation_up(double xint, double yint, double h, int rows, int cols, fftw_complex* in);//计算对应的角频率u、v、延拓因子Q，处理得到向上延拓后的磁异常频谱
	fftw_complex* calculation_down(double xint, double yint, double h, int ln, int col, fftw_complex* in, int choice);//计算对应的角频率u、v、延拓因子Q，处理得到向下延拓后的磁异常频谱
    std::vector<double> get_result(fftw_complex* in, MatrixXd A, MatrixXd B);//删除扩边信息并提取延拓磁异常值
	double calculate_H(double R, int choice);// 用于向下延拓时计算延拓因子
    void TongJi(std::vector<double>dt, std::vector<double>down);//输出统计信息

    void up_run(int gridrow, int gridcol, std::vector<double>X, std::vector<double>Y, std::vector<double>T,
                double xint, double yint, double h, std::string outfile);//完成向上延拓处理
    void down_run(int gridrow, int gridcol, std::vector<double>X, std::vector<double>Y, std::vector<double>T,
                  double xint, double yint, double h, std::string outfile, int choice);//完成向下延拓处理
    void evaluatePrecision(int gridrow, int gridcol, std::vector<double>X, std::vector<double>Y, std::vector<double>T,
                           double xint, double yint, double h, int choice,std::string outfile);// 先向上延拓再向下延拓以达到评估精度的目的

	MatrixXd data;

	fftw_complex* fx;
	fftw_complex* ff;
	fftw_complex* upT;
	fftw_complex* downT;

    std::vector<double> up;// 提取向上延拓结果
    std::vector<double> down;// 提取向下延拓结果

	FFTW fftw;

	int row1;
	int col1;
	int row2;
	int col2;

	double H;// 向下延拓算子

	Eigen::MatrixXd  X1;// 存储格网数据X
	Eigen::MatrixXd  Y1;// 存储格网数据Y
	Eigen::MatrixXd  T1;// 存储格网数据T

    QString out;

private:
	

};


