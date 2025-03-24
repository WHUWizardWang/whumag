#!/root/mag/bin/python
# -*- coding: utf-8 -*-
import numpy as np
import scipy.fftpack
import scipy.interpolate
from sklearn.linear_model import OrthogonalMatchingPursuit
import matplotlib.pyplot as plt
import sys

def load_data(file_path):
    """
    加载地磁数据文件，假设文件格式为：X Y 磁异常值
    """
    data = np.loadtxt(file_path)
    return data[:, 0], data[:, 1], data[:, 2]

def create_dct_dictionary(size):
    """
    创建离散余弦变换（DCT）字典
    """
    dct_matrix = scipy.fftpack.dct(np.eye(size), norm='ortho')
    return dct_matrix

def sparse_reconstruction(data, dct_dict, n_nonzero_coefs):
    """
    使用OMP算法进行压缩感知稀疏重构
    """
    omp = OrthogonalMatchingPursuit(n_nonzero_coefs=n_nonzero_coefs)
    omp.fit(dct_dict, data)
    coef = omp.coef_
    return coef

def apply_sampling(data, sampling_factor):
    """
    根据采样系数随机采样数据
    """
    sampled_data = np.zeros_like(data)
    sample_indices = np.random.choice(len(data), len(data) // sampling_factor, replace=False)
    sampled_data[sample_indices] = data[sample_indices]
    return sampled_data

def calculate_rms(original, reconstructed):
    """
    计算均方根误差（RMS）
    """
    return np.sqrt(np.mean((original - reconstructed) ** 2))

def plot_contour(x, y, z, rms=None, title='Magnetic Anomalies Contour Plot'):
    """
    绘制等值线图并显示RMS
    """
    # 创建网格数据
    grid_x, grid_y = np.meshgrid(np.linspace(x.min(), x.max(), 100), np.linspace(y.min(), y.max(), 100))
    grid_z = scipy.interpolate.griddata((x, y), z, (grid_x, grid_y), method='cubic')

    plt.figure(facecolor='white')
    contour = plt.contour(grid_x, grid_y, grid_z,levels=14, colors='black', linewidths=1.5)
    plt.clabel(contour, inline=True, fontsize=8, colors='black')
    if rms is not None:
        plt.suptitle(f"RMS Error: {rms:.4f}", fontsize=12, y=0.92)
    plt.title(title)
    plt.xlabel('X')
    plt.ylabel('Y')
    plt.show()

def main(file_path, n_nonzero_coefs, sampling_factor,output_dir,out_filename):
    # 加载数据
    x, y, magnetic_anomalies = load_data(file_path)

    # 应用采样
    sampled_magnetic_anomalies = apply_sampling(magnetic_anomalies, sampling_factor)

    # 创建DCT字典
    dct_dict = create_dct_dictionary(len(magnetic_anomalies))

    # 稀疏重构
    sparse_code = sparse_reconstruction(sampled_magnetic_anomalies, dct_dict, n_nonzero_coefs)

    # 基于DCT字典和稀疏代码的重构信号
    reconstructed_signal = np.dot(dct_dict, sparse_code)

    # 计算RMS
    rms_error = calculate_rms(magnetic_anomalies, reconstructed_signal)

    # 输出结果到文件 result.txt 和 temp.tmp
    result_file_path = f"{output_dir}/{out_filename}"
    temp_file_path = f"{output_dir}/temp.tmp"

    with open(result_file_path, "w") as result_file, open(temp_file_path, "w") as temp_file:
        for i in range(len(x)):
            line = "{} {} {}\n".format(x[i], y[i], reconstructed_signal[i])
            result_file.write(line)
            temp_file.write(line)

    print(f"Results have been written to {result_file_path} and {temp_file_path}")
#    plot_contour(x, y, reconstructed_signal, rms=rms_error)


if __name__ == "__main__":
    # 获取文件路径、非零系数数量和采样系数参数
    if len(sys.argv) != 6:
        print("Usage: python geophysical_reconstruction.py <file_path> <n_nonzero_coefs> <sampling_factor>")
        sys.exit(1)

    file_path = sys.argv[1]
    n_nonzero_coefs = int(sys.argv[2])
    sampling_factor = int(sys.argv[3])
    output_dir = sys.argv[4]
    out_filename = sys.argv[5]
    main(file_path, n_nonzero_coefs, sampling_factor,output_dir,out_filename)
