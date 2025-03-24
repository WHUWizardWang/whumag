#ifndef OMGVALIDATOR_H
#define OMGVALIDATOR_H

#include <QLineEdit>
#include <QRegExpValidator>
#include <QDate>

namespace Omg
{
    class OmgValidator;
}

typedef struct
{
    double x, y, z;
} AnoPoint;

class OmgValidator
{
public:
    static void setValidatorLat(QLineEdit *line_edit)
    {
        // -90.0~90.0
        QRegExp rx("^(-?90|[1-8]?\\d(\\.\\d{1,8})?)$");
        QRegExpValidator *pReg = new QRegExpValidator(rx);
        line_edit->setValidator(pReg);
    }

    static void setValidatorLon(QLineEdit *line_edit)
    {
        // -180.0~180.0
        QRegExp rx("^-?180|((((1[0-7])?|\\d)?\\d)(\\.\\d{1,8})?)$");
        QRegExpValidator *pReg = new QRegExpValidator(rx);
        line_edit->setValidator(pReg);
    }

    static void setValidatorHeight(QLineEdit *line_edit)
    {
        // -9999.9999~9999.9999
        QRegExp rx("^(-?[0]|-?[1-9][0-9]{0,4})(?:\\.\\d{1,4})?$");
        QRegExpValidator *pReg = new QRegExpValidator(rx);
        line_edit->setValidator(pReg);
    }

    static void setValidatorStep(QLineEdit *line_edit)
    {
        // 0~9999.9999
        QRegExp rx("^([0]|[1-9][0-9]{0,4})(?:\\.\\d{1,4})?$");
        QRegExpValidator *pReg = new QRegExpValidator(rx);
        line_edit->setValidator(pReg);
    }

    static double varifyTextLat(const QString &str, bool *ok)
    {
        double num_min = -90.0;
        double num_max = 90.0;
        double num = str.toDouble(ok);
        if (!ok)
        {
            return 0.0;
        }
        if (num < num_min || num > num_max)
        {
            *ok = false;
            return 0.0;
        }
        return num;
    }

    static double varifyTextLon(const QString &str, bool *ok)
    {
        double num_min = -180.0;
        double num_max = 180.0;
        double num = str.toDouble(ok);
        if (!ok)
        {
            return 0.0;
        }
        if (num < num_min || num > num_max)
        {
            *ok = false;
            return 0.0;
        }
        return num;
    }

    static double varifyTextHeight(const QString &str, bool *ok)
    {
        double num_min = -9999.9999;
        double num_max = 9999.9999;
        double num = str.toDouble(ok);
        if (!ok)
        {
            return 0.0;
        }
        if (num < num_min || num > num_max)
        {
            *ok = false;
            return 0.0;
        }
        return num;
    }

    static QDate varifyTextDate(const QString &str, bool *ok)
    {
        QDate date_min(1900, 1, 1);
        QDate date_max(2030, 1, 1);
        QDate date = QDate::fromString(str, "yyyy/MM/dd");
        if (!date.isValid())
        {
            *ok = false;
        }
        if (date < date_min || date > date_max)
        {
            *ok = false;
        }
        return date;
    }

    static double varifyTextStep(const QString &str, bool *ok)
    {
        return varifyTextHeight(str, ok);
    }

    static QDate doubleToDate(double date_f, bool *ok)
    {
        int year = (int)date_f;
        QDate date(year, 1, 1);
        int days = (date_f - year * 1.0) * date.daysInYear();
        date = date.addDays(days);
        *ok = date.isValid();
        return date;
    }

    static double dateToDouble(const QDate &date, bool *ok)
    {
        double date_d = date.year() * 1.0 + date.dayOfYear() * 1.0 / date.daysInYear();
        return date_d;
    }
};

typedef struct
{
    char height_class;
    double lat, lon, height;
    QDate time;
    bool valid;
} MagHeader;

#endif // OMGVALIDATOR_H
