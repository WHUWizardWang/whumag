#include "LSSVMPSO.h"
#include <numeric>
#include <algorithm>
#include <random>
namespace  Geomagnetic{


void LSSVMPSO::split(double n)
{
    nTrainPointNum = int(nAllPointNum * n);
    nTestPointNum = nAllPointNum - nTrainPointNum;
    MTrainCoordinate.resize(nTrainPointNum, 2);
    MTestCoordinate.resize(nTestPointNum, 2);
    MTrainMagAnomaly.resize(nTrainPointNum, 1);
    MTestMagAnomaly.resize(nTestPointNum, 1);

    // Randomly assign points to train/test instead of taking a contiguous
    // block: points arrive in original survey-file order, so a plain
    // block split can put e.g. only the last few survey lines into the test
    // set, biasing the PSO fitness function toward measuring extrapolation
    // to one edge of the survey rather than general interpolation quality.
    std::vector<int> indices(nAllPointNum);
    std::iota(indices.begin(), indices.end(), 0);
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);

    for (int i = 0; i < nTrainPointNum; ++i) {
        MTrainCoordinate.row(i) = MAllCoordinate.row(indices[i]);
        MTrainMagAnomaly(i, 0) = MAllMagAnomaly(indices[i], 0);
    }
    for (int i = 0; i < nTestPointNum; ++i) {
        MTestCoordinate.row(i) = MAllCoordinate.row(indices[nTrainPointNum + i]);
        MTestMagAnomaly(i, 0) = MAllMagAnomaly(indices[nTrainPointNum + i], 0);
    }
}

void LSSVMPSO::get_tempMagnetic()
{
    //cout << "原始磁场强度" << endl << MAllMagAnomaly << endl << endl;
    Matrix<double, 1, 1> tmp;
    tmp = MAllMagAnomaly.colwise().mean();
    tempMagnetic = round(tmp(0,0));
    minMag = MAllMagAnomaly.minCoeff();
    maxMag = MAllMagAnomaly.maxCoeff();
    minX = MAllCoordinate.block(0, 0, nAllPointNum, 1).minCoeff();
    maxX = MAllCoordinate.block(0, 0, nAllPointNum, 1).maxCoeff();
    minY = MAllCoordinate.block(0, 1, nAllPointNum, 1).minCoeff();
    maxY = MAllCoordinate.block(0, 1, nAllPointNum, 1).maxCoeff();
    //cout << minX << "   " << maxX << "   " << minY << "   " << maxY << endl;
    //cout << "均值" << endl << tempMagnetic << endl << endl;
    //cout << "max: " << maxMag << endl;
    //cout << "min: " << minMag << endl;
}

/* ------ 最小二乘：实现 ------ */
void LSSVMPSO::ReadData_LSSVMPSO(Datapoint& datapoint)
{
    nAllPointNum = datapoint.size();
    MAllCoordinate.resize(nAllPointNum, 2);
    MAllMagAnomaly.resize(nAllPointNum, 1);
    int i = 0;
    for (const auto& elem : datapoint)
    {
        MAllCoordinate(i, 0) = elem.second.X;
        MAllCoordinate(i, 1) = elem.second.Y;
        MAllMagAnomaly(i, 0) = elem.second.tMagnetic;
        i = i + 1;
    }
    get_tempMagnetic();
    Matrix<double, Dynamic, Dynamic> tmp;
    tmp = MatrixXd::Ones(nAllPointNum, 1);
    //MAllMagAnomaly -= tmp * tempMagnetic;
    //MAllMagAnomaly = (MAllMagAnomaly - MatrixXd::Ones(nAllPointNum, 1)*minMag)
    //    *  (1/(maxMag - minMag));

    //cout << "归一化后磁场强度" << endl << MAllMagAnomaly << endl << endl;
}

double LSSVMPSO::Gauss(double ix, double iy, double jx, double jy, double sigma)
{
    double tmp = (ix - jx) * (ix - jx) + (iy - jy) * (iy - jy);
    return exp(-tmp / (sigma * sigma));
}

void LSSVMPSO::get_B(double sigma,double C)
{
    MB = MatrixXd::Ones(nTrainPointNum + 1, nTrainPointNum + 1);
    MB(0, 0) = 0;
    // 对角线
    for (int i = 1; i < nTrainPointNum + 1; i++)
    {
        MB(i, i) = 1 + 1 / C;
    }
    // 上三角或者下三角
    for (int i = 1; i < nTrainPointNum + 1; i++)
    {
        for (int j = i+1; j < nTrainPointNum + 1; j++)
        {
            MB(i, j) = Gauss(MTrainCoordinate(i - 1, 0), MTrainCoordinate(i - 1, 1),
                MTrainCoordinate(j - 1, 0), MTrainCoordinate(j - 1, 1),sigma);
            MB(j, i) = MB(i, j);
        }
    }
}

void LSSVMPSO::get_B2(double sigma,double C)
{
    MB = MatrixXd::Ones(nAllPointNum + 1, nAllPointNum + 1);
    MB(0, 0) = 0;
    // 对角线
    for (int i = 1; i < nAllPointNum + 1; i++)
    {
        MB(i, i) = 1 + 1 / C;
    }
    // 上三角或者下三角
    for (int i = 1; i < nAllPointNum + 1; i++)
    {
        for (int j = i+1; j < nAllPointNum + 1; j++)
        {
            MB(i, j) = Gauss(MAllCoordinate(i - 1, 0), MAllCoordinate(i - 1, 1),
                MAllCoordinate(j - 1, 0), MAllCoordinate(j - 1, 1),sigma);
            MB(j, i) = MB(i, j);
        }
    }
}

void LSSVMPSO::get_L()
{

    ML.resize(nTrainPointNum + 1,1);
    ML(0, 0) = 0;
    ML.block(1, 0, nTrainPointNum, 1) = MTrainMagAnomaly;
    //cout << "ML" << ML << endl << endl;
    //cout << "Test" << MTestMagAnomaly << endl << endl;
}

void LSSVMPSO::get_L2()
{

    ML.resize(nAllPointNum + 1,1);
    ML(0, 0) = 0;
    ML.block(1, 0, nAllPointNum, 1) = MAllMagAnomaly;
    //cout << "ML" << ML << endl << endl;
    //cout << "Test" << MTestMagAnomaly << endl << endl;
}

void LSSVMPSO::run(Datapoint& all,Datapoint& train,QString out)
{
    ReadData_LSSVMPSO(train); // 读取数据
    split(0.7); // 切分数据集
//    setPara(); // 设置参数
    get_L();    //构建误差方程
    RandomlyInitial();  //随机初始化
    Refresh();  // 迭代更新
    RegressionFunc(all); //回归
    save_para(all,rms); // 保存最优参数 和 rms
    save_result(all,out); //保存结果文本
}

void LSSVMPSO::save_para(Datapoint& datapoint,double& rms)
{
    std::ofstream f;
    f.open("best_para.txt");
    f.precision(30);
    f << "sigma: " << PGlobalBestParticle.bHistoryBestPosition[0] << endl;
    f << "C: " << PGlobalBestParticle.bHistoryBestPosition[1] << endl;

    MatrixXd difference;
    difference.resize(datapoint.size(), 1);
    int i = 0;
    for (const auto& elem : datapoint)
    {
        //difference(i, 0) = elem.second.tMagnetic - tempMagnetic - result(i, 0);
        difference(i, 0) = elem.second.tMagnetic - result(i, 0);
        i++;
    }
    Matrix<double, 1, 1> m = difference.transpose() * difference;
    rms = sqrt(m(0, 0) / datapoint.size());
    f << "rms: " << rms << endl;
    f.close();
}

double LSSVMPSO::CalFitness(double sigma, double C)
{
    // Builds its own LOCAL kernel matrix rather than writing to the shared
    // `MB` member (which get_B() does): CalFitness is now called
    // concurrently, once per particle, from Refresh()'s parallel loop, and
    // every call needs an independent sigma/C so they cannot share one
    // mutable matrix without racing.
    Matrix<double, Dynamic, Dynamic> B = MatrixXd::Ones(nTrainPointNum + 1, nTrainPointNum + 1);
    B(0, 0) = 0;
    for (int i = 1; i < nTrainPointNum + 1; i++)
    {
        B(i, i) = 1 + 1 / C;
    }
    for (int i = 1; i < nTrainPointNum + 1; i++)
    {
        for (int j = i + 1; j < nTrainPointNum + 1; j++)
        {
            double v = Gauss(MTrainCoordinate(i - 1, 0), MTrainCoordinate(i - 1, 1),
                              MTrainCoordinate(j - 1, 0), MTrainCoordinate(j - 1, 1), sigma);
            B(i, j) = v;
            B(j, i) = v;
        }
    }

    // B is symmetric by construction; solving it directly (rather than via
    // the normal equations B^T*B, as this used to do) avoids squaring its
    // condition number and an extra O(n^3) matrix product per call.
    Matrix<double, Dynamic, Dynamic> Mx = B.ldlt().solve(ML);

    Matrix<double, Dynamic, Dynamic> guess;
    guess.resize(nTestPointNum, nTrainPointNum + 1);
    for (int j = 0; j < nTestPointNum; j++)
    {
        guess(j, 0) = 1;
        for (int i = 0; i < nTrainPointNum; i++)
        {
            guess(j,i+1) = Gauss(MTrainCoordinate(i, 0), MTrainCoordinate(i, 1),
                MTestCoordinate(j, 0), MTestCoordinate(j, 1), sigma);
        }
    }
    Matrix<double, Dynamic, Dynamic> v;
    v.resize(nTestPointNum, 1);
    v = guess * Mx - MTestMagAnomaly;
    Matrix<double, Dynamic, Dynamic> r;
    r.resize(1, 1);
    r = v.transpose() * v;
    return r(0,0);
}

void LSSVMPSO::Refresh()
{
    double f1 = PGlobalBestParticle.dHistoryBestFitness;
    // 用户在界面上配置的dWeightV作为惯性权重线性递减调度的起点（而不是像过去
    // 那样每一代都被硬编码的0.9起点覆盖，导致这个参数完全不起作用），递减到
    // 固定下限0.4 —— 保留了"惯性权重线性递减"这一被广泛认可的PSO改进，同时让
    // 用户配置的值真正产生影响。
    const double weightVStart = dWeightV;
    const double weightVEnd = 0.4;

    // 循环多代
    for (int i = 0; i < nMaxGen; i++)
    {
        std::cout << "第 " << i+1 << "/" << nMaxGen << "代" << endl;
        dWeightV = (weightVStart - weightVEnd) * (nMaxGen - i) / nMaxGen + weightVEnd;

        // 每个粒子的适应度评估互相独立（只读取上一代结束时才更新一次的
        // PGlobalBestParticle，本代内部不会被修改），因此可以安全并行化 ——
        // 这是本模块里迄今为止最昂贵的计算（每次调用CalFitness都要重建核矩阵
        // 并做一次O(n^3)求解，共调用 nParticleNum*nMaxGen 次）。
        // rand0_1()内部已改为线程局部的mt19937；CalFitness内部已改为构建局部
        // 核矩阵而不是写共享成员MB，两者都是让这里能够安全并行的前提条件。
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < nParticleNum; j++)
        {
            // 循环每个维度
            for (int k = 0; k < vGroup[0].nDim; k++)
            {
                vGroup[j].vVelocity[k] = dWeightV * vGroup[j].vVelocity[k] +
                    dC1 * rand0_1() * (vGroup[j].bHistoryBestPosition[k] - vGroup[j].vPosition[k]) +
                    dC2 * rand0_1() * (PGlobalBestParticle.bHistoryBestPosition[k] - vGroup[j].vPosition[k]);
                if (vGroup[j].vVelocity[k] > vVelocityMaxValue[k])
                    vGroup[j].vVelocity[k] = vVelocityMaxValue[k];
                if (vGroup[j].vVelocity[k] < vVelocityMinValue[k])
                    vGroup[j].vVelocity[k] = vVelocityMinValue[k];
                vGroup[j].vPosition[k] += dWeightP * vGroup[j].vVelocity[k];
                if (vGroup[j].vPosition[k] > vPositionMaxValue[k])
                    vGroup[j].vPosition[k] = vPositionMaxValue[k];
                if (vGroup[j].vPosition[k] < vPositionMinValue[k])
                    vGroup[j].vPosition[k] = vPositionMinValue[k];
            }
            vGroup[j].dFitness = CalFitness(vGroup[j].vPosition[0], vGroup[j].vPosition[1]);
            // 适应度和位置更新
            if (vGroup[j].dFitness < vGroup[j].dHistoryBestFitness)
            {
                vGroup[j].dHistoryBestFitness = vGroup[j].dFitness;
                vGroup[j].bHistoryBestPosition = vGroup[j].vPosition;
            }
        }

        // 全局最优粒子的查找放到并行区之外顺序执行（避免多线程竞争写同一个
        // 索引变量），逻辑与原来完全一致：按粒子编号顺序找到最后一个历史最优
        // 适应度优于当前全局最优的粒子。
        int nGlobalBestParticleIndex = -1;
        double dSumFitness = 0;
        for (int j = 0; j < nParticleNum; j++)
        {
            dSumFitness += vGroup[j].dFitness;
            if (vGroup[j].dHistoryBestFitness < PGlobalBestParticle.dHistoryBestFitness)
                nGlobalBestParticleIndex = j;
        }
        if (nGlobalBestParticleIndex != -1)
            PGlobalBestParticle.copy(vGroup[nGlobalBestParticleIndex]);
        vAvgFitnessGen[i] = dSumFitness / nParticleNum;
        double f2 = PGlobalBestParticle.dHistoryBestFitness;
        std::cout << f2 << endl;
        if (f1 - f2 < eps) // f1-f2 < eps
            break;
        f1 = f2;
    }
}

void LSSVMPSO::RandomlyInitial()
{
    int nGlobalBestParticleIndex = -1;

    //初始化第0个粒子，并初步设为全局最优粒子
    //遍历粒子的任一维度
    for (int j = 0; j < vGroup[0].nDim; j++)
    {
        //随机初始化粒子位置与最佳位置
        double tempVal = vPositionMinValue[j];
        tempVal += rand0_1() * (vPositionMaxValue[j] - vPositionMinValue[j]);
        vGroup[0].vPosition[j] = tempVal;
        vGroup[0].bHistoryBestPosition[j] = tempVal;
        //随机初始化粒子速度
        vGroup[0].vVelocity[j] = rand0_1()* vVelocityMaxValue[j];
    }
    //设定粒子初代适应度值与最佳适应度值
    vGroup[0].dFitness = CalFitness(vGroup[0].vPosition[0], vGroup[0].vPosition[1]);
    vGroup[0].dHistoryBestFitness = vGroup[0].dFitness;
    PGlobalBestParticle.copy(vGroup[0]);

    //初始化1~nParticleNum个粒子
    for (int i = 1; i < nParticleNum; i++)
    {
        for (int j = 0; j < vGroup[0].nDim; j++)
        {
            //初始化位置
            double tempVal = vPositionMinValue[j];
            tempVal += rand0_1() * (vPositionMaxValue[j] - vPositionMinValue[j]);
            vGroup[i].vPosition[j] = tempVal;
            vGroup[i].bHistoryBestPosition[j] = tempVal;
            //初始化速度
            vGroup[i].vVelocity[j] = rand0_1() * vVelocityMaxValue[j];
        }
        //更新粒子初代适应度值与最佳适应度值
        vGroup[i].dFitness = CalFitness(vGroup[i].vPosition[0], vGroup[i].vPosition[1]);
        vGroup[i].dHistoryBestFitness = vGroup[i].dFitness;
        if (vGroup[i].dHistoryBestFitness < PGlobalBestParticle.dHistoryBestFitness)
            nGlobalBestParticleIndex = i;
    }
    //更新粒子群全局最佳数据
    if (nGlobalBestParticleIndex != -1)
        PGlobalBestParticle.copy(vGroup[nGlobalBestParticleIndex]);

}

void LSSVMPSO::setPara()
{
    nDim = 2;
    nParticleNum = 500;
    vGroup = new Particle[nParticleNum];
    double dMinSigma = 0.0001;
    double dMaxSigma = 1000;
    double dMinC = 0.001;
    double dMaxC = 10000;
    vPositionMinValue = new double[nDim];
    vPositionMaxValue = new double[nDim];
    vPositionMinValue[0] = dMinSigma;
    vPositionMinValue[1] = dMinC;
    vPositionMaxValue[0] = dMaxSigma;
    vPositionMaxValue[1] = dMaxC;
    dC1 = 1.6;
    dC2 = 1.5;
    nMaxGen = 50;
    vAvgFitnessGen = new double[nMaxGen];
    dk = 0.6;
    dWeightV = 1;
    dWeightP = 1;

    double dVMaxSigma = dk * dMaxSigma;
    double dVMaxC = dk * dMaxC;
    double dVMinSigma = -dVMaxSigma;
    double dVMinC = -dVMaxC;
    vVelocityMinValue = new double[nDim];
    vVelocityMaxValue = new double[nDim];
    double controlV = 0.5;
    vVelocityMinValue[0] = dVMinSigma * controlV;
    vVelocityMinValue[1] = dVMinC * controlV;
    vVelocityMaxValue[0] = dVMaxSigma * controlV;
    vVelocityMaxValue[1] = dVMaxC * controlV;

    eps = 0.000001;
}

/* ------ 回归计算：实现 ------ */
void LSSVMPSO::RegressionFunc(Datapoint& datapoint)
{
    double dSigma = PGlobalBestParticle.bHistoryBestPosition[0];
    double dC = PGlobalBestParticle.bHistoryBestPosition[1];
    get_B(dSigma,dC);
    // MB is symmetric by construction; solve it directly rather than via the
    // normal equations (which needlessly squares its condition number).
    Matrix<double, Dynamic, Dynamic> Mx = MB.ldlt().solve(ML);

    int nTestNum = datapoint.size();
    Matrix<double, Dynamic, Dynamic> tmp;
    tmp.resize(nTrainPointNum+1, nTestNum);
    int j = 0;
    for (const auto& elem : datapoint)
    {
        tmp(0, j) = 1;
        for (int i = 0; i < nTrainPointNum; i++)
        {
            tmp(i+1, j) = Gauss(MTrainCoordinate(i, 0), MTrainCoordinate(i, 1),
                elem.second.X, elem.second.Y,dSigma);
        }
        j++;
    }
    result.resize(nTestNum,1);
    result = tmp.transpose() * Mx;
}

void LSSVMPSO::cal(Datapoint& trainData,Datapoint& datapoint,double para1,double para2,std::string out)
{
    ReadData_LSSVMPSO(trainData);
    double dSigma = para1;
    double dC = para2;
    get_B2(dSigma,dC);
    get_L2();
    // MB is symmetric by construction; solve it directly rather than via the
    // normal equations (which needlessly squares its condition number).
    Matrix<double, Dynamic, Dynamic> Mx = MB.ldlt().solve(ML);

    int nTestNum = datapoint.size();
    Matrix<double, Dynamic, Dynamic> tmp;
    tmp.resize(nAllPointNum+1, nTestNum);
    int j = 0;
    for (const auto& elem : datapoint)
    {
        tmp(0, j) = 1;
        for (int i = 0; i < nAllPointNum; i++)
        {
            tmp(i+1, j) = Gauss(MAllCoordinate(i, 0), MAllCoordinate(i, 1),
                elem.second.X, elem.second.Y,dSigma);
        }
        j++;
    }
    result.resize(nTestNum,1);
    result = tmp.transpose() * Mx;
    save_result(datapoint,QString::fromStdString(out));
}

void LSSVMPSO::save_result(Datapoint& all,QString out)
{
   // 打开文件
   std::ofstream outputFile(out.toStdString());

   // 检查文件是否成功打开
   if (!outputFile.is_open())
   {
       std::cout << "无法打开文件！" << std::endl;
       return;
   }
   int fl=0;
   // 将数据写入文件
   for (const auto& pair : all)
   {
       const SinglePoint& point = pair.second;
       outputFile << point.X << " "
           << point.Y << " "
           << result(fl,0) << std::endl;
       fl++;
   }

   // 关闭文件
   outputFile.close();
   std::cout << "数据已成功写入文件 " << std::endl;
}

void LSSVMPSO::readPara(int& p1, double& p2, double& p3, double& p4, double& p5,
              double& p6, double& p7,int& p8, double& p9, double& p10, double& p11)
{
    nDim = 2;
    nParticleNum = p1;
    vGroup = new Particle[nParticleNum];
    double dMinSigma = p2;
    double dMaxSigma = p3;
    double dMinC = p4;
    double dMaxC = p5;
    vPositionMinValue = new double[nDim];
    vPositionMaxValue = new double[nDim];
    vPositionMinValue[0] = dMinSigma;
    vPositionMinValue[1] = dMinC;
    vPositionMaxValue[0] = dMaxSigma;
    vPositionMaxValue[1] = dMaxC;
    dC1 = p6;
    dC2 = p7;
    nMaxGen = p8;
    vAvgFitnessGen = new double[nMaxGen];
    dk = p9;
    dWeightV = p10;
    dWeightP = p11;

    double dVMaxSigma = dk * dMaxSigma;
    double dVMaxC = dk * dMaxC;
    double dVMinSigma = -dVMaxSigma;
    double dVMinC = -dVMaxC;
    vVelocityMinValue = new double[nDim];
    vVelocityMaxValue = new double[nDim];
    double controlV = 0.5;
    vVelocityMinValue[0] = dVMinSigma * controlV;
    vVelocityMinValue[1] = dVMinC * controlV;
    vVelocityMaxValue[0] = dVMaxSigma * controlV;
    vVelocityMaxValue[1] = dVMaxC * controlV;

    eps = 0.000001;

}

}
