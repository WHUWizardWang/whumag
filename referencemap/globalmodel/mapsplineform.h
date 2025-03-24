#ifndef MAPSPLINEFORM_H
#define MAPSPLINEFORM_H

#include <QWidget>

namespace Ui {
class mapsplineform;
}

class mapsplineform : public QWidget
{
    Q_OBJECT

public:
    explicit mapsplineform(QWidget *parent = nullptr);
    ~mapsplineform();

    double getdoubleSpinBox();  // 曲率
    int getspinBox();           // 弹性稀疏度

private:
    Ui::mapsplineform *ui;
};

#endif // MAPSPLINEFORM_H
