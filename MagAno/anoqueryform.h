#ifndef ANOQUERYFORM_H
#define ANOQUERYFORM_H

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
#include "anoqueryfrompointsetform.h"
#include "anoqueryfromgridsetform.h"
#include "maganoquery.h"
#include "draw/draw_form.h"


namespace Ui
{
    class AnoQueryForm;
}

class AnoQueryForm : public QWidget
{
    Q_OBJECT

public:
    explicit AnoQueryForm(QWidget *parent = nullptr);
    ~AnoQueryForm();

private slots:
    void on_radioButton_point_toggled(bool checked);

    void on_radioButton_file_toggled(bool checked);

    void on_radioButton_grid_toggled(bool checked);

    void on_pushButton_3_clicked();

    void on_pushButton_export_clicked();

private:
    void init_comboBox_error();
    void init_tableView();

    void fill_item_model(QVector<AnoPoint> &ano_pnts);


    Ui::AnoQueryForm *ui;
    AnoQueryFromPointSetForm *point_form_;
    AnoQueryFromGridSetForm *grid_form_;

    QStandardItemModel *table_model_;
};

#endif // ANOQUERYFORM_H
