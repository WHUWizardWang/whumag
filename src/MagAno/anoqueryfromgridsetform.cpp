#include "anoqueryfromgridsetform.h"
#include "ui_anoqueryfromgridsetform.h"

AnoQueryFromGridSetForm::AnoQueryFromGridSetForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AnoQueryFromGridSetForm)
{
    ui->setupUi(this);
    ui->label_15->setVisible(false);
    ui->label_11->setVisible(false);
    ui->label_12->setVisible(false);
    ui->lineEdit_lat_step->setVisible(false);
    ui->lineEdit_lon_step->setVisible(false);
    //    ui->comboBox->addItems(QStringList{"E", "M"});

    OmgValidator::setValidatorLat(ui->lineEdit_lat_min);
    OmgValidator::setValidatorLat(ui->lineEdit_lat_max);
    //    OmgValidator::setValidatorStep(ui->lineEdit_lat_step);
    OmgValidator::setValidatorLon(ui->lineEdit_lon_min);
    OmgValidator::setValidatorLon(ui->lineEdit_lon_max);


//    //    OmgValidator::setValidatorStep(ui->lineEdit_lon_step);

    //    OmgValidator::setValidatorHeight(ui->lineEdit_height_min);
    //    OmgValidator::setValidatorHeight(ui->lineEdit_height_max);
    //    OmgValidator::setValidatorStep(ui->lineEdit_height_step);

    //    OmgValidator::setValidatorStep(ui->lineEdit_date_step);
}

AnoQueryFromGridSetForm::~AnoQueryFromGridSetForm()
{
    delete ui;
}

int AnoQueryFromGridSetForm::getQueryNum(double step_lon,double step_lat)
{
    bool valid(true), ok;
    lat_min = OmgValidator::varifyTextLat(ui->lineEdit_lat_min->text(), &ok);
    valid &= ok;
    lat_max = OmgValidator::varifyTextLat(ui->lineEdit_lat_max->text(), &ok);
    valid &= ok;
    lat_step = step_lat;
    lon_min = OmgValidator::varifyTextLon(ui->lineEdit_lon_min->text(), &ok);
    valid &= ok;
    lon_max = OmgValidator::varifyTextLon(ui->lineEdit_lon_max->text(), &ok);
    valid &= ok;
    lon_step = step_lon;

    if (!valid)
    {
        return 0;
    }

    //    if (ui->comboBox->currentText() == "E")
    //    {
    //        useGeoid = 0;
    //    }
    //    else if (ui->comboBox->currentText() == "M")
    //    {
    //        useGeoid = 1;
    //    }
    //    else
    //    {
    //        useGeoid = -1;
    //    }

    lat_num = getIntervalNum(lat_min, lat_max, lat_step);
    lon_num = getIntervalNum(lon_min, lon_max, lon_step);

    total_num = lat_num * lon_num;
    return total_num;
}

int AnoQueryFromGridSetForm::getIntervalNum(double beg, double end, double interval)
{
    if (end < beg)
    {
        return 0;
    }
    if (interval < 0)
    {
        return 0;
    }
    int n = (end - beg) / interval;
    return n;
}

int AnoQueryFromGridSetForm::toCoordGeodeticArray(QVector<AnoPoint> &ano_pnts)
{
    ano_pnts.clear();

    for (int i_lat = 0; i_lat < lat_num; ++i_lat)
    {
        for (int i_lon = 0; i_lon < lon_num; ++i_lon)
        {
            AnoPoint pnt;
            pnt.x = lat_min + lat_step * i_lat;
            pnt.y = lon_min + lon_step * i_lon;
            ano_pnts.push_back(pnt);
        }
    }
    return ano_pnts.size();
}
