#include "queryfromgridsetform.h"
#include "ui_queryfromgridsetform.h"
using wmm::OmgValidator;

QueryFromGridSetForm::QueryFromGridSetForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QueryFromGridSetForm)
{
    ui->setupUi(this);
    ui->comboBox->addItems(QStringList{"E", "M"});

    // the parameter column is narrow: give "from" / "to" equal room and keep the dates readable
    for (QWidget *w : {static_cast<QWidget *>(ui->lineEdit_lat_min), static_cast<QWidget *>(ui->lineEdit_lat_max),
                       static_cast<QWidget *>(ui->lineEdit_lon_min), static_cast<QWidget *>(ui->lineEdit_lon_max),
                       static_cast<QWidget *>(ui->lineEdit_height_max), static_cast<QWidget *>(ui->dateTimeEdit_min),
                       static_cast<QWidget *>(ui->dateTimeEdit_max)})
        w->setMinimumWidth(108);
    for (QWidget *w : {static_cast<QWidget *>(ui->lineEdit_lat_step), static_cast<QWidget *>(ui->lineEdit_lon_step),
                       static_cast<QWidget *>(ui->lineEdit_height_step), static_cast<QWidget *>(ui->lineEdit_date_step)})
        w->setMinimumWidth(42);
    // the spin buttons reserve 20 px of padding, which cut off the last digit of the date
    for (QDateTimeEdit *e : {ui->dateTimeEdit_min, ui->dateTimeEdit_max})
    {
        e->setButtonSymbols(QAbstractSpinBox::NoButtons);
        e->setStyleSheet(QStringLiteral("QDateTimeEdit { padding-right: 8px; }"));
    }
    for (int c : {2, 3, 5, 6})
        ui->gridLayout->setColumnStretch(c, 1);
    ui->gridLayout->setColumnStretch(8, 0);

    // let the query dialog show how many points the current ranges add up to
    for (QLineEdit *edit : findChildren<QLineEdit *>())
        connect(edit, &QLineEdit::textChanged, this, &QueryFromGridSetForm::changed);

    //    OmgValidator::setValidatorLat(ui->lineEdit_lat_min);
    //    OmgValidator::setValidatorLat(ui->lineEdit_lat_max);
    //    OmgValidator::setValidatorStep(ui->lineEdit_lat_step);

    //    OmgValidator::setValidatorLon(ui->lineEdit_lon_min);
    //    OmgValidator::setValidatorLon(ui->lineEdit_lon_max);
    //    OmgValidator::setValidatorStep(ui->lineEdit_lon_step);

    //    OmgValidator::setValidatorHeight(ui->lineEdit_height_min);
    //    OmgValidator::setValidatorHeight(ui->lineEdit_height_max);
    //    OmgValidator::setValidatorStep(ui->lineEdit_height_step);

    //    OmgValidator::setValidatorStep(ui->lineEdit_date_step);
}

QueryFromGridSetForm::~QueryFromGridSetForm()
{
    delete ui;
}

int QueryFromGridSetForm::getQueryNum()
{
    bool valid(true), ok;
    lat_min = OmgValidator::varifyTextLat(ui->lineEdit_lat_min->text(), &ok);
    valid &= ok;
    lat_max = OmgValidator::varifyTextLat(ui->lineEdit_lat_max->text(), &ok);
    valid &= ok;
    lat_step = OmgValidator::varifyTextStep(ui->lineEdit_lat_step->text(), &ok);
    valid &= ok;
    lon_min = OmgValidator::varifyTextLon(ui->lineEdit_lon_min->text(), &ok);
    valid &= ok;
    lon_max = OmgValidator::varifyTextLon(ui->lineEdit_lon_max->text(), &ok);
    valid &= ok;
    lon_step = OmgValidator::varifyTextStep(ui->lineEdit_lon_step->text(), &ok);
    valid &= ok;
    height_min = OmgValidator::varifyTextHeight(ui->lineEdit_height_min->text(), &ok);
    valid &= ok;
    height_max = OmgValidator::varifyTextHeight(ui->lineEdit_height_max->text(), &ok);
    valid &= ok;
    height_step = OmgValidator::varifyTextStep(ui->lineEdit_height_step->text(), &ok);
    valid &= ok;
    QDate date_t_min = OmgValidator::varifyTextDate(ui->dateTimeEdit_min->text(), &ok);
    valid &= ok;
    QDate date_t_max = OmgValidator::varifyTextDate(ui->dateTimeEdit_max->text(), &ok);
    valid &= ok;
    date_step = OmgValidator::varifyTextStep(ui->lineEdit_date_step->text(), &ok);
    valid &= ok;
    date_min = OmgValidator::dateToDouble(date_t_min, &ok);
    valid &= ok;
    date_max = OmgValidator::dateToDouble(date_t_max, &ok);
    valid &= ok;
    if (!valid)
    {
        return 0;
    }

    if (ui->comboBox->currentText() == "E")
    {
        useGeoid = 0;
    }
    else if (ui->comboBox->currentText() == "M")
    {
        useGeoid = 1;
    }
    else
    {
        useGeoid = -1;
    }

    lat_num = getIntervalNum(lat_min, lat_max, lat_step);
    lon_num = getIntervalNum(lon_min, lon_max, lon_step);
    height_num = getIntervalNum(height_min, height_max, height_step);
    date_num = getIntervalNum(date_min, date_max, date_step);
    total_num = lat_num * lon_num * height_num * date_num;
    return total_num;
}

int QueryFromGridSetForm::getIntervalNum(double beg, double end, double interval)
{
    if (end < beg)
    {
        return 0;
    }
    if (interval <= 0)
    {
        return 0;
    }
    int n = (end - beg) / interval;
    return n;
}

int QueryFromGridSetForm::toCoordGeodeticArray(MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr, int length)
{
    if (length != total_num)
    {
        return -1;
    }

    int index = 0;
    for (int i_lat = 0; i_lat < lat_num; ++i_lat)
    {
        for (int i_lon = 0; i_lon < lon_num; ++i_lon)
        {
            for (int i_hei = 0; i_hei < height_num; ++i_hei)
            {
                for (int i_date = 0; i_date < date_num; ++i_date)
                {
                    MAGtype_CoordGeodetic coord;
                    MAGtype_Date userdate;
                    coord.phi = lat_min + lat_step * i_lat;
                    coord.lambda = lon_min + lon_step * i_lon;
                    coord.UseGeoid = useGeoid;
                    coord.HeightAboveGeoid = height_min + height_step * i_hei;
                    if (!useGeoid)
                    {
                        coord.HeightAboveEllipsoid = coord.HeightAboveGeoid;
                    }
                    userdate.DecimalYear = date_min + date_step * i_date;
                    bool ok;
                    QDate date = OmgValidator::doubleToDate(userdate.DecimalYear, &ok);
                    if (!ok)
                    {
                        return -2;
                    }
                    userdate.Year = date.year();
                    userdate.Month = date.month();
                    userdate.Day = date.day();

                    CoordGeodeticArr[index] = coord;
                    UserDateArr[index] = userdate;
                    ++index;
                }
            }
        }
    }
    return index;
}
