#ifndef QUERYFROMPOINTSETFORM_H
#define QUERYFROMPOINTSETFORM_H

#include <QWidget>
#include <QLineEdit>
#include <MagAno/OmgValidator.h>
#include "GeomagnetismHeader.h"

namespace Ui
{
    class QueryFromPointSetForm;
}

class QueryFromPointSetForm : public QWidget
{
    Q_OBJECT

public:
    explicit QueryFromPointSetForm(QWidget *parent = nullptr);
    ~QueryFromPointSetForm();

    MagHeader toParameters();
    int toCoordGeodeticArray(MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr, int length);

private:

    Ui::QueryFromPointSetForm *ui;
};

#endif // QUERYFROMPOINTSETFORM_H
