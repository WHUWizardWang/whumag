#ifndef QUERYFROMFILESETFORM_H
#define QUERYFROMFILESETFORM_H

#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include "GeomagnetismHeader.h"

namespace Ui
{
    class QueryFromFileSetForm;
}

class QueryFromFileSetForm : public QWidget
{
    Q_OBJECT

public:
    explicit QueryFromFileSetForm(QWidget *parent = nullptr);
    ~QueryFromFileSetForm();

    int getQueryNum();
    int toCoordGeodeticArray(MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr, int length);
    bool parse(const QString &str, MAGtype_CoordGeodetic *CoordGeodeticArr, MAGtype_Date *UserDateArr);

private slots:
    void on_pushButton_clicked();

private:
    Ui::QueryFromFileSetForm *ui;
};

#endif // QUERYFROMFILESETFORM_H
