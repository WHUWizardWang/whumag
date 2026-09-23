#ifndef INPUTPARA1_H
#define INPUTPARA1_H

#endif // INPUTPARA1_H
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDate>
#include <QDateEdit>
#include <QDebug>

int inputPara_up(QDialog &dialog,double &h,bool &useBL,
                  int &data_num,QStringList &data_name_list,int &data_index,QString &dir,
                  double &step_x,double &step_y);

// autoParameter: choose the regularisation parameter at the L-curve corner; otherwise |parameter|
// (alpha, or the number of iterations for the iterative operators)
int inputPara_down(QDialog &dialog,double &h,int &type,bool &useBL,
                    int &data_num,QStringList &data_name_list,int &data_index,QString &dir,
                    double &step_x,double &step_y,bool &autoParameter,double &parameter);

int inputPara_correct(QDialog &dialog,QDate &date0,QDate &date1,int &useGeoid,double &height,
                      int &data_num,QStringList &data_name_list,int &data_index,QString &dir);
