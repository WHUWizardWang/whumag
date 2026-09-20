#ifndef QUERYFROMGRIDSETFORM_H
#define QUERYFROMGRIDSETFORM_H

#include <QWidget>
#include "OmgValidator.h"
#include "GeomagnetismHeader.h"

namespace Ui
{
    class QueryFromGridSetForm;
}

class QueryFromGridSetForm : public QWidget
{
    Q_OBJECT

public:
    explicit QueryFromGridSetForm(QWidget *parent = nullptr);
    ~QueryFromGridSetForm();
    int getQueryNum();
    // Used after getQueryNum().
    int toCoordGeodeticArray(MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr, int length);

signals:
    void changed();   // any of the range / step / date edits changed

private:
    int getIntervalNum(double beg, double end, double interval);

    Ui::QueryFromGridSetForm *ui;
    double lat_min, lat_max, lat_step;
    double lon_min, lon_max, lon_step;
    double height_min, height_max, height_step;
    double date_min, date_max, date_step;
    int lat_num, lon_num, height_num, date_num, total_num;
    int useGeoid;
};

#endif // QUERYFROMGRIDSETFORM_H
