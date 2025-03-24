#ifndef EIGENQDEBUG_H
#define EIGENQDEBUG_H
#include <QDebug>
#include <eigen-3.4.0/Eigen/Dense>

//便于使用qDebug()<<输出eigen库结果
template <typename Derived>
QDebug operator<<(QDebug debug, const Eigen::MatrixBase<Derived> &matrix){
    debug.nospace() << "[";
    for (int i = 0; i < matrix.rows(); ++i) {
        if (i != 0)
            debug.nospace() << " ";
        for (int j = 0; j < matrix.cols(); ++j) {
            debug.nospace() << matrix(i, j);
            if (j != matrix.cols() - 1)
                debug.nospace() << ", ";
        }
        if (i != matrix.rows() - 1)
            debug.nospace() << ";\n";
    }
    debug.nospace() << "]";
    return debug.space();
};
#endif // EIGENQDEBUG_H
