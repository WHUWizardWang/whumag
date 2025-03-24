#include "mappolyhedralform.h"
#include "ui_mappolyhedralform.h"

mappolyhedralform::mappolyhedralform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mappolyhedralform)
{
    ui->setupUi(this);
}

mappolyhedralform::~mappolyhedralform()
{
    delete ui;
}

int mappolyhedralform::getModelType()
{
    if (ui->radioButton->isChecked())
        return 0;
    else if (ui->radioButton_2->isChecked())
        return 1;
    else return -1;

}

double mappolyhedralform::getdoubleSpinBoxPara()
{
    return ui->doubleSpinBox->value();
}
