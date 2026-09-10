// CompressiveSensing.cpp
#include "CompressiveSensing.h"
#include <QDebug>

CompressiveSensing::ReconstructionResult
CompressiveSensing::reconstruct(const Geomagnetic::Datapoint& data,
                                int n_nonzero_coefs,
                                int sampling_factor)
{
    ReconstructionResult result;
    int data_size = data.size();

    // 提取数据
    Eigen::VectorXd magnetic_anomalies(data_size);
    result.x.reserve(data_size);
    result.y.reserve(data_size);

    int i = 0;
    for (const auto& [_, point] : data) {
        magnetic_anomalies(i++) = point.tMagnetic;
        result.x.push_back(point.X);
        result.y.push_back(point.Y);
    }

    // 创建DCT字典
    Eigen::MatrixXd dct_dict = createDctDictionary(data_size);

    // 应用采样
    Eigen::VectorXd sampled_data = applySampling(magnetic_anomalies, sampling_factor);

    // 稀疏重构
    Eigen::VectorXd sparse_code = sparseReconstruction(sampled_data, dct_dict, n_nonzero_coefs);

    // 重构信号
    Eigen::VectorXd reconstructed = dct_dict * sparse_code;
    result.reconstructed_signal.resize(data_size);
    for (int i = 0; i < data_size; ++i) {
        result.reconstructed_signal[i] = reconstructed(i);
    }

    // 计算RMS误差
    result.rms_error = calculateRms(magnetic_anomalies, reconstructed);

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

Eigen::VectorXd CompressiveSensing::applySampling(const Eigen::VectorXd& data,
                                                  int sampling_factor)
{
    int size = data.size();
    if (sampling_factor <= 0) {
        sampling_factor = 1;  // treat 0/negative as "no downsampling" instead of dividing by zero
    }
    int sampled_size = size / sampling_factor;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, size - 1);

    Eigen::VectorXd sampled_data = Eigen::VectorXd::Zero(size);
    std::vector<bool> used(size, false);

    for (int i = 0; i < sampled_size; ++i) {
        int idx;
        do {
            idx = dis(gen);
        } while (used[idx]);

        used[idx] = true;
        sampled_data(idx) = data(idx);
    }

    return sampled_data;
}

Eigen::VectorXd CompressiveSensing::sparseReconstruction(const Eigen::VectorXd& data,
                                                         const Eigen::MatrixXd& dict,
                                                         int n_nonzero_coefs)
{
    const int N = dict.cols();
    const int M = dict.rows();
    Eigen::VectorXd sparse_code = Eigen::VectorXd::Zero(N);
    Eigen::VectorXd residual = data;
    const double tol = 1e-10;  // 收敛阈值

    // COSAMP 算法推荐使用 2s 大小的支撑集
    const int support_size = 2 * n_nonzero_coefs;

    // 预计算字典的转置，避免重复计算
    Eigen::MatrixXd dict_transpose = dict.transpose();

    // 用于存储支撑集的索引
    std::vector<int> support_set;
    support_set.reserve(support_size);

    // 迭代直到收敛或达到最大迭代次数
    const int max_iter = 50;
    for (int iter = 0; iter < max_iter; ++iter) {
        // 1. Identification: 计算代理残差
        Eigen::VectorXd proxy = dict_transpose * residual;

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

        // 3. Estimation: 在合并后的支撑集上求解最小二乘问题
        Eigen::MatrixXd dict_subset(M, merged_support.size());
        int col_idx = 0;
        for (int idx : merged_support) {
            dict_subset.col(col_idx++) = dict.col(idx);
        }

        // 使用 QR 分解求解最小二乘问题
        Eigen::VectorXd estimated_coeffs =
            dict_subset.colPivHouseholderQr().solve(data);

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

        // 5. 更新残差
        residual = data - dict * new_sparse_code;

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
