#include "mapTaylorLegendreform.h"
#include "ui_mapTaylorLegendreform.h"

maptaylorlegendreform::maptaylorlegendreform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::maptaylorlegendreform)
{
    ui->setupUi(this);
    ui->spinBox->setRange(1,100);
}

maptaylorlegendreform::~maptaylorlegendreform()
{
    delete ui;
}

int maptaylorlegendreform::getspinbox()
{
    return ui->spinBox->value();
}
