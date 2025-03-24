#include "anoqueryfrompointsetform.h"
#include "ui_anoqueryfrompointsetform.h"

AnoQueryFromPointSetForm::AnoQueryFromPointSetForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AnoQueryFromPointSetForm)
{
    ui->setupUi(this);
    //    ui->comboBox->addItems(QStringList{"E", "M"});

//        OmgValidator::setValidatorLat(ui->lineEdit_lat);
//        OmgValidator::setValidatorLon(ui->lineEdit_lon);
    //    OmgValidator::setValidatorHeight(ui->lineEdit_height);
}

AnoQueryFromPointSetForm::~AnoQueryFromPointSetForm()
{
    delete ui;
}

MagHeader AnoQueryFromPointSetForm::toParameters()
{
    bool ok;
    MagHeader header;
    header.valid = true;
    header.lat = OmgValidator::varifyTextLat(ui->lineEdit_lat->text(), &ok);
    header.valid &= ok;
    header.lon = OmgValidator::varifyTextLon(ui->lineEdit_lon->text(), &ok);
    header.valid &= ok;
    //    header.height = OmgValidator::varifyTextHeight(ui->lineEdit_height->text(), &ok);
    //    header.valid &= ok;
    //    header.time = OmgValidator::varifyTextDate(ui->dateTimeEdit->text(), &ok);
    //    header.valid &= ok;
    //    header.height_class = ui->comboBox->currentText()[0].unicode();
    return header;
}

int AnoQueryFromPointSetForm::toCoordGeodeticArray(QVector<AnoPoint> &ano_pnts)
{

    //    int length = 1;
    ano_pnts.clear();
    AnoPoint pnt;
    bool valid = true, ok;

    // Latitude and Longitude
    pnt.x = OmgValidator::varifyTextLat(ui->lineEdit_lat->text(), &ok);
    valid &= ok;
    pnt.y = OmgValidator::varifyTextLon(ui->lineEdit_lon->text(), &ok);
    valid &= ok;


    if (!valid)
    {
        return -1;
    }

    ano_pnts.push_back(pnt);

    return 1;
}


