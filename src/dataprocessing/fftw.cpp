//#include "fftw.h"

//FFTW::FFTW()
//{
//}

//FFTW::~FFTW()
//{
//}

//void FFTW::fftshift(fftw_complex* data, int rows, int cols)
//{
//    // 创建一个临时数组来辅助频谱中心化
//    std::vector<fftw_complex> temp(rows * cols);

//    // 频谱中心化处理
//    for (int i = 0; i < rows; ++i) {
//        for (int j = 0; j < cols; ++j) {
//            // 计算中心化后的新位置
//            int new_i = (i + rows / 2) % rows; // 使用模运算实现环形移动
//            int new_j = (j + cols / 2) % cols;

//            // 复制原始数据到临时数组
//            temp[new_i * cols + new_j][0] = data[i * cols + j][0];
//            temp[new_i * cols + new_j][1] = data[i * cols + j][1];
//        }
//    }

//    // 从临时数组复制回中心化后的数据
//    for (int i = 0; i < rows; ++i) {
//        for (int j = 0; j < cols; ++j) {
//            data[i * cols + j][0] = temp[i * cols + j][0];
//            data[i * cols + j][1] = temp[i * cols + j][1];
//        }
//    }
//}

//fftw_complex* FFTW::fft_2d(const MatrixXd& real_input, int& rows, int& cols)
//{
//    fftw_complex* in = fftw_alloc_complex(rows * cols);
//    fftw_complex* out = fftw_alloc_complex(rows * cols);
//    fftw_plan plan = fftw_plan_dft_2d(rows, cols, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
//    for (int i = 0; i < rows; ++i) {
//        for (int j = 0; j < cols; ++j) {
//            in[i * cols + j][0] = real_input(i, j);  // Real part
//            in[i * cols + j][1] = 0.0;               // Imaginary part
//        }
//    }
//    fftw_execute(plan);
//    fftw_destroy_plan(plan);
//    fftw_free(in);
//    return out;
//    fftw_free(out);
//}

//fftw_complex* FFTW::ifft_2d(int& rows,int& cols,fftw_complex* in)
//{
    
//    fftw_complex* out = fftw_alloc_complex(rows * cols);
    
//    // 规划和执行二维傅立叶逆变换
//    fftw_plan plan = fftw_plan_dft_2d(rows, cols, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
//    fftw_execute(plan);

//    // 清理FFTW分配的内存
//    fftw_destroy_plan(plan);
//    fftw_free(in);

//    //执行归一化
//    for (int i = 0; i < rows * cols; i++)
//    {
//        out[i][0] = out[i][0] / rows / cols;
//        out[i][1] = out[i][1] / rows / cols;
//    }

//    return out;
//    fftw_free(out);
//}
