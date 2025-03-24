#include "maplssvmpsoform.h"
#include "ui_maplssvmpsoform.h"

maplssvmpsoform::maplssvmpsoform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::maplssvmpsoform)
{
    ui->setupUi(this);
}

maplssvmpsoform::~maplssvmpsoform()
{
    delete ui;
}

int maplssvmpsoform::getPara(int& p1, double& p2, double& p3, double& p4, double& p5,
                             double& p6, double& p7,int& p8, double& p9, double& p10, double& p11)
{
    p1 = ui->spinBox_nParticleNum->value();
    p2 = ui->doubleSpinBox_dMinSigma->value();
    p3 = ui->doubleSpinBox_dMaxSigma->value();
    p4 = ui->doubleSpinBox_dMinC->value();
    p5 = ui->doubleSpinBox_dMaxC->value();
    p6 = ui->doubleSpinBox_dC1->value();
    p7 = ui->doubleSpinBox_dC2->value();
    p8 = ui->spinBox_nMaxGen->value();
    p9 = ui->doubleSpinBox_dk->value();
    p10 = ui->doubleSpinBox_dWeightV->value();
    p11 = ui->doubleSpinBox_dWeightP->value();
    return 0;
}
