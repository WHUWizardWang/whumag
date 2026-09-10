#ifndef MAPPOLYHEDRALFORM_H
#define MAPPOLYHEDRALFORM_H

#include <QWidget>

namespace Ui {
class mappolyhedralform;
}

class mappolyhedralform : public QWidget
{
    Q_OBJECT

public:
    explicit mappolyhedralform(QWidget *parent = nullptr);
    ~mappolyhedralform();

    int getModelType();              // 多面函数
    double getdoubleSpinBoxPara();   // 平滑因子

private:
    Ui::mappolyhedralform *ui;
};

#endif // MAPPOLYHEDRALFORM_H
