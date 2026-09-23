#ifndef MERGEFORM_H
#define MERGEFORM_H

#include <QCheckBox>
#include <QWidget>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QList>
#include <QHeaderView>
#include <QAbstractItemModel>
#include "fusion.h"
#include "draw/draw_form.h"

namespace Ui {
class mergeForm;
}

class mergeForm : public QWidget
{
    Q_OBJECT

public:
    explicit mergeForm(QWidget *parent = nullptr);
    ~mergeForm();

    QString filepath;   // project directory: a bare output file name is saved to its Processed folder

private slots:
    void on_chooseFiles_clicked();
    void on_confirm_clicked();

    void on_pushButton_clicked();

signals:
    void textUpdated(const QString &text); // 定义信号
    void treeUpdated(int flag); // 定义信号

private:
    void buildLayout();
    bool readJob(Proc::FusionJob &job);
    void showResult(const Proc::FusionJob &job);

    QCheckBox *removeBias_ = nullptr;
    QCheckBox *rejectOutliers_ = nullptr;
    bool running_ = false;

    Ui::mergeForm *ui;

};

#endif // MERGEFORM_H
