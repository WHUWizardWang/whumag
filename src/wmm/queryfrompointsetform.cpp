#include "queryfrompointsetform.h"
#include "ui_queryfrompointsetform.h"

QueryFromPointSetForm::QueryFromPointSetForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QueryFromPointSetForm)
{
    ui->setupUi(this);
    ui->comboBox->addItems(QStringList{"E", "M"});


    //    OmgValidator::setValidatorLat(ui->lineEdit_lat);
    //    OmgValidator::setValidatorLon(ui->lineEdit_lon);
    //    OmgValidator::setValidatorHeight(ui->lineEdit_height);
}

QueryFromPointSetForm::~QueryFromPointSetForm()
{
    delete ui;
}

MagHeader QueryFromPointSetForm::toParameters()
{
    bool ok;
    MagHeader header;
    header.valid = true;
    header.lat = OmgValidator::varifyTextLat(ui->lineEdit_lat->text(), &ok);
    header.valid &= ok;
    header.lon = OmgValidator::varifyTextLon(ui->lineEdit_lon->text(), &ok);
    header.valid &= ok;
    header.height = OmgValidator::varifyTextHeight(ui->lineEdit_height->text(), &ok);
    header.valid &= ok;
    header.time = OmgValidator::varifyTextDate(ui->dateTimeEdit->text(), &ok);
    header.valid &= ok;
    header.height_class = ui->comboBox->currentText()[0].unicode();
    return header;
}

int QueryFromPointSetForm::toCoordGeodeticArray(MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr, int length)
{
    //    int length = 1;
    if (CoordGeodeticArr == nullptr || UserDateArr == nullptr)
    {
        return -1;
    }
    bool valid = true, ok;

    // Latitude and Longitude
    CoordGeodeticArr->phi = OmgValidator::varifyTextLat(ui->lineEdit_lat->text(), &ok);
    valid &= ok;
    CoordGeodeticArr->lambda = OmgValidator::varifyTextLon(ui->lineEdit_lon->text(), &ok);
    valid &= ok;

    // Altitude
    double height = OmgValidator::varifyTextHeight(ui->lineEdit_height->text(), &ok);
    valid &= ok;
    char height_class = ui->comboBox->currentText()[0].unicode();
    if (height_class == 'E')
    {
        CoordGeodeticArr->HeightAboveEllipsoid = height;
        CoordGeodeticArr->HeightAboveGeoid = CoordGeodeticArr->HeightAboveEllipsoid;
        CoordGeodeticArr->UseGeoid = 0;
    }
    else if (height_class == 'M')
    {
        CoordGeodeticArr->HeightAboveGeoid = height;
        CoordGeodeticArr->UseGeoid = 1;
    }

    // Date
    QDate time = OmgValidator::varifyTextDate(ui->dateTimeEdit->text(), &ok);
    valid &= ok;
    UserDateArr->Year = time.year();
    UserDateArr->Month = time.month();
    UserDateArr->Day = time.day();
    UserDateArr->DecimalYear = time.year() + time.dayOfYear() * 1.0 / time.daysInYear() * 1.0;

    if (!valid)
    {
        return -1;
    }

    return length;
}


