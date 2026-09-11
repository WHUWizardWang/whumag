#pragma once
#include "eigen-3.4.0/Eigen/Dense"
#include "ReadData.h"
#include <fstream>
#include <iomanip>
#include <random>

namespace Geomagnetic
{
    // 粒子类
    class Particle
    {
    public:
        int nDim; // 参数维度（position和velocity的维度）, eg. C和sigma, ndim=2
        std::vector<double> vPosition; // 粒子的位置，即候选解
        std::vector<double> vVelocity; // 粒子运动速度
        std::vector<double> bHistoryBestPosition; // 粒子历史最优位置
        double dFitness; // 粒子适应度
        double dHistoryBestFitness; 
        Particle()
        {
            nDim = 2;
            vPosition = { 0,0 };
            vVelocity = { 0,0 };
            bHistoryBestPosition = { 0,0 };
            dFitness = 99999999;
            dHistoryBestFitness = 99999999;
        }
        Particle(int tmpnDim, std::vector<double> tmpvPosition, std::vector<double> tmpvVelocity, double tmpdFitness)
        {
            nDim = tmpnDim;
            vPosition.assign(tmpvPosition.begin(), tmpvPosition.end());
            bHistoryBestPosition.assign(tmpvPosition.begin(), tmpvPosition.end());
            vVelocity.assign(tmpvVelocity.begin(), tmpvVelocity.end());
            dFitness = tmpdFitness;
            dHistoryBestFitness = tmpdFitness;
        }
        void copy(Particle temp) // 复制构造
        {
            nDim = temp.nDim;
            std::vector <double>().swap(vPosition);
            std::vector <double>().swap(vVelocity);
            std::vector <double>().swap(bHistoryBestPosition);
            vPosition.assign(temp.vPosition.begin(), temp.vPosition.end());
            vVelocity.assign(temp.vVelocity.begin(), temp.vVelocity.end());
            bHistoryBestPosition.assign(temp.bHistoryBestPosition.begin(), temp.bHistoryBestPosition.end());
            dFitness = temp.dFitness;
            dHistoryBestFitness = temp.dHistoryBestFitness;
        }
    };
    //
    class LSSVMPSO
    {
    public:
        int nAllPointNum; // 点数
        int nTrainPointNum;
        int nTestPointNum;
        double tempMagnetic; //磁异常减数
        double minMag;
        double maxMag;
        double minX,maxX;
        double minY,maxY;
        double rms;
        void get_tempMagnetic();
        void split(double n); // 划分测试集和训练集,n为训练集占比如0.7

        /* ------ 最小二乘 ------ */ 
        Matrix<double, Dynamic, 2> MAllCoordinate; // 总
        Matrix<double, Dynamic, 1> MAllMagAnomaly; // 
        Matrix<double, Dynamic, 2> MTrainCoordinate; // 训练集
        Matrix<double, Dynamic,1> MTrainMagAnomaly; // 
        Matrix<double, Dynamic, 2> MTestCoordinate; // 测试集
        Matrix<double, Dynamic, 1> MTestMagAnomaly; // 
        Matrix<double, Dynamic, Dynamic> MB; // B系数阵
        Matrix<double, Dynamic, Dynamic> ML; // L阵
        Matrix<double, Dynamic, Dynamic> Mv; // v改正数
        void ReadData_LSSVMPSO(Datapoint& datapoint); // 读取所需数据
        double Gauss(double ix,double iy, double jx, double jy, double sigma); // 高斯核函数
        void get_B(double sigma,double C); // 获取B矩阵
        void get_B2(double sigma,double C);
        void get_L(); // 获取L矩阵
        void get_L2();
        void cal_v(); // 最小二乘求解v


        /* ------ 粒子群优化 ------ */
        int nDim;
        int nParticleNum; // 粒子数
        Particle* vGroup; // 粒子群
        Particle PGlobalBestParticle; //搜索过程得到的全局最优粒子
        double* vPositionMinValue; //粒子位置的最小界
        double* vPositionMaxValue; //粒子位置的最大界
        double* vVelocityMinValue; //粒子速度的最小界
        double* vVelocityMaxValue; //粒子速度的最大界
        double dC1; // 局部搜索能力
        double dC2; // 全局搜索能力
        int nMaxGen; // 最大进化数量
        double dk; // 扰动速度 v=kx
        double dWeightV; //(wV best belongs to [0.8,1.2]),速率更新公式中速度前面的弹性系数
        double dWeightP; // 种群更新公式中速度前面的弹性系数
        double dMaxSpeed; //粒子允许最大速度
        double eps;
        double* vAvgFitnessGen;
        Matrix<double, Dynamic, Dynamic> result; // 回归函数

        // 在0-1上均匀分布的随机数。使用线程局部的mt19937而不是全局rand()：
        // rand()的内部状态是进程全局的，从多个线程并发调用是数据竞争（未定义行为），
        // 而CalFitness现在会被并行地对每个粒子调用。
        double rand0_1(void) {
            thread_local std::mt19937 gen{std::random_device{}()};
            thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(gen);
        }
        double CalFitness(double sigma,double C);
        void RandomlyInitial(); // 初始化
        void Refresh(); // 更新粒子群 迭代寻优
        void setPara(); // 设置PSO参数
        void RegressionFunc(Datapoint& datapoint);
        void cal(Datapoint& trainData,Datapoint& datapoint,double para1,double para2,std::string out);
        
        void run(Datapoint& all,Datapoint& train,QString out);
        void save_para(Datapoint& datapoint,double& rms); // 保存最优参数到txt文件
        void save_result(Datapoint& all,QString out); // 保存经纬度、XY、估计值、真实值、差值
        void readPara(int& p1, double& p2, double& p3, double& p4, double& p5,
                      double& p6, double& p7,int& p8, double& p9, double& p10, double& p11);

        LSSVMPSO() : vGroup(nullptr), vPositionMinValue(nullptr), vPositionMaxValue(nullptr),
                     vVelocityMinValue(nullptr), vVelocityMaxValue(nullptr), vAvgFitnessGen(nullptr) {}
        ~LSSVMPSO()
        {
            delete[] vGroup;
            delete[] vPositionMinValue;
            delete[] vPositionMaxValue;
            delete[] vVelocityMinValue;
            delete[] vVelocityMaxValue;
            delete[] vAvgFitnessGen;
        }
        LSSVMPSO(const LSSVMPSO&) = delete;
        LSSVMPSO& operator=(const LSSVMPSO&) = delete;
    };

}

