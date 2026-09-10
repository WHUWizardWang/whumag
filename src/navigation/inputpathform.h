#ifndef INPUTPATHFORM_H
#define INPUTPATHFORM_H
#pragma once

#include <QWidget>


namespace Ui {
class InputPathForm;

}

namespace Geomagnetic {
class AUTONAV;
class SitanMatching;
}

class InputPathForm : public QWidget
{
    Q_OBJECT

public:
    explicit InputPathForm(QWidget *parent = nullptr);
    ~InputPathForm();
    QString backGFile;      // 背景图文件路径
    QString INSFile;        // ins航迹文件路径
    QString realFile;       // 真实航迹文件路径
    QString outputPath;     // 输出文件路径
    int check();
    void navTERCOM();
    void navICCP();
    void navSITAN();
    void navTERCOM_ICCP();
    void navAUTO();

private slots:
    void on_pushButton_backG_clicked();     // 选择 - 背景图
    void on_pushButton_INS_clicked();       // 选择 - ins航迹
    void on_pushButton_real_clicked();      // 选择 - 真实航迹
    void on_pushButton_out_clicked();    // 选择 - 输出文件
    void on_pushButton_confirm_clicked();   // 确定按钮
    void on_pushButton_cancel_clicked();    // 取消按钮
    void on_comboBox_currentIndexChanged(int index);



private:
    Ui::InputPathForm *ui;
    Geomagnetic::AUTONAV* anav;
    Geomagnetic::SitanMatching* st;
};

#endif // INPUTPATHFORM_H
