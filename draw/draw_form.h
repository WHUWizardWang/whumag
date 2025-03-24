#ifndef DRAW_FORM_H
#define DRAW_FORM_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QDate>
#include <qdebug.h>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QCoreApplication>
#include <QLabel>
#include <QMessageBox>
#include <QColor>
#include <QProcess>
#include "MagAno/OmgValidator.h"
#include "qcustomplot.h"
#include "contourplotter.h"

namespace Ui {
class draw_Form;
}

class draw_Form : public QWidget
{
    Q_OBJECT

public:
    explicit draw_Form(QWidget *parent = nullptr);
    ~draw_Form();
    void set_HeatOrSactterView(QVector<double> xx,QVector<double> yy,QVector<double> result);
    void set_heatMapView(QVector<double> xx,QVector<double> yy,QVector<double> result);
    void autoset_heatMapView(QVector<double> xx,QVector<double> yy,QVector<double> result);;
    std::pair<double, double> findMinMax(const QVector<QVector<double>>& vec);
    double findDifferenceInArithmeticSequence(const QVector<double>& sequence);
    void create_xyz_p(QVector<AnoPoint> ano_pnts,QVector<double> &xx,QVector<double> &yy,QVector<double> &result);
    void create_xyz_f(QString filename,QVector<double> &xx,QVector<double> &yy,QVector<double> &result);
    void set_ContourView(QString filename);
    void create_contour_txt(QVector<AnoPoint> &ano_pnts);
    int findClosestIndex(const QList<double>& sortedValues, double target);
    QString con_path;

    //格网分辨率
    double x_step;
    double y_step;
    //是否插值
    int type;
    // 是否报错,false为存在错误
    bool magWarn = true;

private slots:
    void on_pushButton_path_clicked();

    void on_pushButton_save_clicked();

private:
    Ui::draw_Form *ui;
};

#endif // DRAW_FORM_H
