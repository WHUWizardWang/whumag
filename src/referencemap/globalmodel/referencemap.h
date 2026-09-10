#ifndef REFERENCEMAP_H
#define REFERENCEMAP_H

#include <QWidget>
#include "mapTaylorLegendreform.h"
#include "mappolyhedralform.h"
#include "mapsplineform.h"
#include "mapcompressform.h"
#include "maplssvmpsoform.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QButtonGroup>
#include "referencemap/ReconstructionManager.h"
#include "ReadData.h"
#include "referencemap/GeomagneticModel.h"
#include "referencemap/LSSVMPSO.h"
#include "referencemap/inputpara_rm.h"
#include "draw/draw_form.h"

namespace Ui {
class ReferenceMap;
}

class ReferenceMap : public QWidget
{
    Q_OBJECT

public:
    explicit ReferenceMap(QWidget *parent = nullptr);
    ~ReferenceMap();
    QString projectPath;            // 工程路径
    int data_num = 0;               // 路径下的实测数据
    QStringList nameList_real;      // 所有实测路径
    void updateComboxData();
    int sparse_para = 2;        // 抽稀参数
    double dx = 0.5;               // 经度（X方向分辨率）
    double dy = 0.5;               // 纬度（Y方向分辨率）

private slots:
    void on_comboBox_model_currentIndexChanged(int index);      // 选择combobox更改对应界面
    void on_pushButton_clicked();                               // 点击处理按钮，进行建模
    void on_radioButtonGroup_toggled(int id);

    void on_pushButton_2_clicked();

private:
    Ui::ReferenceMap *ui;
    maptaylorlegendreform *taylorlegendre_form_;      // 参数界面:taylor和legendre
    mappolyhedralform *polyhedral_form_;              // 参数界面:polyhedral
    mapsplineform *spline_form_;                      // 参数界面:spline
    mapcompressform *compress_form_;                  // 参数界面:compress
    maplssvmpsoform *lssvmpso_form_;                  // 参数界面：lssvmpso
    void setFormsEditable(bool editable);             // 设置参数界面是否可编辑


    int modelIndex = -1;        // 选择建模方法的索引


signals:
    void treeUpdated(int flag); // 定义信号
};



#endif // REFERENCEMAP_H
