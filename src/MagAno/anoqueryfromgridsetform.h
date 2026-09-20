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

    // Grid geometry of the last getQueryNum() call (valid when it returned > 0)
    int latCount() const { return lat_num; }
    int lonCount() const { return lon_num; }
    double latMinimum() const { return lat_min; }
    double lonMinimum() const { return lon_min; }
    double latStep() const { return lat_step; }
    double lonStep() const { return lon_step; }

signals:
    void changed();   // any of the range edits changed

private:
    int getIntervalNum(double beg, double end, double interval);

    Ui::AnoQueryFromGridSetForm *ui;
    double lat_min = 0, lat_max = 0, lat_step = 0;
    double lon_min = 0, lon_max = 0, lon_step = 0;
    int lat_num = 0, lon_num = 0, total_num = 0;
};

#endif // ANOQUERYFROMGRIDSETFORM_H
