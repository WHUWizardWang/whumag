#ifndef AUTOREFERENCEMAP_H
#define AUTOREFERENCEMAP_H

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
#include "dataprocessing/accuracy.h"

namespace Ui {
class AutoReferenceMap;
}

class AutoReferenceMap : public QWidget
{
    Q_OBJECT

public:
    explicit AutoReferenceMap(QWidget *parent = nullptr);
    ~AutoReferenceMap();
    QString projectPath;            // 工程路径
    int data_num = 0;               // 路径下的实测数据
    QStringList nameList_real;      // 所有实测路径
    void updateComboxData();
    int sparse_para = 2;        // 抽稀参数
    double dx = 0.5;               // 经度（X方向分辨率）
    double dy = 0.5;               // 纬度（Y方向分辨率）

private slots:
    void on_pushButton_clicked();                               // 点击处理按钮，进行建模
    void on_comboBox_height_currentIndexChanged(int index);

    void on_pushButton_save_clicked();

    void on_comboBox_height_currentTextChanged(const QString &arg1);

private:
    Ui::AutoReferenceMap *ui;
    bool loadResourceFile();      // 加载资源文件

    int heightIndex = -1;       // 选择高度的索引
    QString saveFilePath;       // 保存路径
    Accuracy accuracy;          // 精度评估类


    double calculateRMS(const Geomagnetic::Datapoint &datapoints,
                        const Geomagnetic::Datapoint &dataresults);
signals:
    void treeUpdated(int flag); // 定义信号
};



#endif // REFERENCEMAP_H
