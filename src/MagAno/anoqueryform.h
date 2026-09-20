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

class Banner;
class Chip;
class QCPColorMap;
class QCPColorScale;
class QCustomPlot;
class QPushButton;
class QTimer;
class StatCard;

class AnoQueryForm : public QWidget
{
    Q_OBJECT

public:
    explicit AnoQueryForm(QWidget *parent = nullptr);
    ~AnoQueryForm();

#ifdef WHUMAG_UI_TEST
    Q_INVOKABLE void testFillSynthetic();
#endif

private slots:
    void on_radioButton_point_toggled(bool checked);

    void on_radioButton_file_toggled(bool checked);

    void on_radioButton_grid_toggled(bool checked);

    void on_pushButton_3_clicked();

    void on_pushButton_export_clicked();

private:
    void init_comboBox_error();
    void init_tableView();

    void fill_item_model(QVector<AnoPoint> &ano_pnts, const QVector<char> &matched);

    // ---- result views (layout built in the constructor)
    void buildResultViews(QVBoxLayout *middle, QVBoxLayout *leftLayout, int leftInsertIndex);
    void updateCountBanner();
    void applyPlotTheme();
    void showResults(int ret, qint64 elapsedMs, bool gridMode);
    void updatePlot(bool gridMode);
    void openDetailWindow();
    static void stepsForProduct(const QString &product, double *lonStep, double *latStep);

    Ui::AnoQueryForm *ui;
    AnoQueryFromPointSetForm *point_form_;
    AnoQueryFromGridSetForm *grid_form_;

    QStandardItemModel *table_model_;

    Chip *statusChip_ = nullptr;
    Banner *resultBanner_ = nullptr;
    Banner *countBanner_ = nullptr;
    QWidget *statsHost_ = nullptr;
    StatCard *statMin_ = nullptr;
    StatCard *statMax_ = nullptr;
    StatCard *statValid_ = nullptr;
    StatCard *statTime_ = nullptr;
    QFrame *chartCard_ = nullptr;
    QCustomPlot *plot_ = nullptr;
    QCPColorMap *heat_ = nullptr;
    QCPColorScale *scale_ = nullptr;
    QLabel *chartInfo_ = nullptr;
    QTimer *countTimer_ = nullptr;

    // last query (used by the chart and the "在新窗口查看" button)
    QVector<AnoPoint> lastPoints_;
    QVector<char> lastMatched_;
    int lastLatCount_ = 0;
    int lastLonCount_ = 0;
    double lastLatMin_ = 0, lastLonMin_ = 0, lastLatStep_ = 0, lastLonStep_ = 0;
};

#endif // ANOQUERYFORM_H
