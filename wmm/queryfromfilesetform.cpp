#include "queryfromfilesetform.h"
#include "ui_queryfromfilesetform.h"
#include "OmgValidator.h"

QueryFromFileSetForm::QueryFromFileSetForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QueryFromFileSetForm)
{
    ui->setupUi(this);
}

QueryFromFileSetForm::~QueryFromFileSetForm()
{
    delete ui;
}

int QueryFromFileSetForm::getQueryNum()
{
    QFile file(ui->lineEdit->text());
    if (!file.open(QIODevice::ReadOnly))
    {
        return -1;
    }

    int length = 0;
    MAGtype_CoordGeodetic CoordGeodetic;
    MAGtype_Date UserDate;
    QTextStream stream(&file);
    while (stream.atEnd() == false)
    {
        QString line = stream.readLine();

        bool valid = parse(line, &CoordGeodetic, &UserDate);
        if (valid)
        {
            ++length;
        }
    }
    file.close();
    return length;
}

int QueryFromFileSetForm::toCoordGeodeticArray(MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr, int length)
{
    QFile file(ui->lineEdit->text());
    if (!file.open(QIODevice::ReadOnly))
    {
        return -1;
    }

    MAGtype_CoordGeodetic coord_tmp;
    MAGtype_Date userdate_tmp;
    int idx = 0;
    QTextStream stream(&file);
    while (stream.atEnd() == false)
    {
        QString line = stream.readLine();
        bool valid = parse(line, &coord_tmp, &userdate_tmp);

        if (valid)
        {
            CoordGeodeticArr[idx] = coord_tmp;
            UserDateArr[idx] = userdate_tmp;
            ++idx;
        }
    }
    file.close();
    return idx;
}

bool QueryFromFileSetForm::parse(const QString &str, MAGtype_CoordGeodetic *coord, MAGtype_Date *userdate)
{
    QStringList str_arr = str.split(' ', QString::SkipEmptyParts);
    if (str_arr.length() != 5)
    {
        return false;
    }

    bool valid(true), ok;

    // Date
    double date_f = str_arr[0].toDouble(&ok);
    if (!ok)
    {
        return false;
    }
    QDate date = OmgValidator::doubleToDate(date_f, &ok);
    userdate->Year = date.year();
    userdate->Month = date.month();
    userdate->Day = date.day();
    userdate->DecimalYear = date.year() * 1.0 + date.dayOfYear() * 1.0 / date.daysInYear();
    OmgValidator::varifyTextDate(date.toString("yyyy/MM/dd"), &ok);
    valid &= ok;

    // Height class
    if (str_arr[1] == "E")
    {
        coord->UseGeoid = 0;
    }
    else if (str_arr[1] == "M")
    {
        coord->UseGeoid = 1;
    }
    else
    {
        return false;
    }
    // Height
    double height = str_arr[2].right(str_arr[2].length() - 1).toDouble(&ok);
    if (!ok)
    {
        return false;
    }
    if (str_arr[2].at(0) == 'M')
    {
        height *= 1000.0;
    }
    else if (str_arr[2].at(0) == 'F')
    {
        height /= 3280.0839895;
    }
    else if (str_arr[2].at(0) != 'K')
    {
        return false;
    }
    coord->HeightAboveGeoid = OmgValidator::varifyTextHeight(QString::number(height), &ok);
    if (!coord->UseGeoid)
    {
        coord->HeightAboveEllipsoid = coord->HeightAboveGeoid;
    }
    valid &= ok;

    // Latitude
    coord->phi = OmgValidator::varifyTextLat(str_arr[3], &ok);
    valid &= ok;
    // Longitude
    coord->lambda = OmgValidator::varifyTextLon(str_arr[4], &ok);
    valid &= ok;

    return valid;
}

void QueryFromFileSetForm::on_pushButton_clicked()
{
    QString runPath = QCoreApplication::applicationDirPath();
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"), runPath, "Text Files(*.txt)");
    ui->lineEdit->setText(file);
}
