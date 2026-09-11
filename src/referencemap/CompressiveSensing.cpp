// CompressiveSensing.cpp
#include "CompressiveSensing.h"
#include <QDebug>
#include <numeric>
#include <algorithm>

CompressiveSensing::ReconstructionResult
CompressiveSensing::reconstruct(const Geomagnetic::Datapoint& data,
                                int n_nonzero_coefs,
                                int sampling_factor)
{
    ReconstructionResult result;
    int data_size = data.size();

    // A dense N x N DCT dictionary (N = total survey points) is the
    // dominant cost here: unlike the Polyhedral/Spline paths in
    // GeomagneticModel.cpp, this had no size guard at all, so a large
    // survey could try to allocate tens of GB and hang/crash the UI with no
    // warning. Refuse rather than do that.
    const int maxPoints = 5000;
    if (data_size > maxPoints) {
        qWarning() << "CompressiveSensing::reconstruct:" << data_size
                   << "points exceeds the" << maxPoints
                   << "-point limit for a dense DCT dictionary; aborting.";
        return result;
    }
    if (data_size == 0) {
        return result;
    }

    // 提取数据。DCT字典按1维顺序对信号做变换，为了让相邻下标在空间上也相邻
    // （否则1维DCT捕捉不到磁场的2维空间相关性），这里先按X、Y坐标排序，
    // 构建/重构都在这个排序后的顺序下进行，最后再按原始顺序整理输出。
    struct IndexedPoint { double x, y, t; };
    std::vector<IndexedPoint> pts;
    pts.reserve(data_size);
    for (const auto& [_, point] : data) {
        pts.push_back({point.X, point.Y, point.tMagnetic});
    }
    std::vector<int> spatialOrder(data_size);
    std::iota(spatialOrder.begin(), spatialOrder.end(), 0);
    std::sort(spatialOrder.begin(), spatialOrder.end(), [&pts](int a, int b) {
        if (pts[a].x != pts[b].x) return pts[a].x < pts[b].x;
        return pts[a].y < pts[b].y;
    });

    Eigen::VectorXd magnetic_anomalies(data_size);
    result.x.resize(data_size);
    result.y.resize(data_size);
    for (int i = 0; i < data_size; ++i) {
        const IndexedPoint& p = pts[spatialOrder[i]];
        magnetic_anomalies(i) = p.t;
        result.x[i] = p.x;
        result.y[i] = p.y;
    }

    // 创建DCT字典
    Eigen::MatrixXd dct_dict = createDctDictionary(data_size);

    // 随机选取"实际测到"的位置 —— 不再像过去那样把未采样位置直接置零后
    // 当作真值参与拟合，而是只保留这些位置真正参与重构计算。
    std::vector<int> sampled_indices = selectSampledIndices(data_size, sampling_factor);
    Eigen::VectorXd measured(sampled_indices.size());
    for (size_t k = 0; k < sampled_indices.size(); ++k) {
        measured(static_cast<int>(k)) = magnetic_anomalies(sampled_indices[k]);
    }

    // 稀疏重构：只用"实际测到"的位置做拟合
    Eigen::VectorXd sparse_code = sparseReconstruction(measured, dct_dict, sampled_indices, n_nonzero_coefs);

    // 重构信号：拟合出的稀疏编码可以还原出全部N个位置（含未采样的）的估计值，
    // 这才是压缩感知"补全"的本意
    Eigen::VectorXd reconstructed = dct_dict * sparse_code;
    result.reconstructed_signal.resize(data_size);
    for (int i = 0; i < data_size; ++i) {
        result.reconstructed_signal[i] = reconstructed(i);
    }

    // 精度评估只看"未采样、留出来做验证"的位置 —— 如果把参与拟合的那些点也
    // 算进RMS，误差会被系统性地低估（那些点本来就是拟合目标的一部分）。
    std::vector<bool> isSampled(data_size, false);
    for (int idx : sampled_indices) isSampled[idx] = true;
    std::vector<int> heldOutIndices;
    for (int i = 0; i < data_size; ++i) {
        if (!isSampled[i]) heldOutIndices.push_back(i);
    }
    if (!heldOutIndices.empty()) {
        Eigen::VectorXd orig_held(heldOutIndices.size()), recon_held(heldOutIndices.size());
        for (size_t k = 0; k < heldOutIndices.size(); ++k) {
            orig_held(static_cast<int>(k)) = magnetic_anomalies(heldOutIndices[k]);
            recon_held(static_cast<int>(k)) = reconstructed(heldOutIndices[k]);
        }
        result.rms_error = calculateRms(orig_held, recon_held);
    } else {
        // sampling_factor left nothing held out (e.g. <=1): fall back to the
        // full-signal error since there's no independent validation set.
        result.rms_error = calculateRms(magnetic_anomalies, reconstructed);
    }

    return result;
}

Eigen::MatrixXd CompressiveSensing::createDctDictionary(int size)
{
    Eigen::MatrixXd dct = Eigen::MatrixXd::Zero(size, size);
    double norm_factor = std::sqrt(2.0 / size);

    for (int k = 0; k < size; ++k) {
        for (int n = 0; n < size; ++n) {
            if (k == 0) {
                dct(k, n) = std::sqrt(1.0 / size);
            } else {
                dct(k, n) = norm_factor * std::cos(M_PI * (2 * n + 1) * k / (2.0 * size));
            }
        }
    }
    return dct;
}

std::vector<int> CompressiveSensing::selectSampledIndices(int size, int sampling_factor)
{
    if (sampling_factor <= 0) {
        sampling_factor = 1;  // treat 0/negative as "no downsampling" instead of dividing by zero
    }
    int sampled_size = size / sampling_factor;
    sampled_size = std::max(1, std::min(sampled_size, size));

    std::vector<int> indices(size);
    std::iota(indices.begin(), indices.end(), 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);
    indices.resize(sampled_size);
    std::sort(indices.begin(), indices.end());
    return indices;
}

// CoSaMP-style greedy pursuit, restricted throughout to the actually-measured
// positions.
//
// The previous version zero-filled every unsampled position and ran this
// exact algorithm against that zero-filled full-length vector -- i.e. it
// told the least-squares/residual steps "the true value at every unmeasured
// point is exactly 0", which is the opposite of what reconstruction/gap-
// filling is supposed to do (and biases the result toward zero as
// sampling_factor grows). Standard compressed sensing solves
// y_measured = Phi*x where Phi is the dictionary restricted to the measured
// rows; unmeasured positions never enter the fit at all. That's what
// `dict_measured` (built from `sampled_indices`) implements here -- the
// proxy correlation, the least-squares estimate, and the residual are all
// computed only over the M actually-measured samples, never the full N.
Eigen::VectorXd CompressiveSensing::sparseReconstruction(const Eigen::VectorXd& measured_data,
                                                         const Eigen::MatrixXd& dict,
                                                         const std::vector<int>& sampled_indices,
                                                         int n_nonzero_coefs)
{
    const int N = dict.cols();
    const int M = static_cast<int>(sampled_indices.size());

    Eigen::MatrixXd dict_measured(M, N);
    for (int k = 0; k < M; ++k) {
        dict_measured.row(k) = dict.row(sampled_indices[k]);
    }

    Eigen::VectorXd sparse_code = Eigen::VectorXd::Zero(N);
    Eigen::VectorXd residual = measured_data;
    const double tol = 1e-10;  // 收敛阈值

    // COSAMP 算法推荐使用 2s 大小的支撑集
    const int support_size = 2 * n_nonzero_coefs;

    // 预计算字典的转置，避免重复计算
    Eigen::MatrixXd dict_measured_transpose = dict_measured.transpose();

    // 用于存储支撑集的索引
    std::vector<int> support_set;
    support_set.reserve(support_size);

    // 迭代直到收敛或达到最大迭代次数
    const int max_iter = 50;
    for (int iter = 0; iter < max_iter; ++iter) {
        // 1. Identification: 计算代理残差（只对实际测到的位置）
        Eigen::VectorXd proxy = dict_measured_transpose * residual;

        // 2. 支撑集合并
        std::vector<std::pair<double, int>> correlation_pairs(N);
#pragma omp parallel for
        for (int i = 0; i < N; ++i) {
            correlation_pairs[i] = std::make_pair(std::abs(proxy(i)), i);
        }

        // 部分排序找到最大的 2s 个元素
        std::partial_sort(correlation_pairs.begin(),
                          correlation_pairs.begin() + std::min(support_size, N),
                          correlation_pairs.end(),
                          std::greater<std::pair<double, int>>());

        // 获取新的候选支撑集
        std::vector<int> new_support;
        new_support.reserve(support_size);
        for (int i = 0; i < support_size && i < N; ++i) {
            if (correlation_pairs[i].first > tol) {
                new_support.push_back(correlation_pairs[i].second);
            }
        }

        // 合并当前支撑集和新的候选支撑集
        std::set<int> merged_support(support_set.begin(), support_set.end());
        merged_support.insert(new_support.begin(), new_support.end());

        // 3. Estimation: 在合并后的支撑集上、只用实测位置求解最小二乘问题
        Eigen::MatrixXd dict_subset(M, merged_support.size());
        int col_idx = 0;
        for (int idx : merged_support) {
            dict_subset.col(col_idx++) = dict_measured.col(idx);
        }

        // 使用 QR 分解求解最小二乘问题
        Eigen::VectorXd estimated_coeffs =
            dict_subset.colPivHouseholderQr().solve(measured_data);

        // 4. Pruning: 保留最大的 s 个系数
        std::vector<std::pair<double, int>> coef_pairs(estimated_coeffs.size());
#pragma omp parallel for
        for (int i = 0; i < estimated_coeffs.size(); ++i) {
            coef_pairs[i] = std::make_pair(std::abs(estimated_coeffs(i)), i);
        }

        std::partial_sort(coef_pairs.begin(),
                          coef_pairs.begin() + std::min<size_t>(n_nonzero_coefs, coef_pairs.size()),
                          coef_pairs.end(),
                          std::greater<std::pair<double, int>>());

        // 更新支撑集和稀疏编码
        support_set.clear();
        Eigen::VectorXd new_sparse_code = Eigen::VectorXd::Zero(N);

        int idx = 0;
        for (const auto& pair : coef_pairs) {
            if (idx >= n_nonzero_coefs) break;
            int original_idx = *std::next(merged_support.begin(), pair.second);
            support_set.push_back(original_idx);
            new_sparse_code(original_idx) = estimated_coeffs(pair.second);
            idx++;
        }

        // 5. 更新残差（只对实际测到的位置）
        residual = measured_data - dict_measured * new_sparse_code;

        // 检查收敛条件
        if (residual.norm() < tol ||
            (sparse_code - new_sparse_code).norm() < tol) {
            sparse_code = new_sparse_code;
            break;
        }

        sparse_code = new_sparse_code;
    }

    return sparse_code;
}

double CompressiveSensing::calculateRms(const Eigen::VectorXd& original,
                                        const Eigen::VectorXd& reconstructed)
{
    return std::sqrt((original - reconstructed).array().square().mean());
}
