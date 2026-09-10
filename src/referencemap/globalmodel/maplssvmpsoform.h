#ifndef MAPLSSVMPSOFORM_H
#define MAPLSSVMPSOFORM_H

#include <QWidget>

namespace Ui {
class maplssvmpsoform;
}

class maplssvmpsoform : public QWidget
{
    Q_OBJECT

public:
    explicit maplssvmpsoform(QWidget *parent = nullptr);
    ~maplssvmpsoform();
    int getPara(int& p1, double& p2, double& p3, double& p4, double& p5,
                double& p6, double& p7,int& p8, double& p9, double& p10, double& p11);

private:
    Ui::maplssvmpsoform *ui;
};

#endif // MAPLSSVMPSOFORM_H
