#ifndef QUERYFORM_H
#define QUERYFORM_H

#include <QWidget>
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QFrame>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QFileDialog>
#include <QVector>
#include "queryfrompointsetform.h"
#include "queryfromfilesetform.h"
#include "queryfromgridsetform.h"
#include "wmm_point.h"
#include "igrf_point.h"

namespace Ui
{
    class QueryForm;
}

class Banner;
class Chip;
class QTimer;
class StatCard;

class QueryForm : public QWidget
{
    Q_OBJECT

public:
    explicit QueryForm(QWidget *parent = nullptr);
    ~QueryForm();

private slots:
    void on_radioButton_point_toggled(bool checked);

    void on_radioButton_file_toggled(bool checked);

    void on_radioButton_grid_toggled(bool checked);

    void on_pushButton_3_clicked();

    void on_pushButton_export_clicked();

private:
    void init_comboBox_wmm();
    void init_tableView();

    void fill_item_model(MAGtype_CoordGeodetic *CoordGeodeticArr,
                         MAGtype_Date *UserDateArr,
                         MAGtype_GeoMagneticElements *GeoMagneticElementsArr,
                         int length);

    // ---- result views (layout built in the constructor)
    void buildResultViews(QVBoxLayout *middle, QVBoxLayout *leftLayout, int leftInsertIndex);
    void updateCountBanner();
    void showFailure(const QString &chipText, const QString &title, const QString &body);
    void showSummary(const MAGtype_GeoMagneticElements *elements, int length, bool pointMode, qint64 elapsedMs);

    Ui::QueryForm *ui;
    QueryFromPointSetForm *point_form_;
    QueryFromFileSetForm *file_form_;
    QueryFromGridSetForm *grid_form_;
    QStandardItemModel *table_model_;

    Chip *statusChip_ = nullptr;
    Banner *resultBanner_ = nullptr;
    Banner *countBanner_ = nullptr;
    QWidget *summaryHost_ = nullptr;
    QVector<StatCard *> cards_;
    QTimer *countTimer_ = nullptr;
};

#endif // QUERYFORM_H
