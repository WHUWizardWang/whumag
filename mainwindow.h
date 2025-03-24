#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTreeWidgetItem>
#include <QtQuickWidgets/QQuickWidget>
#include <QtConcurrent/QtConcurrent>
#include <chrono>
#include "buildprojectform.h"
#include "importform.h"
#include "wmm/queryform.h"
#include "MagAno/anoqueryform.h"
#include "ReadData.h"
#include "referencemap/GeomagneticModel.h"
#include "referencemap/LSSVMPSO.h"
#include "referencemap/inputpara_rm.h"
#include "draw/draw_form.h"
#include "dataprocessing/inputpara_dp.h"
#include "navigation/iccp.h"
#include "navigation/tercom.h"
#include "navigation/sitan.h"
#include "dataprocessing/extension.h"
#include "dataprocessing/readFile.h"
#include "dataprocessing/fftw.h"
#include "dataprocessing/mergeform.h"
#include "dataprocessing/Time_TongHua.h"
#include "dataprocessing/accuracy.h"
#include "dataprocessing/MagneticComplexityAnalyzer.h"
#include "subarea/subarea.h"
#include "subarea/subareablocks.h"
#include "Map/mapform.h"
#include "MagAno/OmgValidator.h"
#include "help.h"
#include "realtime_redirector.h"
#include "database/database.h"
#include "navigation/inputpathform.h"
#include "referencemap/globalmodel/referencemap.h"
#include "referencemap/globalmodel/autoreferencemap.h"
QT_BEGIN_NAMESPACE
class ImportForm;
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void updateTextBrowser(const QString &text);            // 更新主界面的文本框区域
    ~MainWindow();

private slots:
    void on_action_newproject_triggered();                  // 打开新建工程窗口
    void CreateNewProject(GeoMagnetismProject *gmproj);     // 新建工程
    void on_action_exit_triggered();                        // 退出
    void on_action_openproject_triggered();                 // 打开工程
    void action_history_slot();                             // 载入历史工程
    void on_action_import_triggered();                      // 打开导入窗口
    void on_action_query_triggered();                       // 打开查询窗口
    void on_action_anoquery_triggered();                    // 打开查询窗口
    void On_DouClickedTreeOpen_Slots(QTreeWidgetItem *);    // 展开树节点
    void On_DouClickedTreeClose_Slots(QTreeWidgetItem *);   // 关闭树节点
    void adddata(const QString &tablename,const QString &inputs,double &dx,double &dy);
    double calculateRMS(const Geomagnetic::Datapoint &datapoints, const Geomagnetic::Datapoint &dataresults);

    void on_action_taylor_build_triggered();                // 基准图构建：泰勒多项式
    void on_action_legendre_build_triggered();              // 基准图构建：legendre项式
    void on_action_polyhedral_build_triggered();            // 基准图构建：多面函数
    void on_action_spline_build_triggered();                // 基准图构建：样条函数
    void on_action_compress_build_triggered();              // 基准图构建：压缩感知（python）
    void on_action_lssvmpso_build_triggered();              // 基准图构建：粒子群寻优的最小二乘支持向量机
    void on_action_sparsePara_triggered();                  // 基准图构建：抽稀参数设置
    void on_action_subarea_triggered();                     // 基准图构建：分区
    void on_action_build_triggered();                       // 基准图构建：建模
    void on_action_suball_triggered();                      // 基准图构建：分区+建模
    void on_action_globalmodel_triggered();                 // 整区建模界面
    void on_action_up_triggered();                          // 数据处理：向上延拓
    void on_action_down_triggered();                        // 数据处理：向下延拓
    void on_action_evaluate_triggered();                    // 数据处理：精度评估
    void on_action_merge_triggered();                       // 数据处理：数据融合
    void on_action_correct_triggered();                     // 数据处理：通化改正
    void on_action_navPara_triggered();                     // 匹配导航：参数选择
    void on_actionTERCOM_triggered();                       // 匹配导航：TERCOM
    void on_actionICCP_triggered();                         // 匹配导航：ICCP
    void on_actionSITAN_triggered();                        // 匹配导航：SITAN
    void on_actionjianhexian_triggered();                   // 精度评估：检核线1
    void on_actionjianhexian2_triggered();                  // 精度评估：检核线2
    void on_actionshow_triggered();                         // 显示：热力图和等值线图
    void on_action_3d_triggered();                          // 显示：3D图
    void on_action_about_triggered();                       // 帮助：关于
    void on_action_database_triggered();                    // 数据库界面


    void updateTree(int flag);
    void on_action_readlines_triggered();

    int findIndexByString(const QString &targetString);


    void on_action_TERCOM_ICCP_triggered();

    void on_action_nav_triggered();

    void on_action_manuals_triggered();

    void on_action_shuimian_triggered();

    void on_action_auto_triggered();

    void on_action_kongzhong_triggered();

    void on_action_xishushuixia_triggered();

    void on_action_xishukongzhong_triggered();

private:
    void ProjectChanged();                                  // 更新工程树及历史工程信息
    void ProjectChanged_data();                             // 更新实测数据树的信息
    void ProjectChanged_processed();                        // 更新处理后数据树的信息
    void UpdateLastOpenedPath(const QString &path);         // 更新上次打开的文件夹路径
    void UpdateHistoricalProject();                         // 更新历史工程信息
    void LoadHistoricalProjInfo();                          // 加载历史工程信息
    void SaveHistoricalProjInfo();                          // 保存历史工程信息

    Ui::MainWindow      *ui;                                // 主窗体
    BuildProjectForm    *build_project_form_;               // 新建工程窗口
    ImportForm          *import_form_;                      // （向当前工程）导入数据窗口
    QueryForm           *query_form_;                       // 全球磁场模型查询窗口
    AnoQueryForm        *query_form_ano_;                   // 磁异常查询窗口
    mergeForm           *merge_from_;                       // 数据融合窗口
    database            *database_form;                     // 数据库窗口
    InputPathForm       *navPara_form_;                     // 匹配导航路径选择
    ReferenceMap        *referenceMap_form_;                // 整取建模窗口
    AutoReferenceMap    *autoReferenceMap_form_;            // 自动建模窗口

    GeoMagnetismProject *geomag_proj_;                      // 地磁工程类实例，记录工程各类参数
    QString             last_opened_path_;                  // 上次打开的文件夹路径
    QString             historical_proj_file_path_;         // 记录历史工程路径的文件路径
    QVector<QString>    historical_projects_;               // 历史工程路径数组

    QVector<InputData> inputVector;
    int sparse_para = 2;                                    // 抽稀参数
    Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
    Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
    Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
    Geomagnetic::ReadData readdata;
    Geomagnetic::Datainfo datainfo;
    int data_num = 0;                                       // 当前工程中加载的实测数据数
    subareaBlocks sub;                                      // 分区
    int sub_flag_ = 0;                                      // 是否分区
};
#endif // MAINWINDOW_H
