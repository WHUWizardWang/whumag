#include "extension.h"

#include "statsutil.h"

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

void yanTuo::createGrid(const Geomagnetic::Datapoint& datapoints,
                        double step_x,
                        double step_y,
                        Eigen::MatrixXd& X1,
                        Eigen::MatrixXd& Y1,
                        Eigen::MatrixXd& T1)
{
    if (datapoints.empty()) return;

    if (step_x <= 0.0 || step_y <= 0.0 || !std::isfinite(step_x) || !std::isfinite(step_y)) {
        X1.resize(0, 0);
        Y1.resize(0, 0);
        T1.resize(0, 0);
        return;
    }

    // Step 1: 提取并排序唯一的输入坐标 (已为米单位)
    std::vector<double> xs, ys;
    xs.reserve(datapoints.size());
    ys.reserve(datapoints.size());
    for (const auto& kv : datapoints) {
        xs.push_back(kv.second.X);
        ys.push_back(kv.second.Y);
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

    int in_cols = static_cast<int>(xs.size());
    int in_rows = static_cast<int>(ys.size());

    // Step 2: 构建输入格网的值矩阵 T_in
    Eigen::MatrixXd T_in(in_rows, in_cols);
    std::map<std::pair<double,double>, double> valueMap;
    for (const auto& kv : datapoints) {
        valueMap[{kv.second.Y, kv.second.X}] = kv.second.tMagnetic;
    }
    for (int i = 0; i < in_rows; ++i) {
        for (int j = 0; j < in_cols; ++j) {
            T_in(i, j) = valueMap[{ys[i], xs[j]}];
        }
    }

    // Step 3: 计算输出格网范围与尺寸 (以米为单位)
    double minX = xs.front(), maxX = xs.back();
    double minY = ys.front(), maxY = ys.back();
    int rows = static_cast<int>((maxY - minY) / step_y) + 1;
    int cols = static_cast<int>((maxX - minX) / step_x) + 1;

    X1.resize(rows, cols);
    Y1.resize(rows, cols);
    T1.resize(rows, cols);

    // Step 4: 双线性插值或最近邻填充输出格网
    for (int i = 0; i < rows; ++i) {
        double y = minY + i * step_y;
        for (int j = 0; j < cols; ++j) {
            double x = minX + j * step_x;
            X1(i, j) = x;
            Y1(i, j) = y;

            auto itx1 = std::upper_bound(xs.begin(), xs.end(), x);
            auto ity1 = std::upper_bound(ys.begin(), ys.end(), y);
            if (itx1 == xs.begin() || ity1 == ys.begin() || itx1 == xs.end() || ity1 == ys.end()) {
                // 边界外，最近邻插值
                double minDist = std::numeric_limits<double>::max();
                double nearestT = 0;
                for (const auto& kv : datapoints) {
                    double dx = kv.second.X - x;
                    double dy = kv.second.Y - y;
                    double dist = dx * dx + dy * dy;
                    if (dist < minDist) {
                        minDist = dist;
                        nearestT = kv.second.tMagnetic;
                    }
                }
                T1(i, j) = nearestT;
                continue;
            }
            int ix1 = static_cast<int>(itx1 - xs.begin());
            int ix0 = ix1 - 1;
            int iy1 = static_cast<int>(ity1 - ys.begin());
            int iy0 = iy1 - 1;

            double x0 = xs[ix0], x1_ = xs[ix1];
            double y0 = ys[iy0], y1_ = ys[iy1];
            double f00 = T_in(iy0, ix0);
            double f10 = T_in(iy0, ix1);
            double f01 = T_in(iy1, ix0);
            double f11 = T_in(iy1, ix1);

            double tx = (x - x0) / (x1_ - x0);
            double ty = (y - y0) / (y1_ - y0);

            T1(i, j) = (1 - tx) * (1 - ty) * f00
                       + tx * (1 - ty) * f10
                       + (1 - tx) * ty * f01
                       + tx * ty * f11;
        }
    }
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
    StatsResult s_dt = computeStats(dt);
    StatsResult s_down = computeStats(down);
    StatsResult s_delt = computeStats(delt);

    // 输出统计数据

    out = out+"Statistics of the difference between theoretical and computed values:\n";
    out = out + "Max_dt: " +QString::number(s_dt.max) +
            ", Min_dt: " +QString::number(s_dt.min) +
            ", Mean_dt: " +QString::number(s_dt.mean) +
            ", Std Dev_dt: " +QString::number(s_dt.stddev) +"\n";
    out = out + "Max_down: " +QString::number(s_down.max)+
            ", Min_down: " +QString::number(s_down.min)+
            ", Mean_down: " +QString::number(s_down.mean)+
            ", Std Dev_down: " +QString::number(s_down.stddev)+"\n";
    out = out + "Max_delt: " +QString::number(s_delt.max)+
            ", Min_delt: "  +QString::number(s_delt.min) +
            ", Mean_delt: " +QString::number(s_delt.mean)+
            ", Std Dev_delt: " +QString::number(s_delt.stddev)+"\n";

}
void yanTuo::up_run(Geomagnetic::Datapoint& datapoints, double step_x, double step_y, double h, std::string outfile)
{
    createGrid(datapoints, step_x, step_y, X1, Y1, T1);
    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步，进行二维FFT变换
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = calculation_up(step_x, step_y, h, row2, col2, fx);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    upT = fftw.ifft_2d(row2, col2, ff);
    up = get_result(upT, T1, data);
    fftw_free(upT);

    //输出向上延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
    if (!fout.is_open()) {
        std::cerr << "无法打开输出文件: " << outfile << std::endl;
        return;
    }

    // 按行列输出：X1, Y1, 原始 T1, 延拓结果 up
    for (int i = 0; i < row1; ++i) {
        for (int j = 0; j < col1; ++j) {
            double x  = X1(i, j);
            double y  = Y1(i, j);
            double t2 = up[i * col1 + j];
            fout << x << " "
                 << y << " "
                 << t2 << std::endl;
        }
    }
    fout.close();
}
void yanTuo::up_run_BL(Geomagnetic::Datapoint& datapoints, double step_x_deg, double step_y_deg, double h, std::string outfile)
{
    try {
        qDebug() << "开始向上延拓处理, 数据点数:" << datapoints.size();

        if (datapoints.empty()) {
            throw std::runtime_error("输入数据为空");
        }

        // 保存原始的经纬度坐标
        std::map<int, std::pair<double, double>> originalCoords;
        int idx = 0;
        for (const auto& kv : datapoints) {
            originalCoords[idx] = {kv.second.lon, kv.second.lat};
            idx++;
        }

        // 间隔转换
        double sumLat = 0.0;
        for (const auto& kv : datapoints) {
            sumLat += kv.second.lat;
        }
        double latMean = sumLat / datapoints.size();

        const double earthR = 6371000.0;
        const double deg2rad = M_PI / 180.0;
        double meterPerDegLat = earthR * deg2rad;
        double meterPerDegLon = earthR * std::cos(latMean * deg2rad) * deg2rad;

        double step_x = step_x_deg * meterPerDegLon;
        double step_y = step_y_deg * meterPerDegLat;
        h = h * 1000;

        // 坐标转换
        for (auto& kv : datapoints) {
            kv.second.X = kv.second.lon * meterPerDegLon;
            kv.second.Y = kv.second.lat * meterPerDegLat;
        }

        qDebug() << "坐标转换完成";

        // 格网化
        createGrid(datapoints, step_x, step_y, X1, Y1, T1);
        qDebug() << "格网化完成, 网格大小:" << T1.rows() << "x" << T1.cols();

        if (T1.rows() == 0 || T1.cols() == 0) {
            throw std::runtime_error("格网化失败，网格为空");
        }

        // 扩边处理
        data = addBorder(T1);
        row1 = T1.rows();
        col1 = T1.cols();
        row2 = data.rows();
        col2 = data.cols();

        qDebug() << "扩边处理完成:" << row1 << "x" << col1 << " -> " << row2 << "x" << col2;

        // FFT 变换
        fx = nullptr;  // 初始化为空指针
        try {
            fx = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
            if (!fx) {
                throw std::runtime_error("FFT 内存分配失败");
            }

            // 注意：这里假设 fft_2d 函数会填充 fx，而不是重新分配
            fftw_complex* temp_fx = fftw.fft_2d(data, row2, col2);
            if (temp_fx != fx) {
                // 如果 fft_2d 返回了不同的指针，需要复制数据
                memcpy(fx, temp_fx, sizeof(fftw_complex) * row2 * col2);
                fftw_free(temp_fx);
            }

            fftw.fftshift(fx, row2, col2);
            qDebug() << "FFT 变换完成";

        } catch (...) {
            if (fx) {
                fftw_free(fx);
                fx = nullptr;
            }
            throw;
        }

        // 计算延拓
        ff = nullptr;
        try {
            ff = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
            if (!ff) {
                throw std::runtime_error("延拓计算内存分配失败");
            }

            fftw_complex* temp_ff = calculation_up(step_x, step_y, h, row2, col2, fx);
            if (temp_ff != ff) {
                memcpy(ff, temp_ff, sizeof(fftw_complex) * row2 * col2);
                fftw_free(temp_ff);
            }

            fftw.fftshift(ff, row2, col2);
            qDebug() << "延拓计算完成";

        } catch (...) {
            if (ff) {
                fftw_free(ff);
                ff = nullptr;
            }
            if (fx) {
                fftw_free(fx);
                fx = nullptr;
            }
            throw;
        }

        // 清理 fx
        if (fx) {
            fftw_free(fx);
            fx = nullptr;
        }

        // 逆变换
        upT = nullptr;
        try {
            upT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * row2 * col2);
            if (!upT) {
                throw std::runtime_error("逆变换内存分配失败");
            }

            fftw_complex* temp_upT = fftw.ifft_2d(row2, col2, ff);
            if (temp_upT != upT) {
                memcpy(upT, temp_upT, sizeof(fftw_complex) * row2 * col2);
                fftw_free(temp_upT);
            }

            up = get_result(upT, T1, data);
            qDebug() << "逆变换完成";

        } catch (...) {
            if (upT) {
                fftw_free(upT);
                upT = nullptr;
            }
            if (ff) {
                fftw_free(ff);
                ff = nullptr;
            }
            throw;
        }

        // 输出结果
        std::ofstream fout(outfile);
        if (!fout.is_open())
            throw std::runtime_error("无法打开输出文件: " + outfile);

        qDebug() << "开始写入结果文件:" << QString::fromStdString(outfile);

        int pointCount = 0;
        for (int i = 0; i < row1; ++i) {
            for (int j = 0; j < col1; ++j) {
                std::size_t index = static_cast<std::size_t>(i) * col1 + j;
                Q_ASSERT(index < static_cast<std::size_t>(row1) * col1); // 越界即中断

                // 将平面坐标转换回经纬度
                double x_meter = X1(i, j);
                double y_meter = Y1(i, j);
                double lon = x_meter / meterPerDegLon;
                double lat = y_meter / meterPerDegLat;
                double t2  = up[index];

                if (std::isnan(lon) || std::isnan(lat) || std::isnan(t2))
                    continue;

                fout << std::fixed << std::setprecision(6)
                     << lon << ' ' << lat << ' '
                     << std::setprecision(3) << t2 << '\n';
                ++pointCount;
            }
        }
        fout.close();
        qDebug() << "结果写入完成，共" << pointCount << "个数据点（经纬度坐标）";

        /******************** 现在再释放 FFTW 缓冲 ****************/
        if (upT) { fftw_free(upT); upT = nullptr; }
    } catch (const std::exception& e) {
        qCritical() << "up_run_BL 异常:" << e.what();
        throw;
    }
    
}

void yanTuo::down_run(Geomagnetic::Datapoint& datapoints,double step_x, double step_y,
                      double h, std::string outfile, int choice)
{
    //格网化
    // createGrid(X, gridro);
    createGrid(datapoints, step_x, step_y, X1, Y1, T1);
    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步 进行二维fft变换
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = calculation_down(step_x, step_y, h, row2, col2, fx, choice);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    downT = fftw.ifft_2d(row2, col2, ff);
    down = get_result(downT, T1, data);
    fftw_free(downT);

    //输出向下延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
    if (!fout.is_open()) {
        std::cerr << "无法打开输出文件: " << outfile << std::endl;
        return;
    }

    // 按行列输出：X1, Y1, 原始 T1, 延拓结果 up
    for (int i = 0; i < row1; ++i) {
        for (int j = 0; j < col1; ++j) {
            double x  = X1(i, j);
            double y  = Y1(i, j);
            double t2 = down[i * col1 + j];
            fout << x << " "
                 << y << " "
                 << t2 << std::endl;
        }
    }
    fout.close();

}

void yanTuo::down_run_BL(Geomagnetic::Datapoint& datapoints,double step_x_deg, double step_y_deg,
                         double h, std::string outfile, int choice)
{
    // 间隔转换
    double sumLat = 0.0;
    for (const auto& kv : datapoints) {
        sumLat += kv.second.lat;
    }
    double latMean = sumLat / datapoints.size();
    const double earthR = 6371000.0;             // Earth radius in meters
    const double deg2rad = M_PI / 180.0;
    double meterPerDegLat = earthR * deg2rad;    // meters per degree latitude
    double meterPerDegLon = earthR * std::cos(latMean * deg2rad) * deg2rad; // meters per degree longitude
    double step_x = step_x_deg * meterPerDegLon;
    double step_y = step_y_deg * meterPerDegLat;
    h = h * 1000;
    for (auto& kv : datapoints) {
        kv.second.X = kv.second.lon * meterPerDegLon;
        kv.second.Y = kv.second.lat * meterPerDegLat;
    }
    //格网化
    createGrid(datapoints, step_x, step_y, X1, Y1, T1);

    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步 进行二维fft变换
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = calculation_down(step_x, step_y, h, row2, col2, fx, choice);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    downT = fftw.ifft_2d(row2, col2, ff);
    down = get_result(downT, T1, data);
    fftw_free(downT);

    //输出向下延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
    if (!fout.is_open())
        throw std::runtime_error("无法打开输出文件: " + outfile);

    for (int i = 0; i < row1; ++i) {
        for (int j = 0; j < col1; ++j) {
            std::size_t idx = static_cast<std::size_t>(i)*col1 + j;

            double x_meter = X1(i,j);
            double y_meter = Y1(i,j);
            double lon = x_meter / meterPerDegLon;   // 经度（度）
            double lat = y_meter / meterPerDegLat;   // 纬度（度）
            double t2  = down[idx];

            fout << std::fixed << std::setprecision(6)
                 << lon << " " << lat << " "
                 << std::setprecision(3) << t2 << "\n";
        }
    }
}
void yanTuo::evaluatePrecision(Geomagnetic::Datapoint& datapoints,bool useBL,
                               double step_x_deg, double step_y_deg, double h, int choice,std::string outfile)
{

    double step_x, step_y;
    double sumLat = 0.0;
    for (const auto& kv : datapoints) {
        sumLat += kv.second.lat;
    }
    double latMean = sumLat / datapoints.size();
    const double earthR = 6371000.0;             // Earth radius in meters
    const double deg2rad = M_PI / 180.0;
    double meterPerDegLat = earthR * deg2rad;    // meters per degree latitude
    double meterPerDegLon = earthR * std::cos(latMean * deg2rad) * deg2rad; // meters per degree longitude
    if(useBL)
    {
        // 间隔转换
        step_x = step_x_deg * meterPerDegLon;
        step_y = step_y_deg * meterPerDegLat;
        h = h * 1000;
        for (auto& kv : datapoints) {
            kv.second.X = kv.second.lon * meterPerDegLon;
            kv.second.Y = kv.second.lat * meterPerDegLat;
        }
    }
    else
    {
        step_x = step_x_deg;
        step_y = step_y_deg;
    }
    createGrid(datapoints, step_x, step_y, X1, Y1, T1);
    /*------------------------------------------------------- 先获取向上延拓结果，作为向下延拓的起算数据----------------------------------------------------------------------- */
    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步，进行二维FFT变换
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = calculation_up(step_x, step_y, h, row2, col2, fx);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    upT = fftw.ifft_2d(row2, col2, ff);
    up = get_result(upT, T1, data);
    fftw_free(upT);

    /* ----------------------------------------------利用向上延拓结果，向下延拓至原始平面进行精度评估 ------------------------------------------------------------------------*/
    //格网化
    // createGrid(up, gridrow, gridcol, T1);

    //第一步，进行扩边处理
    data = addBorder(T1);
    row1 = T1.rows();
    col1 = T1.cols();
    row2 = data.rows();
    col2 = data.cols();

    //第二步 进行二维fft变换
    fx = fftw.fft_2d(data, row2, col2);
    fftw.fftshift(fx, row2, col2);

    //第三步 计算对应的角频率u、v、延拓因子Q，及延拓公式，得到延拓后的磁异常频谱
    ff = calculation_down(step_x, step_y, h, row2, col2, fx, choice);
    fftw.fftshift(ff, row2, col2);
    fftw_free(fx);

    //第五步 傅立叶逆变换，重构延拓后的磁异常
    downT = fftw.ifft_2d(row2, col2, ff);
    down = get_result(downT, T1, data);
    fftw_free(downT);

    //输出向下延拓结果 X Y Z T1 T2
    std::ofstream fout(outfile);
    if(!useBL)
    {
        for (int i = 0; i < row1; ++i) {
            for (int j = 0; j < col1; ++j) {
                double x  = X1(i, j);
                double y  = Y1(i, j);
                double t2 = down[i * col1 + j];
                fout << x << " "
                     << y << " "
                     << t2 << std::endl;
            }
        }
    }
    else
    {
        for (int i = 0; i < row1; ++i) {
            for (int j = 0; j < col1; ++j) {
                std::size_t idx = static_cast<std::size_t>(i)*col1 + j;

                double x_meter = X1(i,j);
                double y_meter = Y1(i,j);
                double lon = x_meter / meterPerDegLon;   // 经度（度）
                double lat = y_meter / meterPerDegLat;   // 纬度（度）
                double t2  = down[idx];

                fout << std::fixed << std::setprecision(6)
                     << lon << " " << lat << " "
                     << std::setprecision(3) << t2 << "\n";
            }
        }
    }
    fout.close();

    /* ------------------------------------------------------利用上面的计算结果进行精度评估,输出精度报告 ------------------------------------------------------------------------*/

    out = out + "*************输出精度报告***********\n";
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
    std::vector<double> T;
    for (int i = 0; i < row1; ++i) {
        for (int j = 0; j < col1; ++j) {
            T.push_back(T1(i, j));
        }
    }
    TongJi(T, down);
}

