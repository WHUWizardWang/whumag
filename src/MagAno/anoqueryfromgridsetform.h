#ifndef ANOQUERYFROMGRIDSETFORM_H
#define ANOQUERYFROMGRIDSETFORM_H

#include <QWidget>
#include "OmgValidator.h"

namespace Ui
{
    class AnoQueryFromGridSetForm;
}

class AnoQueryFromGridSetForm : public QWidget
{
    Q_OBJECT

public:
    explicit AnoQueryFromGridSetForm(QWidget *parent = nullptr);
    ~AnoQueryFromGridSetForm();
    int getQueryNum(double lon_step,double lat_step);
    // Used after getQueryNum().
    int toCoordGeodeticArray(QVector<AnoPoint> &ano_pnts);

private:
    int getIntervalNum(double beg, double end, double interval);

    Ui::AnoQueryFromGridSetForm *ui;
    double lat_min, lat_max, lat_step;
    double lon_min, lon_max, lon_step;
    int lat_num, lon_num, height_num, date_num, total_num;
    int useGeoid;
};

#endif // ANOQUERYFROMGRIDSETFORM_H
