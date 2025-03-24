#!/home/greatwall/mag/bin/python3.8

import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from scipy.interpolate import griddata
import sys
matplotlib.use('TkAgg')  # 或者使用 'TkAgg', 'Qt4Agg' 等

def main(filepath,pngpath, rms=None):
    print(f"Reading data from {filepath}...")  # 添加调试信息
    try:
        data = np.loadtxt(filepath)
        print(f"Data shape: {data.shape}")  # 调试信息
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        sys.exit(1)

    # 假设 data.txt 中的列分别是 X, Y, Z
    X = data[:, 0]
    Y = data[:, 1]
    Z = data[:, 2]

    print("Creating grid...")  # 添加调试信息
    # 创建网格
    xi = np.linspace(X.min(), X.max(), 100)
    yi = np.linspace(Y.min(), Y.max(), 100)
    xi, yi = np.meshgrid(xi, yi)

    print("Interpolating data...")  # 添加调试信息
    # 插值
    zi = griddata((X, Y), Z, (xi, yi), method='cubic')

    print(f"Interpolated data shape: {zi.shape}")  # 调试信息
    if zi.shape != (100, 100):
        print("Error: Interpolated data shape is incorrect.", file=sys.stderr)
        sys.exit(1)

    print("Plotting data...")  # 添加调试信息
    # 绘制等值线图
    plt.figure(facecolor='white')

    # 绘制等值线图，去掉填充颜色，只保留黑色的等值线
    contours = plt.contour(xi, yi, zi, levels=14, colors='black', linewidths=1.5)

    # 显示等值线标签
    plt.clabel(contours, inline=True, fontsize=8, colors='black')

    if rms is not None:
        plt.suptitle(f"RMSE: {rms:.4f}", fontsize=12, y=0.92)

    # 添加标题和标签
    plt.xlabel('X /km')
    plt.ylabel('Y /km')

    # 保存为图片文件
    plt.savefig(pngpath + '/con.png', dpi=300)
    print("Plot saved as con.png")  # 添加调试信息

    # 显示图像
    # plt.show()

if __name__ == '__main__':
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: script.py <file_path> [rms]", file=sys.stderr)
        sys.exit(1)

    filepath = sys.argv[1]
    pngpath = sys.argv[2]
    rms = float(sys.argv[3]) if len(sys.argv) == 4 else None
    main(filepath, pngpath,rms)
