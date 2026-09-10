#ifndef INPUTPARA2_H
#define INPUTPARA2_H

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
int checkAndSaveFile(const QString &folderPath,const QString &fileName);

int inputPara_taylor(QDialog &dialog,int &order,QString &filename,
                     int &data_num,QStringList &data_name_list,int &data_index,QString dir);

int inputPara_polyhedral(QDialog &dialog,short &type,double &para,QString &filename,
                         int &data_num,QStringList &data_name_list,int &data_index,QString dir);

int inputPara_spline(QDialog &dialog,double &c,int &s,QString &filename,
                     int &data_num,QStringList &data_name_list,int &data_index,QString dir);

int inputPara_compress(QDialog &dialog,double &n_nonzero_coefs,QString &filename,
                       int &data_num,QStringList &data_name_list,int &data_index,QString dir);

int inputPara_lssvmpso(QDialog &dialog,QString &filename,
                       int &data_num,QStringList &data_name_list,int &data_index,QString dir);

#endif // INPUTPARA_H
