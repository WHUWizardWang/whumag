#include "extension.h"

#include "fftw.h"

FFTW::FFTW()
{
}

FFTW::~FFTW()
{
}

void FFTW::fftshift(fftw_complex* data, int rows, int cols)
{
    // 创建一个临时数组来辅助频谱中心化
    std::vector<fftw_complex> temp(rows * cols);

    // 频谱中心化处理
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            // 计算中心化后的新位置
            int new_i = (i + rows / 2) % rows; // 使用模运算实现环形移动
            int new_j = (j + cols / 2) % cols;

            // 复制原始数据到临时数组
            temp[new_i * cols + new_j][0] = data[i * cols + j][0];
            temp[new_i * cols + new_j][1] = data[i * cols + j][1];
        }
    }

    // 从临时数组复制回中心化后的数据
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            data[i * cols + j][0] = temp[i * cols + j][0];
            data[i * cols + j][1] = temp[i * cols + j][1];
        }
    }
}

fftw_complex* FFTW::fft_2d(const MatrixXd& real_input, int& rows, int& cols)
{
    fftw_complex* in = fftw_alloc_complex(rows * cols);
    fftw_complex* out = fftw_alloc_complex(rows * cols);
    fftw_plan plan = fftw_plan_dft_2d(rows, cols, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            in[i * cols + j][0] = real_input(i, j);  // Real part
            in[i * cols + j][1] = 0.0;               // Imaginary part
        }
    }
    fftw_execute(plan);
    fftw_destroy_plan(plan);
    fftw_free(in);
    return out;
    fftw_free(out);
}

fftw_complex* FFTW::ifft_2d(int& rows,int& cols,fftw_complex* in)
{

    fftw_complex* out = fftw_alloc_complex(rows * cols);

    // 规划和执行二维傅立叶逆变换
    fftw_plan plan = fftw_plan_dft_2d(rows, cols, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(plan);

    // 清理FFTW分配的内存
    fftw_destroy_plan(plan);
    fftw_free(in);

    //执行归一化
    for (int i = 0; i < rows * cols; i++)
    {
        out[i][0] = out[i][0] / rows / cols;
        out[i][1] = out[i][1] / rows / cols;
    }

    return out;
    fftw_free(out);
}

yanTuo::yanTuo()
{
}

yanTuo::~yanTuo()
{
}

void yanTuo::createGrid(std::vector<double>a, int rows, int cols, MatrixXd& A)
{
    int i = 0;
    int j = 0;
    Eigen::MatrixXd  B(rows, cols);
    for (i = 0; i < rows; i = i + 1)
    {
        for (j = 0; j < cols; j = j + 1)
        {
            B(i, j) = a[j + i * cols];
        }
    }
    A = B;
}

MatrixXd yanTuo::addBorder(MatrixXd A)
{
    int m = A.rows();
    int n = A.cols();
    int mn1 = pow(2, ceil(log2(m)));
    int mn2 = pow(2, ceil(log2(n)));

    // Zero-padding
    MatrixXd B = MatrixXd::Zero(mn1, mn2);
    int m1 = (mn1 - m) / 2;
    int n1 = (mn2 - n) / 2;

    B.block(m1, n1, m, n) = A;  // 将输入矩阵 a 数据复制到中心

    //扩边
    for (int i = m1 - 1; i >= 0; --i) {
        B.row(i) = B.row(i + 1);  // 复制下一行
    }
    // 下边界
    for (int i = m1 + m; i < mn1; ++i) {
        B.row(i) = B.row(i - 1);  // 复制上一行
    }
    // 左边界
    for (int j = n1 - 1; j >= 0; --j) {
        B.col(j) = B.col(j + 1);  // 复制右一列
    }
    // 右边界
    for (int j = n1 + n; j < mn2; ++j) {
        B.col(j) = B.col(j - 1);  // 复制左一列
    }

    return B;
}

double yanTuo::calculate_H(double R,int choice)
{
    // 初始化参数
    int s = 1;
    int nn = 13; // 迭代次数
    double sgm = 0.95; // 正则化参数值

    // 计算不同的迭代方法的因子
    double Tik = R / (sgm + R * R);// Tikhonov正则化法向下延拓算子                                                   （choice=1）
    double Integral_iteration = 1.0 / R * (1.0 - std::pow(1.0 - s * R, nn));// 积分迭代法向下延拓算子                （choice=2）
    double Landweber_iteration = 1.0 / R * (1.0 - std::pow(1.0 - sgm * R * R, nn));// Landweber迭代法向下延拓算子    （choice=3）
    double Tik_iteration = 1.0 / R * (1.0 - std::pow(sgm / (R * R + sgm), nn));// 迭代Tikhonov正则化法向下延拓算子   （choice=4）

    // 判断用户选择的延拓算子
    if (choice == 1) 
    {
        return Tik;
    }

    if (choice == 2)
    {
        return Integral_iteration;
    }
    if (choice == 3)
    {
        return Landweber_iteration;
    }
    if (choice == 4)
    {
        return Tik_iteration;
    }
}

fftw_complex* yanTuo::calculation_up(double xint,double yint,double h,int rows,int cols,fftw_complex*in)
{
    double wnx = 2 * M_PI / (xint * rows);
    double wny = 2 * M_PI / (yint * cols);
    int cx = rows / 2 + 1;
    int cy = cols / 2 + 1;

    fftw_complex* ff;
    ff = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * rows * cols);

    // 计算向上延拓后的磁异常频谱
    for (int i = 1; i < rows + 1; ++i) {
        double freqx = ((i - cx) * wnx);
        for (int j = 1; j < cols + 1; ++j) {
            double freqy = ((j - cy) * wny);
            double freq = std::sqrt(freqx * freqx + freqy * freqy);
            double R = std::exp(-freq * h);
            ff[(i - 1) * cols + j - 1][0] = in[(i - 1) * cols + j - 1][0] * R;
            ff[(i - 1) * cols + j - 1][1] = in[(i - 1) * cols + j - 1][1] * R;
        }
    }

    return ff;
}

fftw_complex* yanTuo::calculation_down(double xint, double yint, double h, int ln, int col, fftw_complex* in,int choice)
{
    //第三步 计算对应的角频率u和v
    double wnx2 = 2 * M_PI / (xint * ln);
    double wny2 = 2 * M_PI / (yint * col);
    int cx2 = ln / 2 + 1;
    int cy2 = col / 2 + 1;

    //第四步，计算延拓因子Q，及延拓公式，得到延拓后的磁异常频谱


    fftw_complex* ff2;
    ff2 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * ln * col);

    for (int i = 1; i < ln + 1; ++i) {
        double freqx2 = (i - cx2) * wnx2; // 计算u
        for (int j = 1; j < col + 1; ++j)
        {
            double freqy2 = (j - cy2) * wny2; // 计算v
            double freq2 = std::sqrt(freqx2 * freqx2 + freqy2 * freqy2);
            double R = std::exp(-freq2 * h);// 计算向上延拓因子

            // 获取不同的迭代方法的因子
            H = calculate_H(R, choice);

            // 应用迭代算子进行计算
            ff2[(i - 1) * col + j - 1][0] = in[(i - 1) * col + j - 1][0] * H;
            ff2[(i - 1) * col + j - 1][1] = in[(i - 1) * col + j - 1][1] * H;

        }
            
    }
    
    return ff2;
}

std::vector<double> yanTuo::get_result(fftw_complex* in,MatrixXd A,MatrixXd B)
{
    int m = A.rows();
    int n = A.cols();
    int mn1 = pow(2, ceil(log2(m)));
    int mn2 = pow(2, ceil(log2(n)));
    int m1 = (mn1 - m) / 2;
    int n1 = (mn2 - n) / 2;

    std::vector<double>upT;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            int idx = (i + m1) * B.cols() + (j + n1);
            upT.push_back(in[idx][0]);
        }
    }
    return upT;
}

void yanTuo::TongJi(std::vector<double>dt, std::vector<double>down)
{
    // 计算差值
    std::vector<double> delt;
    std::transform(dt.begin(), dt.end(), down.begin(), std::back_inserter(delt),
        [](double a, double b) { return a - b; });

    // 计算统计数据
    double max_dt = *std::max_element(dt.begin(), dt.end());
    double min_dt = *std::min_element(dt.begin(), dt.end());
    double sum_dt = std::accumulate(dt.begin(), dt.end(), 0.0);
    double mean_dt = sum_dt / dt.size();
    double sum_sq_diff_dt = std::inner_product(dt.begin(), dt.end(), dt.begin(), 0.0,
        [](double sum, double diff) { return sum + diff * diff; },
        [](double a, double b) { return a + b; });
    double std_dt = std::sqrt(sum_sq_diff_dt / dt.size());

    double max_down = *std::max_element(down.begin(), down.end());
    double min_down = *std::min_element(down.begin(), down.end());
    double sum_down = std::accumulate(down.begin(), down.end(), 0.0);
    double mean_down = sum_down / down.size();
    double sum_sq_diff_down = std::inner_product(down.begin(), down.end(), down.begin(), 0.0,
        [](double sum, double diff) { return sum + diff * diff; },
        [](double a, double b) { return a + b; });
    double std_down = std::sqrt(sum_sq_diff_down / down.size());

    double max_delt = *std::max_element(delt.begin(), delt.end());
    double min_delt = *std::min_element(delt.begin(), delt.end());
    double sum_delt = std::accumulate(delt.begin(), delt.end(), 0.0);
    double mean_delt = sum_delt / delt.size();
    double sum_sq_diff = std::inner_product(delt.begin(), delt.end(), delt.begin(), 0.0,
        [](double sum, double diff) { return sum + diff * diff; },
        [](double a, double b) { return a + b; });
    double std_delt = std::sqrt(sum_sq_diff / delt.size());

    // 输出统计数据

    out = out+"Statistics of the difference between theoretical and computed values:\n";
    out = out + "Max_dt: " +QString::number(max_dt) +
            ", Min_dt: " +QString::number(min_dt) +
            ", Mean_dt: " +QString::number(mean_dt) +
            ", Std Dev_dt: " +QString::number(std_dt) +"\n";
    out = out + "Max_down: " +QString::number(max_down)+
            ", Min_down: " +QString::number(min_down)+
            ", Mean_down: " +QString::number(mean_down)+
            ", Std Dev_down: " +QString::number(std_down)+"\n";
    out = out + "Max_delt: " +QString::number(max_delt)+
            ", Min_delt: "  +QString::number(min_delt) +
            ", Mean_delt: " +QString::number(mean_delt)+
            ", Std Dev_delt: " +QString::number(std_delt)+"\n";

}

void yanTuo::up_run(int gridrow,int gridcol,std::vector<double>X, std::vector<double>Y, std::vector<double>T, double xint, double yint, double h,std::string outfile)
{
    //格网化
    createGrid(X, gridrow, gridcol, X1);
    createGrid(Y, gridrow, gridcol, Y1);
    createGrid(T, gridrow, gridcol, T1);

    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步，进行二维FFT变换
    fx = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * data.rows() * data.cols());
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    ff = calculation_up(xint, yint, h, row2, col2, fx);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    upT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    upT = fftw.ifft_2d(row2, col2, ff);
    up = get_result(upT, T1, data);
    fftw_free(upT);

    //输出向上延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
//    fout << "X Y h T T_up" << endl;
    for (int i = 0; i < up.size(); i = i + 1)
    {
        fout << X[i] << " " << Y[i]  << " " << up[i] << std::endl;
    }
    fout.close();
    
//    cout << "向上延拓处理完成！" << endl;
//    cout << endl << endl << endl << endl;
}

void yanTuo::down_run(int gridrow, int gridcol, std::vector<double>X, std::vector<double>Y, std::vector<double>T, double xint, double yint, double h, std::string outfile, int choice)
{
    //格网化
    createGrid(X, gridrow, gridcol, X1);
    createGrid(Y, gridrow, gridcol, Y1);
    createGrid(T, gridrow, gridcol, T1);

    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步 进行二维fft变换
    fx = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * data.rows() * data.cols());
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    ff = calculation_down(xint, yint, h, row2, col2, fx, choice);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    downT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    downT = fftw.ifft_2d(row2, col2, ff);
    down = get_result(downT, T1, data);
    fftw_free(downT);

    //输出向下延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
//    fout << "X Y h T T_down" << endl;
    for (int i = 0; i < down.size(); i = i + 1)
    {
        fout << X[i] << " " << Y[i] << " " << down[i] << std::endl;
    }
    fout.close();
//    QMessageBox::information(this,"提示","向下延拓处理完成！");

}

void yanTuo::evaluatePrecision(int gridrow, int gridcol, std::vector<double>X, std::vector<double>Y, std::vector<double>T,
                               double xint, double yint, double h, int choice,std::string outfile)
{
    //格网化
    createGrid(X, gridrow, gridcol, X1);
    createGrid(Y, gridrow, gridcol, Y1);
    createGrid(T, gridrow, gridcol, T1);

    /*------------------------------------------------------- 先获取向上延拓结果，作为向下延拓的起算数据----------------------------------------------------------------------- */
    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步，进行二维FFT变换
    fx = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * data.rows() * data.cols());
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    ff = calculation_up(xint, yint, h, row2, col2, fx);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    upT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    upT = fftw.ifft_2d(row2, col2, ff);
    up = get_result(upT, T1, data);
    fftw_free(upT);

    /* ----------------------------------------------利用向上延拓结果，向下延拓至原始平面进行精度评估 ------------------------------------------------------------------------*/
    //格网化
    createGrid(up, gridrow, gridcol, T1);

    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步 进行二维fft变换
    fx = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * data.rows() * data.cols());
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    ff = calculation_down(xint, yint, h, row2, col2, fx, choice);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    downT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
    downT = fftw.ifft_2d(row2, col2, ff);
    down = get_result(downT, T1, data);
    fftw_free(downT);

    //输出向下延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
//    fout << "X Y h T T_down" << endl;
    for (int i = 0; i < down.size(); i = i + 1)
    {
        fout << X[i] << " " << Y[i] << " " << down[i] << std::endl;
    }
    fout.close();

    /* ------------------------------------------------------利用上面的计算结果进行精度评估,输出精度报告 ------------------------------------------------------------------------*/

    out = out + "*************输出精度报告***********\n";
    // 输出相关计算信息
    out = out+"xint: "+QString::number(xint)+"\n";
    out = out+"yint: "+QString::number(yint)+"\n";
    out = out+"h: "+QString::number(h)+"\n";
    out = out+"gridrows: "+QString::number(gridrow)+"\n";
    out = out+"gridcols: "+QString::number(gridcol)+"\n";
    if (choice == 1)
    {
        out = out+"向下延拓因子计算方式:  Tikhonov正则化法向下延拓算子" +"\n";
    }

    if (choice == 2)
    {
        out = out+"向下延拓因子计算方式:  积分迭代法向下延拓算子"  +"\n";
    }
    if (choice == 3)
    {
        out = out+"向下延拓因子计算方式:  Landweber迭代法向下延拓算子"  +"\n";
    }
    if (choice == 4)
    {
        out = out+"向下延拓因子计算方式:  迭代Tikhonov正则化法向下延拓算子"  +"\n";
    }
    out = out+"精度统计信息如下：" + "\n";
    TongJi(T, down);
}

