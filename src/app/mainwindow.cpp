#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dataprocessing/lcurveplot.h"
#include "dataquerydialog.h"
#include "explorerpanel.h"
#include "welcomepage.h"
#include "tasklistwidget.h"
#include "thememanager.h"
#include "uiicons.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , build_project_form_(new BuildProjectForm)
    , import_form_(new ImportForm)
    , query_form_(new QueryForm)
    , query_form_ano_(new AnoQueryForm)
    , merge_from_(new mergeForm())
    , database_form(new database())
    , navigation_form_(new NavigationForm())
    , geomag_proj_(new GeoMagnetismProject())
    , referenceMap_form_(new ReferenceMap())
    , autoReferenceMap_form_(new AutoReferenceMap())
    , last_opened_path_(QDir::currentPath())
{
    ui->setupUi(this);
    setWindowState(Qt::WindowMaximized);
    // 两个查询窗体放进同一个"数据查询"窗口；窗口拥有它们
    queryDialog_ = new DataQueryDialog(query_form_, query_form_ano_, this);
    QWidget *widget_map = new MapForm();
    ui->horizontalLayout_2->addWidget(widget_map);

    // Load information of historical projects
    historical_proj_file_path_ = QDir::currentPath() + "/historical_projects.txt";
    LoadHistoricalProjInfo();

    connect(build_project_form_, SIGNAL(Created(GeoMagnetismProject *)),
            this, SLOT(CreateNewProject(GeoMagnetismProject *)));
    connect(import_form_, &ImportForm::inputReceived,
            this, &MainWindow::adddata);

    // 更新text
    connect(merge_from_, &mergeForm::textUpdated,
            this, &MainWindow::updateTextBrowser);
    connect(import_form_, &ImportForm::textUpdated,
            this, &MainWindow::updateTextBrowser);
    // 更新工程树
    connect(referenceMap_form_,&ReferenceMap::treeUpdated,
            this,&MainWindow::updateTree);
    connect(merge_from_,&mergeForm::treeUpdated,
            this,&MainWindow::updateTree);

    // 重设计的界面外壳：导轨、命令栏、工程面板、任务/日志、状态栏
    setupShell();
}

MainWindow::~MainWindow()
{
    closing_ = true;
    // 析构各类子窗体
    delete merge_from_;
    delete build_project_form_;
    delete import_form_;
    delete queryDialog_;   // 同时释放它承载的两个查询窗体
    delete geomag_proj_;
    delete database_form;
    delete navigation_form_;
    delete referenceMap_form_;
    delete autoReferenceMap_form_;
    delete ui;
}

void MainWindow::updateTextBrowser(const QString &text) {
    ui->textBrowser->append(text); // 更新QTextBrowser
}

void MainWindow::updateTree(int flag)
{
    Q_UNUSED(flag)
    ProjectChanged_processed();
    mapBuilt_ = true;
    updatePipelineState();
}

void MainWindow::on_action_newproject_triggered()
{
    build_project_form_->show();
}

void MainWindow::CreateNewProject(GeoMagnetismProject *gmproj){
    ProjectManager::Save(*geomag_proj_);
    geomag_proj_ = gmproj;
    gmproj = nullptr;

    UpdateLastOpenedPath(geomag_proj_->Path());
    ProjectChanged();
}

void MainWindow::on_action_exit_triggered()
{
    // Default to save current project
    ProjectManager::Save(*geomag_proj_);
    close();
}

void MainWindow::on_action_openproject_triggered()
{
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle(QStringLiteral("选择文件"));
    fileDialog->setNameFilter(tr("File(*.proj*)"));
    fileDialog->setViewMode(QFileDialog::Detail);
    fileDialog->setDirectory(last_opened_path_);
    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    if(fileNames.empty()){
        return;
    }

    openProjectPath(fileNames[0]);
}

namespace {
// 工程树节点：图标名保存在 UserRole，主题切换时由 refreshTreeIcons() 重新着色
QTreeWidgetItem *makeTreeItem(const QString &text, const QString &iconName)
{
    auto *item = new QTreeWidgetItem(QStringList{text});
    item->setData(0, Qt::UserRole, iconName);
    item->setIcon(0, UiIcons::icon(iconName, 16));
    return item;
}

void styleCountColumn(QTreeWidgetItem *category, int count)
{
    category->setText(1, count > 0 ? QString::number(count) : QStringLiteral("0"));
    category->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    category->setForeground(1, QBrush(ThemeManager::instance().color("t3")));
}
} // namespace

void MainWindow::ProjectChanged(){
    // Update project tree
    ui->treeWidget->clear();
    ui->treeWidget->setColumnCount(2);
    ui->treeWidget->header()->setStretchLastSection(false);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->treeWidget->header()->resizeSection(1, 44);
    QTreeWidgetItem *topItem = makeTreeItem(geomag_proj_->Name(), "folder");
    QFont bold = topItem->font(0);
    bold.setBold(true);
    topItem->setFont(0, bold);
    ui->treeWidget->addTopLevelItem(topItem);

    // Update children (the order is relied upon by ProjectChanged_data / _processed: index 2 = 实测, 3 = 处理)
    QTreeWidgetItem *global_item = makeTreeItem("全球磁场模型", "globe");
    QTreeWidgetItem *anomily_item = makeTreeItem("磁异常模型", "anomaly");
    QTreeWidgetItem *survey_item = makeTreeItem("实测数据", "ship");
    QTreeWidgetItem *process_item = makeTreeItem("处理数据", "sliders");

    topItem->addChild(global_item);
    topItem->addChild(anomily_item);
    topItem->addChild(survey_item);
    topItem->addChild(process_item);
    for (QTreeWidgetItem *cat : {global_item, anomily_item, survey_item, process_item})
        styleCountColumn(cat, 0);
    topItem->setExpanded(true);
    survey_item->setExpanded(true);
    process_item->setExpanded(true);

    UpdateHistoricalProject();
    refreshShell();
}

void MainWindow::ProjectChanged_data()
{
    QTreeWidgetItem *topItem = ui->treeWidget->topLevelItem(0);
    if (topItem == nullptr)
    {
        return; // No project tree yet (New/Open Project not run) - nothing to update.
    }
    // Update children
    int childCount = topItem->child(2)->childCount();
    for (int i = childCount - 1; i >= 0; --i)
    {
        QTreeWidgetItem *childItem = topItem->child(2)->child(i);
        if (childItem != nullptr)
        {
            delete childItem;
        }
    }
    for(QString str:geomag_proj_->nameList_real)
    {
        topItem->child(2)->addChild(makeTreeItem(str, "file"));
    }
    styleCountColumn(topItem->child(2), geomag_proj_->nameList_real.size());
    data_num = geomag_proj_->nameList_real.size();
    updatePipelineState();
}

void MainWindow::ProjectChanged_processed()
{
    geomag_proj_->nameList_processed.clear();
    QDir dir_processed(geomag_proj_->Path()+"/Processed");
    if (!dir_processed.exists())
    {
        qDebug() << "目录不存在：" << dir_processed.path();
        return;
    }
    // 设置文件过滤器，只匹配.txt和.dat文件
    QStringList filters;
    filters << "*.txt" << "*.dat";
    dir_processed.setNameFilters(filters);
    // 获取所有匹配的文件信息列表
    QFileInfoList list_processed = dir_processed.entryInfoList(QDir::Files);
    // 遍历文件信息列表
    foreach (const QFileInfo &fileInfo, list_processed)
    {
        geomag_proj_->nameList_processed.append(fileInfo.fileName());
    }
    // Update children
    QTreeWidgetItem *topItem = ui->treeWidget->topLevelItem(0);
    if (topItem == nullptr)
    {
        return; // No project tree yet (New/Open Project not run) - nothing to update.
    }
    int childCount = topItem->child(3)->childCount();
    for (int i = childCount - 1; i >= 0; --i)
    {
        QTreeWidgetItem *childItem = topItem->child(3)->child(i);
        if (childItem != nullptr)
        {
            delete childItem;
        }
    }
    for(QString str:geomag_proj_->nameList_processed)
    {
        topItem->child(3)->addChild(makeTreeItem(str, "file"));
    }
    styleCountColumn(topItem->child(3), geomag_proj_->nameList_processed.size());
    updatePipelineState();
}

void MainWindow::UpdateLastOpenedPath(const QString &path)
{
    QFileInfo file_info(path);
    if (file_info.isDir()) {
        last_opened_path_ = path;
    } else if (file_info.isFile()) {
        last_opened_path_ = path.left(path.lastIndexOf('/') + 1);
    }
}

void MainWindow::LoadHistoricalProjInfo(){
    historical_projects_.clear();
    QFile historical_file(historical_proj_file_path_);
    if (historical_file.exists()){
        historical_file.open(QIODevice::ReadOnly);
        QTextStream stream(&historical_file);
        QString line;
        while(!stream.atEnd()){
            line = stream.readLine();
            if(line.length()>0){
                historical_projects_.push_back(line);
            }
        }
    }
    historical_file.close();

    //
    ui->menu_history->clear();
    foreach (QString path, historical_projects_) {
        ui->menu_history->addAction(path, this, SLOT(action_history_slot()));
    }
}

void MainWindow::action_history_slot(){
    QAction *action = (QAction*)sender();
    openProjectPath(action->text());
}

void MainWindow::openProjectPath(const QString &proj_file){
    UpdateLastOpenedPath(proj_file);

    ProjectManager::Save(*geomag_proj_);
    if (!ProjectManager::Load(*geomag_proj_, proj_file)) {
        QMessageBox::warning(this, tr("打开工程"),
                             tr("无法读取工程：\n%1\n\n请确认工程文件完整，且同一目录下有 Measured 和 Processed 文件夹。")
                                 .arg(QDir::toNativeSeparators(proj_file)));
        // Load() clears the file lists first; put the open project's lists back
        if (!geomag_proj_->Name().isEmpty())
            ProjectManager::Load(*geomag_proj_, geomag_proj_->Path() + "/" + geomag_proj_->Name() + ".proj");
        return;
    }
    ProjectChanged();
    ProjectChanged_data();
    ProjectChanged_processed();
}

void MainWindow::UpdateHistoricalProject(){
    QString current_proj = geomag_proj_->Path()+"/"+geomag_proj_->Name()+".proj";
    int index = historical_projects_.indexOf(current_proj);
    if(index != -1){
        historical_projects_.remove(index);
    }
    historical_projects_.push_front(current_proj);

    int max_item_num = 10;
    if (historical_projects_.size()>max_item_num){
        historical_projects_.remove(max_item_num, historical_projects_.size() - max_item_num);
    }

    SaveHistoricalProjInfo();
    LoadHistoricalProjInfo();
}

void MainWindow::SaveHistoricalProjInfo(){
    QFile historical_file(historical_proj_file_path_);
    historical_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream stream(&historical_file);
    foreach(QString path, historical_projects_){
        stream << path << "\r\n";
    }
    historical_file.close();
}

void MainWindow::on_action_import_triggered()
{
    import_form_->projectPath = geomag_proj_->Path();
    import_form_->show();
}

void MainWindow::on_action_query_triggered()
{
    queryDialog_->showPage(DataQueryDialog::GlobalModel);
}

void MainWindow::on_action_anoquery_triggered()
{
    if (DatabaseManager::instance().isOffline())
    {
        QMessageBox::information(this, tr("离线工作"), tr("当前为离线工作模式，全球磁异常查询需要数据库。\n重新启动程序并连接数据库后即可使用。"));
        return;
    }
    queryDialog_->showPage(DataQueryDialog::Anomaly);
}

int MainWindow::findIndexByString(const QString &targetString)
{
    for (int i = 0;i < inputVector.size();++i)
    {
        if (inputVector[i].fileName == targetString)
            return i;
    }
    return -1;
}

void MainWindow::adddata(const QString &tablename,const QString &input,double &dx,double &dy)
{
    data_num = data_num + 1;
    geomag_proj_->nameList_real.push_back(tablename);
    InputData tmp;
    tmp.fileName = tablename;
    tmp.filePath = geomag_proj_->Path()+"/Measured/"+tablename;
    tmp.GridSizeX = dx;
    tmp.GridSizeY = dy;
    // 复制文件到工程路径下
    copyFile(input,geomag_proj_->Path()+"/Measured/"+tablename);
    ProjectChanged_data();
}

void MainWindow::on_action_taylor_build_triggered()
{
    datapoints.clear();
    datapoint_sparse.clear();
    datapoint_result.clear();

    int data_index;
    int order;
    QString filename;
    QDialog dialog(this);
    int ret = inputPara_taylor(dialog,order,filename,
                               data_num,geomag_proj_->nameList_real,data_index,geomag_proj_->Path());
    if (ret == -1)
        return;
    if (data_index < 0 || data_index >= geomag_proj_->nameList_real.size())
    {
        QMessageBox::warning(this, "错误", "请先导入实测数据后再进行该操作!");
        return;
    }
    std::string str = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(str,datapoints);
    datainfo.Cutoff = order;
    Geomagnetic::TaylorModel taylor;
    std::string s = geomag_proj_->Path().toStdString()+"/Processed/"+filename.toStdString();
    datapoint_result = readdata.setDataResult(datapoints,0.5);
    readdata.DataSet(datapoints, datainfo);

    ui->textBrowser->append("正在初始化...");
    QCoreApplication::processEvents();
    taylor.setMinMax(datapoints);
    taylor.Normalize(datapoints);
    taylor.Normalize(datapoint_result);
    taylor.setCenter(datainfo.Cx, datainfo.Cy);
    ui->textBrowser->append("正在计算结果...");
    QCoreApplication::processEvents();
    taylor.applyInterpolation(datapoints, datapoint_result, datainfo.Cutoff);
    taylor.Denormalize(datapoints);
    taylor.Denormalize(datapoint_result);
    ui->textBrowser->append("结果正在保存与输出...");
    QCoreApplication::processEvents();
    readdata.resultOut(datapoint_result, s);
    QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
    ui->textBrowser->append("基于泰勒多项式方法结果计算完成\n");

    ProjectChanged_processed();
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
    draw_form_->set_heatMapView(xx,yy,zz);
    if(draw_form_->magWarn == false)
        return;
    draw_form_->set_ContourView(QString::fromStdString(s));
    draw_form_->show();

}


void MainWindow::on_action_polyhedral_build_triggered()
{
    datapoints.clear();
    datapoint_sparse.clear();
    datapoint_result.clear();

    short type;
    double para;
    int data_index;
    QString filename;
    //
    QDialog dialog(this);
    int ret = inputPara_polyhedral(dialog,type,para,filename,
                                   data_num,geomag_proj_->nameList_real,data_index,geomag_proj_->Path());
    if (ret == -1)
        return;
    if (ret == -2)
    {
        QMessageBox::warning(nullptr, "错误", "未选择函数!");
        return;
    }
    std::string str = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(str,datapoints);
    Geomagnetic::Polyhedral poly;
    std::string s=geomag_proj_->Path().toStdString()+"/Processed/"+filename.toStdString();
    datainfo.PolyQ = type;
    datainfo.sigma2 = para;
//    readdata.interval = sparse_para*10;
//    readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
    datapoint_result = readdata.setDataResult(datapoints,0.5);
    readdata.DataSet(datapoint_sparse, datainfo);
//    for (auto& elem : datapoint_result)
//    {
//        elem.second.tMagnetic = 0;
//    }
    auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    ui->textBrowser->append("正在初始化...");
    QCoreApplication::processEvents();
    poly.init(datainfo, datapoint_sparse);
    ui->textBrowser->append("正在计算Q矩阵...");
    QCoreApplication::processEvents();
    poly.ComputeQ(datainfo, datapoint_sparse);
    ui->textBrowser->append("正在计算X矩阵...");
    QCoreApplication::processEvents();
    poly.ComputeX(datainfo, datapoint_sparse);
    ui->textBrowser->append("正在计算结果...");
    QCoreApplication::processEvents();
    poly.Result(datainfo, datapoint_result,datapoint_sparse);
    auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    std::chrono::duration<double> elapsed = end - start;
    QString outstr = "多面函数处理时间: " + QString::number(elapsed.count()) + "s";
    ui->textBrowser->append("<p style='color:blue;'>"+outstr+"</p>");
    QCoreApplication::processEvents();
    ui->textBrowser->append("结果正在保存与输出...");
    QCoreApplication::processEvents();
    readdata.resultOut(datapoint_result, s);
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
    draw_form_->set_heatMapView(xx,yy,zz);
    draw_form_->set_ContourView(QString::fromStdString(s));
    draw_form_->show();
    //
    QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
    ui->textBrowser->append("基于多面函数方法结果计算完成\n");
    QCoreApplication::processEvents();
    double rms = calculateRMS(datapoints,datapoint_result);
    ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
    //
    ProjectChanged_processed();
}

void MainWindow::on_action_spline_build_triggered()
{
    datapoints.clear();
    datapoint_sparse.clear();
    datapoint_result.clear();

    double curvature;
    int sparsity;
    int data_index;
    QString filename;
    QDialog dialog(this);
    int ret = inputPara_spline(dialog,curvature,sparsity,filename,
                               data_num,geomag_proj_->nameList_real,data_index,geomag_proj_->Path());
    if (ret == -1)
        return;

    std::string str = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(str,datapoints);
    Geomagnetic::Splinecurve spline;
    std::string s=geomag_proj_->Path().toStdString()+"/Processed/"+filename.toStdString();

    datainfo.E= curvature;
    datainfo.C = sparsity;
//    readdata.interval = sparse_para*10;
//    readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
    datapoint_result = readdata.setDataResult(datapoints,0.5); // 格网间隔
    readdata.DataSet(datapoint_sparse, datainfo);
//    for (auto& elem : datapoint_result)
//    {
//        elem.second.tMagnetic = 0;
//    }
    ui->textBrowser->append("正在初始化...");
    QCoreApplication::processEvents();
    spline.init(datainfo, datapoint_sparse);
    ui->textBrowser->append("正在计算X矩阵...");
    QCoreApplication::processEvents();
    spline.ComputeX(datapoint_sparse);
    ui->textBrowser->append("正在计算结果...");
    QCoreApplication::processEvents();
    spline.Result(datapoint_result, datapoint_sparse);
    ui->textBrowser->append("结果正在保存与输出...");
    QCoreApplication::processEvents();
    readdata.resultOut(datapoint_result, s);
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
    draw_form_->set_heatMapView(xx,yy,zz);
    draw_form_->set_ContourView(QString::fromStdString(s));
    draw_form_->show();
    //
    QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
    ui->textBrowser->append("基于样条曲线方法结果计算完成");
    QCoreApplication::processEvents();
    double rms = calculateRMS(datapoints,datapoint_result);
    ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
    //
    ProjectChanged_processed();
}

void MainWindow::on_action_compress_build_triggered()
{
    int data_index;
    QString datafile;
    double n_nonzero_coefs;
    QString filename;
    QDialog dialog(this);
    int ret = inputPara_compress(dialog,n_nonzero_coefs,filename,
                                 data_num,geomag_proj_->nameList_real,data_index,geomag_proj_->Path());
    if (ret == -1)
        return;
    int sampling_factor = 1;
    datafile = geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index];
    QString out_dir = geomag_proj_->Path()+"/Processed";
    QString s = geomag_proj_->Path()+"/Processed/"+filename;
    QString script_path = "/home/greatwall/whumag/cs.py";
    QString pythonPath = "/home/greatwall/mag/bin/python";
    QString command = QString("%1 %2 %3 %4 %5 %6 %7")
                            .arg(pythonPath)
                            .arg(script_path)
                            .arg(datafile)
                            .arg(n_nonzero_coefs)
                            .arg(sampling_factor)
                            .arg(out_dir)
                            .arg(filename);
//    qDebug()<<command;
    //
    // Execute the command
    ui->textBrowser->append("基于压缩感知方法结果计算开始...");
    QCoreApplication::processEvents();
    int result = system(command.toStdString().c_str());
    if (result == 0)
    {
        ui->textBrowser->append("基于压缩感知方法结果计算完成");
        QCoreApplication::processEvents();
        QMessageBox::information(this, "Success", "Geophysical reconstruction completed successfully.");
    }
    else
    {
        QMessageBox::warning(this, "Failure", "Geophysical reconstruction failed.");
    }
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(s,xx,yy,zz);
    draw_form_->set_heatMapView(xx,yy,zz);
    draw_form_->set_ContourView(s);
    draw_form_->show();
    ProjectChanged_processed();
}

void MainWindow::on_action_lssvmpso_build_triggered()
{
    int data_index;
    QDialog dialog(this);
    QString filename;
    int ret = inputPara_lssvmpso(dialog,filename,
                                 data_num,geomag_proj_->nameList_real,data_index,geomag_proj_->Path());
    if (ret == -1)
        return;
    // lssvmpso的输入参数界面要重新设计

    std::string str = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(str,datapoints);
    QString s = geomag_proj_->Path()+"/Processed/"+filename;
    Geomagnetic::LSSVMPSO lssvmpso;
    lssvmpso.setPara();   // 初始化PSO工作状态(vGroup/粒子边界等)，防止run()内RandomlyInitial()访问未初始化数据
//    readdata.interval = sparse_para *10;
    Geomagnetic::Datapoint alldatapoint;
    Geomagnetic::Datapoint Traindatapoint;
    Geomagnetic::Datapoint Testdatapoint;
    readdata.selectLineData(datapoints, Traindatapoint, alldatapoint, readdata.interval); // 10\20 50
//    readdata.setDataResult(datapoints,0.5);
    ui->textBrowser->append("数据加载完毕...");
    QCoreApplication::processEvents();
    ui->textBrowser->append("开始迭代计算...");
    QCoreApplication::processEvents();
    lssvmpso.run(alldatapoint,Traindatapoint,s);
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(s,xx,yy,zz);
    draw_form_->set_heatMapView(xx,yy,zz);
    draw_form_->set_ContourView(s);
    draw_form_->show();
    //
    ui->textBrowser->append("计算完成！");
    QCoreApplication::processEvents();
    //
    ProjectChanged_processed();
}

void MainWindow::on_action_sparsePara_triggered()
{
    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("输入抽稀参数:");
    // Value1
    QString value1 = QString("抽稀参数: ");
    QSpinBox *spinbox1 = new QSpinBox(&dialog);
    form.addRow(value1, spinbox1);
    // Add Cancel and OK button
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        sparse_para = spinbox1->value();
    }
    else return;
}

double MainWindow::calculateRMS(const Geomagnetic::Datapoint &datapoints, const Geomagnetic::Datapoint &dataresults)
{
    double sumSquaredError = 0.0;
    int count = 0;

    for (const auto& [key, datapoint] : datapoints)
    {
        auto it = dataresults.find(key);
        if (it != dataresults.end())
        {
            double error = datapoint.tMagnetic - it->second.tMagnetic;
            sumSquaredError += error * error;
            ++count;
        }
    }

    if (count == 0) return 0.0;  // Avoid division by zero

    return std::sqrt(sumSquaredError / count);
}

// Runs a continuation job in the background and shows the result (before / after, and the L-curve
// for downward continuation).  Shared by 向上延拓, 向下延拓 and 延拓精度评估.
void MainWindow::startContinuation(const Proc::ContinuationJob &job, const QString &title)
{
    const QString inputName = QFileInfo(job.inputFile).fileName();
    ui->textBrowser->append(QStringLiteral("%1开始：%2").arg(title, inputName));
    const QString taskName = QStringLiteral("%1: %2").arg(title, inputName);
    QListWidgetItem *item = new QListWidgetItem(taskName + " （进行中…）");
    taskList->addItem(item);

    auto *watcher = new QFutureWatcher<Proc::ContinuationOutcome>(this);
    connect(watcher, &QFutureWatcher<Proc::ContinuationOutcome>::finished, this, [=]() {
        const Proc::ContinuationOutcome out = watcher->result();
        watcher->deleteLater();
        if (closing_)
            return;
        for (const QString &line : out.log)
            ui->textBrowser->append(QStringLiteral("  ") + line);
        if (!out.ok()) {
            ui->textBrowser->append(QStringLiteral("错误: %1失败：%2").arg(title, out.error));
            item->setText(taskName + " （处理失败）");
            return;
        }
        item->setText(taskName + " （已完成）");
        ui->textBrowser->append(QStringLiteral("%1完成！").arg(title));

        auto *tabs = new QTabWidget;
        tabs->setAttribute(Qt::WA_DeleteOnClose);
        auto addMap = [tabs](const QString &file, const QString &name) {
            auto *page = new QWidget(tabs);
            auto *layout = new QVBoxLayout(page);
            auto *form = new draw_Form(page);
            QVector<double> xx, yy, zz;
            form->create_xyz_f(file, xx, yy, zz);
            if (!xx.isEmpty()) {
                form->autoset_heatMapView(xx, yy, zz);
                if (form->magWarn)
                    form->autoset_contourView(xx, yy, zz);
            }
            layout->addWidget(form);
            tabs->addTab(page, name);
        };
        addMap(job.inputFile, QStringLiteral("延拓前"));
        addMap(job.outputFile, job.kind == Proc::ContinuationKind::RoundTrip ? QStringLiteral("延拓回原高度") : QStringLiteral("延拓后"));
        if (!out.lcurve.isEmpty()) {
            auto *page = new QWidget(tabs);
            auto *layout = new QVBoxLayout(page);
            layout->addWidget(createLCurvePlot(out.lcurve, page));
            tabs->addTab(page, QStringLiteral("L 曲线"));
        }
        tabs->setWindowTitle(title);
        tabs->resize(1100, 700);
        tabs->show();
        ProjectChanged_processed();
    });
    watcher->setFuture(QtConcurrent::run([job]() { return Proc::runContinuation(job); }));
}

// Output path in the project's Processed folder (".txt" added when there is no extension).
QString MainWindow::processedOutputPath(const QString &fileName) const
{
    const QString outDir = geomag_proj_->Path() + "/Processed/";
    QDir().mkpath(outDir);
    QString path = outDir + fileName.trimmed();
    if (QFileInfo(path).suffix().isEmpty())
        path += ".txt";
    return path;
}

void MainWindow::on_action_up_triggered()
{
    double step_x, step_y, h;
    bool useBL = false;
    int data_index;
    QString outName;
    QDialog dialog(this);
    if (inputPara_up(dialog, h, useBL, data_num, geomag_proj_->nameList_real, data_index, outName, step_x, step_y) == -1)
        return;

    Proc::ContinuationJob job;
    job.kind = Proc::ContinuationKind::Upward;
    job.inputFile = geomag_proj_->Path() + "/Measured/" + geomag_proj_->nameList_real[data_index];
    job.outputFile = processedOutputPath(outName);
    job.geographic = useBL;
    job.dx = step_x;
    job.dy = step_y;
    job.height = h;
    startContinuation(job, QStringLiteral("向上延拓"));
}

void MainWindow::on_action_down_triggered()
{
    double step_x, step_y, h, parameter;
    int data_index, type;
    bool useBL = false, autoParameter = true;
    QString outName;
    QDialog dialog(this);
    if (inputPara_down(dialog, h, type, useBL, data_num, geomag_proj_->nameList_real, data_index, outName,
                       step_x, step_y, autoParameter, parameter) == -1)
        return;

    Proc::ContinuationJob job;
    job.kind = Proc::ContinuationKind::Downward;
    job.inputFile = geomag_proj_->Path() + "/Measured/" + geomag_proj_->nameList_real[data_index];
    job.outputFile = processedOutputPath(outName);
    job.geographic = useBL;
    job.dx = step_x;
    job.dy = step_y;
    job.height = h;
    job.downward.method = Proc::DownwardMethod(type);
    job.downward.autoParameter = autoParameter;
    job.downward.parameter = parameter;
    startContinuation(job, QStringLiteral("向下延拓"));
}

void MainWindow::on_action_evaluate_triggered()
{
    double step_x, step_y, h, parameter;
    int data_index, type;
    bool useBL = false, autoParameter = true;
    QString outName;
    QDialog dialog(this);
    if (inputPara_down(dialog, h, type, useBL, data_num, geomag_proj_->nameList_real, data_index, outName,
                       step_x, step_y, autoParameter, parameter) == -1)
        return;

    Proc::ContinuationJob job;
    job.kind = Proc::ContinuationKind::RoundTrip;
    job.inputFile = geomag_proj_->Path() + "/Measured/" + geomag_proj_->nameList_real[data_index];
    job.outputFile = processedOutputPath(outName);
    job.geographic = useBL;
    job.dx = step_x;
    job.dy = step_y;
    job.height = h;
    job.downward.method = Proc::DownwardMethod(type);
    job.downward.autoParameter = autoParameter;
    job.downward.parameter = parameter;
    startContinuation(job, QStringLiteral("延拓精度评估"));
}

void MainWindow::on_action_suball_triggered()
{
    QDialog dialog;
    int data_index;
    QString dir;
    QString filename = geomag_proj_->Path()+"/Processed/"+"sub.txt";
    subareaBlocks sub;
    int ret = sub.inputPara_subarea(dialog,data_num,geomag_proj_->nameList_real,data_index,dir);
    if (ret != 0) return;
    ui->textBrowser->append("分区处理中，请等待...");
    QCoreApplication::processEvents();
    auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    std::string infile = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(infile,datapoints);
    readdata.selectLineData(datapoints, datapoint_sparse, (int)sub.y_step*10);
    sub.subarea(datapoints,datainfo,datapoint_sparse);
    sub.build(datapoints,datainfo,datapoint_sparse,filename);
    auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    ui->textBrowser->append("分区处理完毕!");
    QCoreApplication::processEvents();
    std::chrono::duration<double> elapsed = end - start;
    QString outstr = "分区建模处理时间: " + QString::number(elapsed.count()) + "s";
    ui->textBrowser->append("<p style='color:blue;'>"+outstr+"</p>");
    QCoreApplication::processEvents();
    //
    QTabWidget *tabwidget = new QTabWidget();
    // tab2
    QWidget *tab2 = new QWidget();
    QVBoxLayout *layout2 = new QVBoxLayout();
    QTableWidget *tablewidget = new QTableWidget();
    tablewidget->setRowCount(sub.row_blockCount);
    tablewidget->setColumnCount(sub.col_blockCount);
    tablewidget->verticalHeader()->setVisible(false);
    tablewidget->horizontalHeader()->setVisible(false);
    for(int i=0;i<tablewidget->rowCount();i++)
        tablewidget->setRowHeight(i,200);
    for(int i=0;i<tablewidget->columnCount();i++)
        tablewidget->setColumnWidth(i,200);
    for(int i=0;i<tablewidget->rowCount();i++)
    {
        for(int j=0;j<tablewidget->columnCount();j++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(QString("Block_%1_%2:  %3")
                    .arg(i+1).arg(j+1).arg(sub.subrms[i][j]));
            item->setFlags(item->flags()&~Qt::ItemIsEditable);
            tablewidget->setItem(i,j,item);
        }
    }
    layout2->addWidget(tablewidget);
    tab2 ->setLayout(layout2);
    // tab1
    QWidget *tab1 = new QWidget();
    QVBoxLayout *layout1 = new QVBoxLayout();
    draw_Form *draw_form_1 = new draw_Form;
    QVector<double> xx1,yy1,zz1;
    draw_form_1->create_xyz_p(sub.result,xx1,yy1,zz1);
    draw_form_1->autoset_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->autoset_contourView(xx1,yy1,zz1);
    layout1->addWidget(draw_form_1);
    tab1 ->setLayout(layout1);
    //
    tabwidget->addTab(tab2,"分块RMS结果");
    tabwidget->addTab(tab1,"建模结果");
    tabwidget->setWindowTitle("分区建模");
    tabwidget->resize(1100,700);
    tabwidget->show();
    ProjectChanged_processed();
    //
    ui->textBrowser->append(sub.outstr);
}

void MainWindow::on_action_merge_triggered()
{
    merge_from_->filepath = geomag_proj_->Path();
//    auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    merge_from_->show();
//    auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
//    std::chrono::duration<double> elapsed = end - start;
//    QString outstr = "数据融合 - 页面切换时间: " + QString::number(elapsed.count()) + "s";
//    ui->textBrowser->append(outstr+"\n");

}

void MainWindow::on_action_correct_triggered()
{
    QDate date0, date1;
    int data_index, useGeoid;
    double height;
    QString outName;
    QDialog dialog(this);
    if (inputPara_correct(dialog, date0, date1, useGeoid, height, data_num, geomag_proj_->nameList_real, data_index, outName) == -1)
        return;

    Proc::TimeCorrectionJob job;
    job.inputFile = geomag_proj_->Path() + "/Measured/" + geomag_proj_->nameList_real[data_index];
    job.outputFile = processedOutputPath(outName);
    job.fromDate = date0;
    job.toDate = date1;
    job.aboveGeoid = useGeoid == 1;
    job.heightKm = height;

    const QString title = QStringLiteral("通化");
    const QString inputName = geomag_proj_->nameList_real[data_index];
    ui->textBrowser->append(QStringLiteral("通化开始：%1").arg(inputName));
    const QString taskName = QStringLiteral("通化: %1").arg(inputName);
    QListWidgetItem *item = new QListWidgetItem(taskName + " （进行中…）");
    taskList->addItem(item);

    auto *watcher = new QFutureWatcher<Proc::TimeCorrectionOutcome>(this);
    connect(watcher, &QFutureWatcher<Proc::TimeCorrectionOutcome>::finished, this, [=]() {
        const Proc::TimeCorrectionOutcome out = watcher->result();
        watcher->deleteLater();
        if (closing_)
            return;
        for (const QString &line : out.log)
            ui->textBrowser->append(QStringLiteral("  ") + line);
        if (!out.ok()) {
            ui->textBrowser->append(QStringLiteral("错误: 通化失败：%1").arg(out.error));
            item->setText(taskName + " （处理失败）");
            return;
        }
        item->setText(taskName + " （已完成）");
        ui->textBrowser->append(QStringLiteral("通化完成！"));

        auto *tabs = new QTabWidget;
        tabs->setAttribute(Qt::WA_DeleteOnClose);
        auto addMap = [tabs](const QString &file, const QString &name) {
            auto *page = new QWidget(tabs);
            auto *layout = new QVBoxLayout(page);
            auto *form = new draw_Form(page);
            QVector<double> xx, yy, zz;
            form->create_xyz_f(file, xx, yy, zz);
            if (!xx.isEmpty()) {
                form->autoset_heatMapView(xx, yy, zz);
                if (form->magWarn)
                    form->autoset_contourView(xx, yy, zz);
            }
            layout->addWidget(form);
            tabs->addTab(page, name);
        };
        addMap(job.inputFile, QStringLiteral("通化前"));
        addMap(job.outputFile, QStringLiteral("通化后"));
        tabs->setWindowTitle(title);
        tabs->resize(1100, 700);
        tabs->show();
        ProjectChanged_processed();
    });
    watcher->setFuture(QtConcurrent::run([job]() { return Proc::runTimeCorrection(job); }));
}

void MainWindow::on_actionshow_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("请打开文件"),
                                               QCoreApplication::applicationFilePath(),
                                               "*.*");
    if (filename.isEmpty())
        return;
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(filename,xx,yy,zz);
    draw_form_->autoset_heatMapView(xx,yy,zz);
//    draw_form_->setMapStep(0.0045,0.0045);
    if(draw_form_->magWarn == false)
        return;
    draw_form_->autoset_contourView(xx,yy,zz);
    draw_form_->show();
}

void MainWindow::on_action_subarea_triggered()
{
    QDialog dialog;
    int data_index;
    QString dir;
    int ret = sub.inputPara_subarea(dialog,data_num,geomag_proj_->nameList_real,data_index,dir);
    if(ret!=0)
        return;
    ui->textBrowser->append("分区处理中，请等待...");
    QCoreApplication::processEvents();
    std::string infile = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(infile,datapoints);
    readdata.selectLineData(datapoints, datapoint_sparse, (int)sub.y_step*10);
    sub.subarea(datapoints,datainfo,datapoint_sparse);
    ui->textBrowser->append("分区完毕!");
    QCoreApplication::processEvents();
    sub_flag_ = 1;
}

void MainWindow::on_action_build_triggered()
{
    if (sub_flag_==0)
    {
        QMessageBox msgBox;
        msgBox.setText("如果使用分区建模，请先分区，再建模！");
        msgBox.exec();
        return;
    }
    ui->textBrowser->append("建模处理中，请等待...");
    QCoreApplication::processEvents();
//    QMessageBox msgBox;
//    msgBox.setText("请选择保存路径！");
//    msgBox.exec();
//    QString filename = QFileDialog::getSaveFileName(this, tr("请打开文件"),
//                                                QCoreApplication::applicationFilePath(),
//                                                "*.*");
//    if(filename.isEmpty())
//    {
//        QMessageBox::warning(this,"警告","请选择一个文件");
//        return;
//    }
    QString filename = geomag_proj_->Path()+"/Processed/"+"sub.txt";
    sub.build(datapoints,datainfo,datapoint_sparse,filename);
    ui->textBrowser->append("建模完毕!");
    QCoreApplication::processEvents();
    //
    QTabWidget *tabwidget = new QTabWidget();
    double step1 = 0.5;
    // tab2
    QWidget *tab2 = new QWidget();
    QVBoxLayout *layout2 = new QVBoxLayout();
    QTableWidget *tablewidget = new QTableWidget();
    tablewidget->setRowCount(sub.row_blockCount);
    tablewidget->setColumnCount(sub.col_blockCount);
    tablewidget->verticalHeader()->setVisible(false);
    tablewidget->horizontalHeader()->setVisible(false);
    for(int i=0;i<tablewidget->rowCount();i++)
        tablewidget->setRowHeight(i,200);
    for(int i=0;i<tablewidget->columnCount();i++)
        tablewidget->setColumnWidth(i,200);
    for(int i=0;i<tablewidget->rowCount();i++)
    {
        for(int j=0;j<tablewidget->columnCount();j++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(QString("Block_%1_%2:  %3")
                    .arg(i+1).arg(j+1).arg(sub.subrms[i][j]));
            item->setFlags(item->flags()&~Qt::ItemIsEditable);
            tablewidget->setItem(i,j,item);
        }
    }
    layout2->addWidget(tablewidget);
    tab2 ->setLayout(layout2);
    // tab1
    QWidget *tab1 = new QWidget();
    QVBoxLayout *layout1 = new QVBoxLayout();
    draw_Form *draw_form_1 = new draw_Form;
    QVector<double> xx1,yy1,zz1;
    draw_form_1->create_xyz_p(sub.result,xx1,yy1,zz1);
    draw_form_1->autoset_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->autoset_contourView(xx1,yy1,zz1);
    layout1->addWidget(draw_form_1);
    tab1 ->setLayout(layout1);
    //
    tabwidget->addTab(tab2,"分块RMS结果");
    tabwidget->addTab(tab1,"建模结果");
    tabwidget->setWindowTitle("分区建模");
    tabwidget->resize(1100,700);
    tabwidget->show();
    ProjectChanged_processed();
    //
    ui->textBrowser->append(sub.outstr);
}

void MainWindow::on_actionjianhexian_triggered()
{
    QString backgroundPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择背景场文件"));
    if(backgroundPath.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
        return;
    }
    QString checkPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择检核线文件"));
    if(checkPath.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
        return;
    }
    QTextBrowserRedirector redirector(ui->textBrowser);
    Accuracy myAccuracy;
    myAccuracy.processData(backgroundPath,checkPath);
    evaluated_ = true;
    updatePipelineState();
}

void MainWindow::on_actionjianhexian2_triggered()
{
    QString backgroundPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择背景场文件"));
    if(backgroundPath.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
        return;
    }
    QString checkPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择检核线文件"));
    if(checkPath.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
        return;
    }
    QTextBrowserRedirector redirector(ui->textBrowser);
    Accuracy myAccuracy;
    myAccuracy.processData(backgroundPath,checkPath);
}

void MainWindow::on_action_3d_triggered()
{
    QString runPath = QCoreApplication::applicationDirPath();
    QString imagePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择地磁文件"));
    QString terrainPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择地形文件"));
    if (imagePath.length() == 0 || terrainPath.length()==0)
    {
        QMessageBox::warning(this,"警告","选择文件为空！");
        return;
    }
    QImage image(imagePath);
    QImage terrainImage(terrainPath);
    if (image.isNull() ||terrainImage.isNull())
    {
        QMessageBox::warning(this,"警告","选择图片为空！");
        return;
    }
    if (image.width()!=terrainImage.width() || image.height()!=terrainImage.height())
    {
        QMessageBox::warning(this,"警告","选择图片大小不统一");
        return;
    }
    int ncols = image.width();
    int nrows = image.height();

    QVector<QVector<bool>> mask(nrows);
    QVector<QVector<float>> terrain(nrows);
    QVector<QVector<float>> magano(nrows);
    for (int i = 0; i < nrows; ++i)
    {
        mask[i].resize(ncols);
        terrain[i].resize(ncols);
        magano[i].resize(ncols);
        for (int j = 0; j < ncols; ++j)
        {
            mask[i][j] = true;
            QRgb pixel = image.pixel(j, i);
            QRgb pixel2 = terrainImage.pixel(j, i);
            float z1 = qGray(pixel);
            float z2 = qGray(pixel2);
            terrain[i][j] = z2;
            magano[i][j] = z1;
        }
    }
    ui->widget->setHeight(nrows);
    ui->widget->setWidth(ncols);
    ui->widget->setMask(mask);
    ui->widget->setTerrain(terrain);
    ui->widget->setMagano(magano);
    ui->widget->load();
}

void MainWindow::on_action_about_triggered()
{
    help *help_form_ = new help;
    auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    help_form_->show();
    auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    std::chrono::duration<double> elapsed = end - start;
    QString outstr = "关于 - 页面切换时间: " + QString::number(elapsed.count()) + "s";
    ui->textBrowser->append(outstr+"\n");
}

void MainWindow::on_action_database_triggered()
{
    if (DatabaseManager::instance().isOffline())
    {
        QMessageBox::information(this, tr("离线工作"), tr("当前为离线工作模式，数据库管理需要数据库。\n重新启动程序并连接数据库后即可使用。"));
        return;
    }
    database_form->proPath = geomag_proj_->Path();
    database_form->setWindowState(Qt::WindowMaximized);
//    auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    database_form->show();
//    auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
//    std::chrono::duration<double> elapsed = end - start;
//    QString outstr = "关于 - 页面切换时间: " + QString::number(elapsed.count()) + "s";
//    ui->textBrowser->append(outstr+"\n");
}

void MainWindow::on_action_readlines_triggered()
{
    QFileDialog dialog(nullptr,"选择文件","","All Files(*);;");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QStringList fileNames;
    if (dialog.exec() == QDialog::Accepted)
    {
        fileNames = dialog.selectedFiles();
    }
    else
        return;
    QWidget *widget_map2 = new MapForm();
    QLayout *layout = ui->horizontalLayout_2;
    QLayoutItem *item = layout->itemAt(0);
    if (item->widget())
    {
        widget_map2= item->widget();
    }
    MapForm *widget_map=(MapForm *)widget_map2;

    int ret = readdata.ReadLines(fileNames,geomag_proj_->Path()+"/Measured/z_lines.txt");
    if (ret!=0)
        return;
    int flag =0;
    for(const std::vector<linePoints> &line : readdata.lines)
    {
        VesselPath vp;
        vp.name="123";
        vp.color = "red";
        vp.width = 2;
        int point_index = -1;
        for (const linePoints &p : line)
        {
            point_index++;
            if (point_index%100==0||point_index==line.size())
                vp.pos.push_back((QGeoCoordinate(p.B,p.L)));
        }
        widget_map->test(vp);
        flag++;
        QString str = "加载第 " +QString::number(flag)+"/"+QString::number(readdata.lines.size())+"条测线";
        ui->textBrowser->append(str);
        QCoreApplication::processEvents();
    };
    ProjectChanged_data();
    QMessageBox::warning(this,"提示","导入测线成功！");

}

void MainWindow::on_action_globalmodel_triggered()
{
    referenceMap_form_->data_num = data_num;
    referenceMap_form_->projectPath = geomag_proj_->Path();
    referenceMap_form_->nameList_real = geomag_proj_->nameList_real;
    referenceMap_form_->sparse_para = sparse_para;
    referenceMap_form_->updateComboxData();
    referenceMap_form_->setWindowState(Qt::WindowMaximized);
    referenceMap_form_->show();
}


void MainWindow::on_action_nav_triggered()
{
    navigation_form_->setWindowState(Qt::WindowMaximized);
    navigation_form_->show();
    navigation_form_->raise();
    navigation_form_->activateWindow();
}
QString findManualFile()
{
    // 可能的路径列表，按优先级排序
    QStringList possiblePaths;

    QString appDir = QApplication::applicationDirPath();
    possiblePaths << appDir + "/doc/manuals.pdf"         // 应用程序目录下的doc
                  << appDir + "/../doc/manuals.pdf"       // 上级目录的doc
                  << appDir + "/../share/whumag/doc/manuals.pdf"; // 标准Linux安装路径

    // 检查路径是否存在并返回第一个有效路径
    for (const QString& path : possiblePaths) {
        QFileInfo checkFile(path);
        if (checkFile.exists() && checkFile.isFile()) {
            return path;
        }
    }

    // 如果都不存在，返回默认路径
    return appDir + "/doc/manuals.pdf";
}
void MainWindow::on_action_manuals_triggered()
{
    QString pdfFilePath = findManualFile();
    if(QFile::exists(pdfFilePath))
    {
        QUrl url = QUrl::fromLocalFile(pdfFilePath);
        bool success = QDesktopServices::openUrl(url);
    }
}

void MainWindow::on_action_shuimian_triggered()
{
    // 1. 构造参数对话框
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("设置分析参数"));

    QFormLayout* form = new QFormLayout(&dlg);

    // — 背景场文件 —
    QLineEdit* fileEdit = new QLineEdit(&dlg);
    fileEdit->setReadOnly(true);
    QPushButton* fileBtn = new QPushButton(QStringLiteral("选择文件…"), &dlg);
    QHBoxLayout* fileLay = new QHBoxLayout;
    fileLay->addWidget(fileEdit);
    fileLay->addWidget(fileBtn);
    form->addRow(QStringLiteral("背景场文件："), fileLay);

    // — 格网大小 —
    QDoubleSpinBox* gridSpin = new QDoubleSpinBox(&dlg);
    gridSpin->setRange(0.00001, 1.0);
    gridSpin->setDecimals(4);
    gridSpin->setValue(0.0045);
    form->addRow(QStringLiteral("格网大小 (deg)："), gridSpin);

    // — 子区步长 —
    QSpinBox* jumpSpin = new QSpinBox(&dlg);
    jumpSpin->setRange(1, 1000);
    jumpSpin->setValue(15);
    form->addRow(QStringLiteral("子区步长 (格点)："), jumpSpin);

    // — 保存结果文件 —
    QLineEdit* saveEdit = new QLineEdit(&dlg);
    saveEdit->setReadOnly(true);
    QPushButton* saveBtn = new QPushButton(QStringLiteral("选择路径…"), &dlg);
    QHBoxLayout* saveLay = new QHBoxLayout;
    saveLay->addWidget(saveEdit);
    saveLay->addWidget(saveBtn);
    form->addRow(QStringLiteral("保存结果 (CSV)："), saveLay);

    // 确定／取消按钮
    QDialogButtonBox* box = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(box);

    // 信号连接
    connect(fileBtn, &QPushButton::clicked, this,
            [&dlg, fileEdit]() {
                QString f = QFileDialog::getOpenFileName(
                    &dlg,
                    QStringLiteral("选择背景场文件"),
                    QString(),
                    QStringLiteral("网格文件 (*.txt *.dat *.csv)"));
                if (!f.isEmpty())
                    fileEdit->setText(f);
            });
    connect(saveBtn, &QPushButton::clicked, this,
            [&dlg, saveEdit]() {
                QString s = QFileDialog::getSaveFileName(
                    &dlg,
                    QStringLiteral("保存分析结果"),
                    QString(),
                    QStringLiteral("CSV 文件 (*.csv)"));
                if (!s.isEmpty())
                    saveEdit->setText(s);
            });
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // 弹出对话框
    if (dlg.exec() != QDialog::Accepted
        || fileEdit->text().isEmpty()
        || saveEdit->text().isEmpty())
    {
        QMessageBox::warning(this,
                             QStringLiteral("参数不足"),
                             QStringLiteral("请完整填写所有参数后再运行。"));
        return;
    }

    // 2. 取值
    QString   inputFile = fileEdit->text();
    double    gridSize  = gridSpin->value();
    int       jumpSize  = jumpSpin->value();
    QString   outputCsv = saveEdit->text();

    // 3. 开始处理
    ui->textBrowser->append(QStringLiteral("水面复杂度处理中…"));
    QCoreApplication::processEvents();

    ReadData readdata;
    Datapoint datapoints;
    if (!readdata.readGridFromFile(inputFile.toStdString(), datapoints)) {
        QMessageBox::warning(this,
                             QStringLiteral("读取失败"),
                             QStringLiteral("无法读取文件：%1").arg(inputFile));
        return;
    }

    MagneticComplexityAnalyzer mca;
    ComplexityResult result = mca.analyzeComplexityChunked(datapoints, gridSize);

    // 4. 导出 CSV
    if (!mca.exportToFile(result, outputCsv)) {
        QMessageBox::warning(this,
                             QStringLiteral("导出失败"),
                             QStringLiteral("无法写入文件：%1").arg(outputCsv));
    } else {
        QMessageBox::information(this,
                                 QStringLiteral("导出成功"),
                                 QStringLiteral("分析结果已保存到：%1").arg(outputCsv));
    }

    // 5. 显示热图
    mca.showComplexityMap(result, jumpSize);
    mca.showSpacingMap(result, jumpSize);

    ui->textBrowser->append(QStringLiteral("水面复杂度处理完毕！"));
    QCoreApplication::processEvents();


}


void MainWindow::on_action_auto_triggered()
{
    autoReferenceMap_form_->data_num = data_num;
    autoReferenceMap_form_->projectPath = geomag_proj_->Path();
    autoReferenceMap_form_->nameList_real = geomag_proj_->nameList_real;
    autoReferenceMap_form_->sparse_para = sparse_para;
    autoReferenceMap_form_->updateComboxData();
    autoReferenceMap_form_->setWindowState(Qt::WindowMaximized);
    autoReferenceMap_form_->show();
}


void MainWindow::on_action_kongzhong_triggered()
{
    // 1. 参数对话框
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("设置空中复杂度分析参数"));

    QFormLayout* form = new QFormLayout(&dlg);

    // 背景场文件
    QLineEdit* fileEdit = new QLineEdit(&dlg);
    fileEdit->setReadOnly(true);
    QPushButton* fileBtn = new QPushButton(QStringLiteral("选择文件…"), &dlg);
    QHBoxLayout* fileLay = new QHBoxLayout;
    fileLay->addWidget(fileEdit);
    fileLay->addWidget(fileBtn);
    form->addRow(QStringLiteral("背景场文件："), fileLay);

    // 格网大小
    QDoubleSpinBox* gridSpin = new QDoubleSpinBox(&dlg);
    gridSpin->setRange(0.00001, 1.0);
    gridSpin->setDecimals(6);
    gridSpin->setValue(0.00045);
    form->addRow(QStringLiteral("格网大小 (deg)："), gridSpin);

    // 子区步长
    QSpinBox* jumpSpin = new QSpinBox(&dlg);
    jumpSpin->setRange(1, 10000);
    jumpSpin->setValue(150);
    form->addRow(QStringLiteral("子区步长 (格点)："), jumpSpin);

    // 块大小
    QSpinBox* chunkSpin = new QSpinBox(&dlg);
    chunkSpin->setRange(1, 1000000);
    chunkSpin->setValue(1000);
    form->addRow(QStringLiteral("块大小 (点数)："), chunkSpin);

    // 步长
    QSpinBox* stepSpin = new QSpinBox(&dlg);
    stepSpin->setRange(1, 100);
    stepSpin->setValue(1);
    form->addRow(QStringLiteral("步长："), stepSpin);

    // 保存结果文件
    QLineEdit* saveEdit = new QLineEdit(&dlg);
    saveEdit->setReadOnly(true);
    QPushButton* saveBtn = new QPushButton(QStringLiteral("选择路径…"), &dlg);
    QHBoxLayout* saveLay = new QHBoxLayout;
    saveLay->addWidget(saveEdit);
    saveLay->addWidget(saveBtn);
    form->addRow(QStringLiteral("保存结果 (CSV)："), saveLay);

    // 确定／取消
    QDialogButtonBox* box = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(box);

    // 信号–槽
    connect(fileBtn, &QPushButton::clicked, this, [&]() {
        QString f = QFileDialog::getOpenFileName(&dlg,
                                                 QStringLiteral("选择背景场文件"),
                                                 QString(),
                                                 QStringLiteral("数据文件 (*.txt *.dat *.csv)"));
        if (!f.isEmpty()) fileEdit->setText(f);
    });
    connect(saveBtn, &QPushButton::clicked, this, [&]() {
        QString s = QFileDialog::getSaveFileName(&dlg,
                                                 QStringLiteral("保存分析结果"),
                                                 QString(),
                                                 QStringLiteral("CSV 文件 (*.csv)"));
        if (!s.isEmpty()) saveEdit->setText(s);
    });
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // 执行对话框
    if (dlg.exec() != QDialog::Accepted
        || fileEdit->text().isEmpty()
        || saveEdit->text().isEmpty())
    {
        QMessageBox::warning(this,
                             QStringLiteral("参数不足"),
                             QStringLiteral("请完整填写所有参数后再运行。"));
        return;
    }

    // 2. 读取参数
    QString  inputFile  = fileEdit->text();
    double   gridSize   = gridSpin->value();
    int      jumpSize   = jumpSpin->value();
    int      chunkSize  = chunkSpin->value();
    int      stepSize   = stepSpin->value();
    QString  outputCsv  = saveEdit->text();

    // 3. 执行分析
    ui->textBrowser->append(QStringLiteral("空中复杂度处理中…"));
    QCoreApplication::processEvents();

    ReadData reader;
    Datapoint datapoints;
    if (!reader.readGridFromFile(inputFile.toStdString(), datapoints)
        || datapoints.empty())
    {
        QMessageBox::warning(this,
                             QStringLiteral("读取失败"),
                             QStringLiteral("无法读取文件或文件无效：%1").arg(inputFile));
        return;
    }

    MagneticComplexityAnalyzer mca;
    mca.setSUBAREA_SIZE(jumpSize);
    ComplexityResult result = mca.analyzeComplexityChunked(
        datapoints, gridSize, jumpSize, chunkSize, /*showSpacing=*/true);

    // 4. 导出 CSV
    ui->textBrowser->append(QStringLiteral("数据处理完成，导出结果…"));
    QCoreApplication::processEvents();
    if (!mca.exportToFile(result, outputCsv)) {
        QMessageBox::warning(this,
                             QStringLiteral("导出失败"),
                             QStringLiteral("无法写入文件：%1").arg(outputCsv));
    } else {
        QMessageBox::information(this,
                                 QStringLiteral("导出成功"),
                                 QStringLiteral("分析结果已保存到：%1").arg(outputCsv));
    }

    // 5. 绘制热图
    ui->textBrowser->append(QStringLiteral("生成复杂度热图…"));
    QCoreApplication::processEvents();
    mca.showComplexityMap(result, jumpSize);

    ui->textBrowser->append(QStringLiteral("生成测线间距热图…"));
    QCoreApplication::processEvents();
    mca.showSpacingMap(result, jumpSize);

    ui->textBrowser->append(QStringLiteral("空中复杂度处理完毕！"));
    QCoreApplication::processEvents();

}



void MainWindow::on_action_xishushuixia_triggered()
{

    QString complexityDataFile = QFileDialog::getOpenFileName(this, QStringLiteral("选择检核线文件"));
    if(complexityDataFile.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择复杂度文件");
        return;
    }

    QStringList checkPath = QFileDialog::getOpenFileNames(this, QStringLiteral("选择检核线文件"));
    if(checkPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择检核线文件");
        return;
    }
    // 重定向输出到文本浏览器
    QTextBrowserRedirector redirector(ui->textBrowser);

    // 2. 读取数据
    Accuracy myAccuracy;

//    double totalRMS = myAccuracy.computeCheckLineAccuracy(backgroundPath, checkPath,complexityDataFile,8.5,0.1);
    double totalRMS = myAccuracy.computeSparseCheckLineRMSE(checkPath,complexityDataFile,0.0045);
    ui->textBrowser->append(QString("计算完成，RMS误差 = %1 nT").arg(totalRMS));

}


void MainWindow::on_action_xishukongzhong_triggered()
{
    // 1. 选择文件
    QString backgroundPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择背景场文件"));
    if(backgroundPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择背景场文件");
        return;
    }

    QString checkPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择检核线文件"));
    if(checkPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择检核线文件");
        return;
    }
    QString complexityDataFile = QFileDialog::getOpenFileName(this, QStringLiteral("选择检核线文件"));
    if(complexityDataFile.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择复杂度文件");
        return;
    }

    // 重定向输出到文本浏览器
    QTextBrowserRedirector redirector(ui->textBrowser);

    // 2. 读取数据
    Geomagnetic::Datapoint backgroundData, checkLineData;
    Accuracy myAccuracy;

    double totalRMS = myAccuracy.computeCheckLineAccuracy(backgroundPath, checkPath,complexityDataFile,8.5,0.1,5);

    ui->textBrowser->append(QString("计算完成，RMS误差 = %1 nT").arg(totalRMS));
}


void MainWindow::on_action_3_triggered()
{
    QString backgroundFilePath;
    double lineSpacing;
    double subareaSize;
    QDialog dialog(this);
    dialog.setWindowTitle("参数设置");
    dialog.setMinimumWidth(400);

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // Background field file selection
    QGroupBox* fileGroupBox = new QGroupBox("背景场文件选择", &dialog);
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroupBox);
    QLineEdit* filePathEdit = new QLineEdit(fileGroupBox);
    filePathEdit->setReadOnly(true);
    QPushButton* browseButton = new QPushButton("浏览...", fileGroupBox);
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);

    // Connect browse button
    QObject::connect(browseButton, &QPushButton::clicked, [&filePathEdit, &dialog](){
        QString filePath = QFileDialog::getOpenFileName(&dialog, "选择背景场文件", "",
                                                        "所有文件 (*.*)");
        if (!filePath.isEmpty()) {
            filePathEdit->setText(filePath);
        }
    });

    // Line spacing parameter
    QGroupBox* spacingGroupBox = new QGroupBox("格网间距", &dialog);
    QHBoxLayout* spacingLayout = new QHBoxLayout(spacingGroupBox);
    QDoubleSpinBox* spacingSpinBox = new QDoubleSpinBox(spacingGroupBox);
    spacingSpinBox->setRange(0,1);
    spacingSpinBox->setDecimals(5);
    spacingSpinBox->setSingleStep(0.00001);
    spacingSpinBox->setValue(0.0045);
    spacingLayout->addWidget(spacingSpinBox);

    // Subarea size parameter
    QGroupBox* subareaGroupBox = new QGroupBox("子区大小", &dialog);
    QHBoxLayout* subareaLayout = new QHBoxLayout(subareaGroupBox);
    QDoubleSpinBox* subareaSpinBox = new QDoubleSpinBox(subareaGroupBox);
    subareaSpinBox->setRange(1.0, 10000.0);
    subareaSpinBox->setValue(15.0);
    subareaLayout->addWidget(subareaSpinBox);

    // Add dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Add all widgets to main layout
    mainLayout->addWidget(fileGroupBox);
    mainLayout->addWidget(spacingGroupBox);
    mainLayout->addWidget(subareaGroupBox);
    mainLayout->addWidget(buttonBox);

    // Execute dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Process the input data
        backgroundFilePath = filePathEdit->text();
        lineSpacing = spacingSpinBox->value();
        subareaSize = subareaSpinBox->value();
    }
    else {
        return;
    }

    MagneticComplexityAnalyzer mca;
    GridData gridData;
    ReadData readdata;
    Datainfo datainfo;
    Datapoint data;
    Datapoint datapoints;//原始数据
    Datapoint datapoints1;//格网化后数据
    readdata.readGridFromFile(backgroundFilePath.toStdString(),datapoints);
    readdata.createGridData(datapoints,datapoints1,lineSpacing,lineSpacing);
//    OptimizedCubicInterpolator interpolator;
    std::vector<double> x, y, z;
    std::vector<double> xx,yy;
    for(auto &point : datapoints) {
        x.push_back(point.second.X);
        y.push_back(point.second.Y);
        z.push_back(point.second.tMagnetic);
    }
    for(auto &point : datapoints1) {
        xx.push_back(point.second.X);
        yy.push_back(point.second.Y);
    }
    CubicInterpolator2D interpolator(x,y,z,200);
    std::vector<double> zz = interpolator.interpolate(xx,yy);
    datapoints1.clear();
    //将xx,yy,zz存入datapoints1
    for(int i=0;i<xx.size();i++)
    {
        SinglePoint point;
        point.X = xx[i];
        point.lon = xx[i];
        point.Y = yy[i];
        point.lat = yy[i];
        point.tMagnetic = zz[i];
        datapoints1.insert(std::make_pair(i,point));
    }
    readdata.resultOut(datapoints1,"1111.txt");
    double gridSize = lineSpacing;
    int jumpSize = subareaSize;
    ui->textBrowser->append("复杂度处理中...");
    QCoreApplication::processEvents();
    ComplexityResult result = mca.analyzeComplexityChunked(datapoints1,gridSize,jumpSize);
    QFileDialog dialog_save(this);
    dialog_save.setAcceptMode(QFileDialog::AcceptSave);
    dialog_save.setDefaultSuffix("csv");
    QString filename = dialog_save.getSaveFileName(this, tr("保存分析结果"), "", tr("CSV文件 (*.csv)"));

    if (!filename.isEmpty()) {
        if (mca.exportToFile(result, filename)) {
            QMessageBox::information(this, tr("导出成功"), tr("分析结果已保存到 %1").arg(filename));
        } else {
            QMessageBox::warning(this, tr("导出失败"), tr("无法写入文件 %1").arg(filename));
        }
    }

    // 显示复杂度热图
    mca.showComplexityMap(result,jumpSize);

    // 显示测线间距热图
    // mca->showSpacingMap(result,jumpSize);

    ui->textBrowser->append("复杂度处理完毕!");
    QCoreApplication::processEvents();

}

