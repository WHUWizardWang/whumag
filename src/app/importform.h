#ifndef IMPORTFORM_H
#define IMPORTFORM_H

#include <QWidget>
#include <QFileDialog>
#include "DataStruct.h"
#include "ReadData.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

namespace Ui {
class ImportForm;
}

class ImportForm : public QWidget
{
    Q_OBJECT

public:
    explicit ImportForm(QWidget *parent = nullptr);
    ~ImportForm();
    QString projectPath;
    QString txtPath;

signals:
    void inputReceived(const QString &tablename,const QString &input,double &dx,double &dy);
    void textUpdated(const QString &text); // 定义信号

private slots:
    void on_pushButton_choose_clicked();
    void on_pushButton_confirm_clicked();

public slots:
    void dataInput(const QString &tablename,const QString &input,double &dx,double &dy);

private:
    Ui::ImportForm *ui;
    bool appendTextToFile(const QString &textToAdd);
    void openData(QString filePath,double &xmin,double &xmax,double &ymin,double &ymax);


};

struct InputData{
    QString filePath;
    QString fileName;
    double GridSizeX;
    double GridSizeY;
};

#endif // IMPORTFORM_H
