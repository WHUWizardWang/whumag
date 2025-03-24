#include "mapsplineform.h"
#include "ui_mapsplineform.h"

mapsplineform::mapsplineform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mapsplineform)
{
    ui->setupUi(this);
}

mapsplineform::~mapsplineform()
{
    delete ui;
}

double mapsplineform::getdoubleSpinBox()
{
    return ui->doubleSpinBox->value();
}

int mapsplineform::getspinBox()
{
    return ui->spinBox->value();
}
