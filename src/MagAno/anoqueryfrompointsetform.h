#ifndef ANOQUERYFROMPOINTSETFORM_H
#define ANOQUERYFROMPOINTSETFORM_H

#include <QWidget>
#include <QLineEdit>
#include <QVector>
#include "OmgValidator.h"

namespace Ui
{
    class AnoQueryFromPointSetForm;
}

class AnoQueryFromPointSetForm : public QWidget
{
    Q_OBJECT

public:
    explicit AnoQueryFromPointSetForm(QWidget *parent = nullptr);
    ~AnoQueryFromPointSetForm();

    int toCoordGeodeticArray(QVector<AnoPoint> &ano_pnts);

private:

    Ui::AnoQueryFromPointSetForm *ui;
};

#endif // ANOQUERYFROMPOINTSETFORM_H
