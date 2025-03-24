#ifndef MERGEFORM_H
#define MERGEFORM_H

#include <QWidget>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QList>
#include <QHeaderView>
#include <QAbstractItemModel>
#include "merge.h"
#include "draw/draw_form.h"

namespace Ui {
class mergeForm;
}

class mergeForm : public QWidget
{
    Q_OBJECT

public:
    explicit mergeForm(QWidget *parent = nullptr);
    void readTable();
    QString filepath;       // 文件路径
    QString filename;       // 文件名
    int filecount;          // 文件数

    rongHe myMerge;
    ~mergeForm();

private slots:
    void on_chooseFiles_clicked();
    void on_confirm_clicked();

signals:
    void textUpdated(const QString &text); // 定义信号
    void treeUpdated(int flag); // 定义信号

private:
    Ui::mergeForm *ui;

};

#endif // MERGEFORM_H
