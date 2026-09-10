#include "mapcompressform.h"
#include "ui_mapcompressform.h"

mapcompressform::mapcompressform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mapcompressform)
{
    ui->setupUi(this);
}

mapcompressform::~mapcompressform()
{
    delete ui;
}

int mapcompressform::getspinBox()
{
    return ui->spinBox->value();
}
