#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , build_project_form_(new BuildProjectForm)
    , import_form_(new ImportForm)
    , query_form_(new QueryForm)
    , query_form_ano_(new AnoQueryForm)
    , merge_from_(new mergeForm())
    , database_form(new database())
    , navPara_form_(new InputPathForm())
    , geomag_proj_(new GeoMagnetismProject())
    , referenceMap_form_(new ReferenceMap())
    , autoReferenceMap_form_(new AutoReferenceMap())
    , last_opened_path_(QDir::currentPath())
{
    ui->setupUi(this);
    setWindowState(Qt::WindowMaximized);
    QWidget *widget_map = new MapForm();
    ui->horizontalLayout_2->addWidget(widget_map);

    // Load information of historical projects
    historical_proj_file_path_ = QDir::currentPath() + "/historical_projects.txt";
    LoadHistoricalProjInfo();

    connect(build_project_form_, SIGNAL(Created(GeoMagnetismProject *)),
            this, SLOT(CreateNewProject(GeoMagnetismProject *)));
    connect(ui->treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(On_DouClickedTreeOpen_Slots(QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(On_DouClickedTreeClose_Slots(QTreeWidgetItem*)));
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
}

MainWindow::~MainWindow()
{
    // 析构各类子窗体
    delete merge_from_;
    delete build_project_form_;
    delete import_form_;
    delete query_form_;
    delete query_form_ano_;
    delete geomag_proj_;
    delete ui;
}

void MainWindow::updateTextBrowser(const QString &text) {
    ui->textBrowser->append(text); // 更新QTextBrowser
}

void MainWindow::updateTree(int flag)
{
    ProjectChanged_processed();
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

    QString proj_file = fileNames[0];

    UpdateLastOpenedPath(proj_file);

    ProjectManager::Save(*geomag_proj_);
    ProjectManager::Load(*geomag_proj_, proj_file);
    ProjectChanged();
    ProjectChanged_data();
    ProjectChanged_processed();
}

void MainWindow::ProjectChanged(){
    // Update project tree
    ui->treeWidget->clear();
    ui->treeWidget->setColumnCount(1);
    ui->treeWidget->addTopLevelItem(new QTreeWidgetItem(QList<QString>{geomag_proj_->Name()}));

    QTreeWidgetItem *topItem = ui->treeWidget->topLevelItem(0);
    topItem->setIcon(0, QIcon(":/icons/proj_close.svg"));

    // Update children
    QTreeWidgetItem *global_item = new QTreeWidgetItem(QStringList{"全球磁场模型"});
    global_item->setIcon(0, QIcon(":/icons/item_close_2.svg"));
    QTreeWidgetItem *anomily_item = new QTreeWidgetItem(QStringList{"磁异常模型"});
    anomily_item->setIcon(0, QIcon(":/icons/item_close_1.svg"));
    QTreeWidgetItem *survey_item = new QTreeWidgetItem(QStringList{"实测数据"});
    survey_item->setIcon(0, QIcon(":/icons/item_close_3.svg"));
    QTreeWidgetItem *process_item = new QTreeWidgetItem(QStringList{"处理数据"});
    process_item->setIcon(0, QIcon(":/icons/item_close_4.svg"));

    topItem->addChild(global_item);
    topItem->addChild(anomily_item);
    topItem->addChild(survey_item);
    topItem->addChild(process_item);

    UpdateHistoricalProject();
}

void MainWindow::ProjectChanged_data()
{
    // Update children
    int childCount = ui->treeWidget->topLevelItem(0)->child(2)->childCount();
    for (int i = childCount - 1; i >= 0; --i)
    {
        QTreeWidgetItem *childItem = ui->treeWidget->topLevelItem(0)->child(2)->child(i);
        if (childItem != nullptr)
        {
            delete childItem;
        }
    }
    for(QString str:geomag_proj_->nameList_real)
    {
        QTreeWidgetItem *data_item = new QTreeWidgetItem(QStringList{str});
        data_item->setIcon(0, QIcon(":/icons/real_file.svg"));
        ui->treeWidget->topLevelItem(0)->child(2)->addChild(data_item);
    }
    data_num = geomag_proj_->nameList_real.size();
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
    int childCount = ui->treeWidget->topLevelItem(0)->child(3)->childCount();
    for (int i = childCount - 1; i >= 0; --i)
    {
        QTreeWidgetItem *childItem = ui->treeWidget->topLevelItem(0)->child(3)->child(i);
        if (childItem != nullptr)
        {
            delete childItem;
        }
    }
    for(QString str:geomag_proj_->nameList_processed)
    {
        QTreeWidgetItem *data_item = new QTreeWidgetItem(QStringList{str});
        data_item->setIcon(0, QIcon(":/icons/process_file.svg"));
        ui->treeWidget->topLevelItem(0)->child(3)->addChild(data_item);
    }
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

    QString proj_file = action->text();
    UpdateLastOpenedPath(proj_file);

    ProjectManager::Save(*geomag_proj_);
    ProjectManager::Load(*geomag_proj_, proj_file);
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

void MainWindow::On_DouClickedTreeOpen_Slots(QTreeWidgetItem *item){
    if(item->text(0) == geomag_proj_->Name()){
        item->setIcon(0, QIcon(":/icons/proj_open.svg"));
    } else if(item->text(0) == "全球磁场模型"){
        item->setIcon(0, QIcon(":/icons/item_open_2.svg"));
    } else if(item->text(0) == "磁异常模型") {
        item->setIcon(0, QIcon(":/icons/item_open_1.svg"));
    } else if(item->text(0) == "实测数据") {
        item->setIcon(0, QIcon(":/icons/item_open_3.svg"));
    } else if(item->text(0) == "处理数据") {
        item->setIcon(0, QIcon(":/icons/item_open_4.svg"));
    }
}

void MainWindow::On_DouClickedTreeClose_Slots(QTreeWidgetItem *item){
    if(item->text(0) == geomag_proj_->Name()){
        item->setIcon(0, QIcon(":/icons/proj_close.svg"));
    } else if(item->text(0) == "全球磁场模型"){
        item->setIcon(0, QIcon(":/icons/item_close_2.svg"));
    } else if(item->text(0) == "磁异常模型") {
        item->setIcon(0, QIcon(":/icons/item_close_1.svg"));
    } else if(item->text(0) == "实测数据") {
        item->setIcon(0, QIcon(":/icons/item_close_3.svg"));
    } else if(item->text(0) == "处理数据") {
        item->setIcon(0, QIcon(":/icons/item_close_4.svg"));
    }
}

void MainWindow::on_action_import_triggered()
{
    import_form_->projectPath = geomag_proj_->Path();
    import_form_->show();
}

void MainWindow::on_action_query_triggered()
{
    query_form_->show();
}

void MainWindow::on_action_anoquery_triggered()
{
    query_form_ano_->show();
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
    std::string str = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(str,datapoints);
    datainfo.Cutoff = order;
//    readdata.interval = sparse_para*10;
    Geomagnetic::TaylorModel taylor;
    std::string s = geomag_proj_->Path().toStdString()+"/Processed/"+filename.toStdString();
//    readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
    datapoint_result = readdata.setDataResult(datapoints,0.5);
    readdata.DataSet(datapoint_sparse, datainfo);
//    for (auto& elem : datapoint_result)
//    {
//        elem.second.tMagnetic = 0;
//    }
    ui->textBrowser->append("正在初始化...");
    QCoreApplication::processEvents();
    taylor.init(datainfo);
    ui->textBrowser->append("正在计算M矩阵...");
    QCoreApplication::processEvents();
    taylor.CalculateM(datapoint_sparse);
    ui->textBrowser->append("正在计算AQ矩阵...");
    QCoreApplication::processEvents();
    taylor.CalculateAQ(datapoint_sparse);
    ui->textBrowser->append("正在计算结果...");
    QCoreApplication::processEvents();
    taylor.Result(datapoint_result);
    ui->textBrowser->append("结果正在保存与输出...");
    QCoreApplication::processEvents();
    readdata.resultOut(datapoint_result, s);
    QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
    double rms = calculateRMS(datapoints,datapoint_result);
    ui->textBrowser->append("基于泰勒多项式方法结果计算完成\n");
    ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
    //
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

void MainWindow::on_action_legendre_build_triggered()
{
    datapoints.clear();
    datapoint_sparse.clear();
    datapoint_result.clear();

    int order;
    int data_index;
    QString filename;
    //
    QDialog dialog(this);
    int ret = inputPara_legendre(dialog,order,filename,
                                 data_num,geomag_proj_->nameList_real,data_index,geomag_proj_->Path());
    if (ret == -1)
        return;
    std::string str = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
    readdata.readGridFromFile(str,datapoints);
    Geomagnetic::LegendreModel legendre;
    std::string s = geomag_proj_->Path().toStdString()+"/Processed/"+filename.toStdString();
    datainfo.N = order;
//    readdata.interval = sparse_para*10;
//    readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
    datapoint_result = readdata.setDataResult(datapoints,0.5);
    readdata.DataSet(datapoint_sparse, datainfo);
//    for (auto& elem : datapoint_result)
//    {
//        elem.second.tMagnetic = 0;
//    }
    ui->textBrowser->append("正在初始化...");
    QCoreApplication::processEvents();
    legendre.init(datainfo,datapoint_sparse);
    ui->textBrowser->append("正在进行归一化处理...");
    QCoreApplication::processEvents();
    legendre.NormalizedCalculation(datapoint_sparse,datapoint_result);
    ui->textBrowser->append("正在计算勒让德矩阵...");
    QCoreApplication::processEvents();
    legendre.ComputeLegendreMatrix(datainfo, datapoint_sparse);
    ui->textBrowser->append("正在计算结果...");
    QCoreApplication::processEvents();
    legendre.Result(datainfo, datapoint_result);
    ui->textBrowser->append("结果正在保存与输出...");
    QCoreApplication::processEvents();
    readdata.resultOut(datapoint_result, s);
    QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
    ui->textBrowser->append("基于勒让德多项式方法结果计算完成\n");
    QCoreApplication::processEvents();
    double rms = calculateRMS(datapoints,datapoint_result);
    ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
    //
    draw_Form *draw_form_ = new draw_Form;
    QVector<double> xx,yy,zz;
    draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
    draw_form_->set_heatMapView(xx,yy,zz);
    draw_form_->set_ContourView(QString::fromStdString(s));
    draw_form_->show();
    //
    ProjectChanged_processed();
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

void MainWindow::on_action_up_triggered()
{
    double step_x,step_y,h;

    int data_index;
    QString dir;
    QDialog dialog(this);
    int ret = inputPara_up(dialog,h,data_num,geomag_proj_->nameList_real,data_index,dir);
    if (ret == -1)
        return;
    //调用类
    readFile readfile;
    FFTW fftw;
    yanTuo yantuo;
    std::string infile = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
//    step_x = inputVector[findIndexByString(geomag_proj_->nameList_real[data_index])].GridSizeX;
//    step_y = inputVector[findIndexByString(geomag_proj_->nameList_real[data_index])].GridSizeY;
    step_x = 0.5;
    step_y = 0.5;
    //一键读写文件
    dir = geomag_proj_->Path()+"/Processed/"+dir;
    ui->textBrowser->append("向上延拓处理开始...");
    QCoreApplication::processEvents();
    readfile.readfile_run(infile);
    readfile.cal_grid(step_x,step_y);
    //一键向上延拓
    yantuo.up_run(readfile.gridrow,readfile.gridcol,readfile.X, readfile.Y, readfile.T, step_x, step_y, h, dir.toStdString());
    //
    QTabWidget *tabwidget = new QTabWidget();
    double step1 = 0.5;
    // tab1
    QWidget *tab1 = new QWidget();
    QVBoxLayout *layout1 = new QVBoxLayout();
    draw_Form *draw_form_1 = new draw_Form;
    QVector<double> xx1,yy1,zz1;
    draw_form_1->create_xyz_f(QString::fromStdString(infile),xx1,yy1,zz1);
    draw_form_1->set_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->set_ContourView(QString::fromStdString(infile));
    layout1->addWidget(draw_form_1);
    tab1 ->setLayout(layout1);
    // tab2
    QWidget *tab2 = new QWidget();
    QVBoxLayout *layout2 = new QVBoxLayout();
    draw_Form *draw_form_2 = new draw_Form;
    QVector<double> xx2,yy2,zz2;
    draw_form_2->create_xyz_f(dir,xx2,yy2,zz2);
    draw_form_2->set_heatMapView(xx2,yy2,zz2);
    if(draw_form_2->magWarn == false)
        return;
    draw_form_2->set_ContourView(dir);
    layout2->addWidget(draw_form_2);
    tab2 ->setLayout(layout2);
    //
    tabwidget->addTab(tab1,"延拓前");
    tabwidget->addTab(tab2,"延拓后");
    tabwidget->setWindowTitle("向上延拓");
    tabwidget->resize(1100,700);
    tabwidget->show();
    ProjectChanged_processed();
    ui->textBrowser->append("向上延拓处理完成！");
}

void MainWindow::on_action_down_triggered()
{
    double step_x,step_y,h;
    int data_index;
    int type;
    QString dir;
    QDialog dialog(this);
    int ret = inputPara_down(dialog,h,type,data_num,geomag_proj_->nameList_real,data_index,dir);
    if (ret == -1)
        return;
    //调用类
    readFile readfile;
    FFTW fftw;
    yanTuo yantuo;
    std::string infile = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
//    step_x = inputVector[findIndexByString(geomag_proj_->nameList_real[data_index])].GridSizeX;
//    step_y = inputVector[findIndexByString(geomag_proj_->nameList_real[data_index])].GridSizeY;
    step_x = 0.5;
    step_y = 0.5;
    //一键读写文件
    dir = geomag_proj_->Path()+"/Processed/"+dir;
    ui->textBrowser->append("向下延拓处理开始...");
    QCoreApplication::processEvents();
    readfile.readfile_run(infile);
    readfile.cal_grid(step_x,step_y);
    yantuo.down_run(readfile.gridrow,readfile.gridcol, readfile.X, readfile.Y, readfile.T, step_x, step_y, h,
                    dir.toStdString(), type+1);
    QTabWidget *tabwidget = new QTabWidget();
    double step1 = 0.5;
    // tab1
    QWidget *tab1 = new QWidget();
    QVBoxLayout *layout1 = new QVBoxLayout();
    draw_Form *draw_form_1 = new draw_Form;
    QVector<double> xx1,yy1,zz1;
    draw_form_1->create_xyz_f(QString::fromStdString(infile),xx1,yy1,zz1);
    draw_form_1->set_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->set_ContourView(QString::fromStdString(infile));
    layout1->addWidget(draw_form_1);
    tab1 ->setLayout(layout1);
    // tab2
    QWidget *tab2 = new QWidget();
    QVBoxLayout *layout2 = new QVBoxLayout();
    draw_Form *draw_form_2 = new draw_Form;
    QVector<double> xx2,yy2,zz2;
    draw_form_2->create_xyz_f(dir,xx2,yy2,zz2);
    draw_form_2->set_heatMapView(xx2,yy2,zz2);
    if(draw_form_2->magWarn == false)
        return;
    draw_form_2->set_ContourView(dir);
    layout2->addWidget(draw_form_2);
    tab2 ->setLayout(layout2);
    //
    tabwidget->addTab(tab1,"延拓前");
    tabwidget->addTab(tab2,"延拓后");
    tabwidget->setWindowTitle("向下延拓");
    tabwidget->resize(1100,700);
    tabwidget->show();
    ProjectChanged_processed();
    ui->textBrowser->append("向下延拓处理完成！");
}

void MainWindow::on_action_evaluate_triggered()
{
    double step_x,step_y,h;
    int data_index;
    int type;
    QString dir;
    QDialog dialog(this);
    int ret = inputPara_down(dialog,h,type,data_num,geomag_proj_->nameList_real,data_index,dir);
    if (ret == -1)
        return;
    //调用类
    readFile readfile;
    FFTW fftw;
    yanTuo yantuo;
    std::string infile = (geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index]).toStdString();
//    step_x = inputVector[findIndexByString(geomag_proj_->nameList_real[data_index])].GridSizeX;
//    step_y = inputVector[findIndexByString(geomag_proj_->nameList_real[data_index])].GridSizeY;
    step_x = 0.5;
    step_y = 0.5;
    //一键读写文件
    dir = geomag_proj_->Path()+"/Processed/"+dir;
    ui->textBrowser->append("延拓精度检验开始...");
    QCoreApplication::processEvents();
    readfile.readfile_run(infile);
    readfile.cal_grid(step_x,step_y);
    yantuo.evaluatePrecision(readfile.gridrow,readfile.gridcol,
                             readfile.X, readfile.Y, readfile.T, step_x, step_y, h,type+1,dir.toStdString());
    ui->textBrowser->append(yantuo.out);
    //
    QTabWidget *tabwidget = new QTabWidget();
    double step1 = 0.5;
    // tab1
    QWidget *tab1 = new QWidget();
    QVBoxLayout *layout1 = new QVBoxLayout();
    draw_Form *draw_form_1 = new draw_Form;
    QVector<double> xx1,yy1,zz1;
    draw_form_1->create_xyz_f(QString::fromStdString(infile),xx1,yy1,zz1);
    draw_form_1->set_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->set_ContourView(QString::fromStdString(infile));
    layout1->addWidget(draw_form_1);
    tab1 ->setLayout(layout1);
    // tab2
    QWidget *tab2 = new QWidget();
    QVBoxLayout *layout2 = new QVBoxLayout();
    draw_Form *draw_form_2 = new draw_Form;
    QVector<double> xx2,yy2,zz2;
    draw_form_2->create_xyz_f(dir,xx2,yy2,zz2);
    draw_form_2->set_heatMapView(xx2,yy2,zz2);
    if(draw_form_2->magWarn == false)
        return;
    draw_form_2->set_ContourView(dir);
    layout2->addWidget(draw_form_2);
    tab2 ->setLayout(layout2);
    //
    tabwidget->addTab(tab1,"延拓前");
    tabwidget->addTab(tab2,"延拓后");
    tabwidget->setWindowTitle("精度评估");
    tabwidget->resize(1100,700);
    tabwidget->show();
    ProjectChanged_processed();
    ui->textBrowser->append("延拓精度检验完成！");
    QCoreApplication::processEvents();
}

void MainWindow::on_action_navPara_triggered()
{
    navPara_form_->show();
}

void MainWindow::on_actionTERCOM_triggered()
{
    if(navPara_form_->backGFile.isEmpty() ||
       navPara_form_->INSFile.isEmpty() ||
       navPara_form_->realFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请确认选择文件!");
        return;
    }
    // 设置背景图的分辨率，根据out.txt
    ui->textBrowser->append("TERCOM匹配导航计算开始...");
    QCoreApplication::processEvents();
    Geomagnetic::TercomMatching tm;
    tm.ReadBackground(navPara_form_->backGFile);
    tm.ReadINS(navPara_form_->INSFile);
    tm.ReadTruePath(navPara_form_->realFile);
    Geomagnetic::Datapoint result = tm.matchWithAdaptiveRotation();
    ui->textBrowser->append("TERCOM匹配导航计算完毕!");
    ui->textBrowser->append("等待绘图...");
    QCoreApplication::processEvents();
    tm.drawResult(result);
    //
    QFile file("/home/greatwall/build-whumag-desktop-Debug/tercom_out.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 如果文件打开失败，输出错误信息
        qWarning() << "fail" << file;
    }
    QTextStream out(&file);
    for (auto& point : result)
    {
        out << point.second.X<< "," << point.second.Y << endl;
    }
    file.close();
//    ui->textBrowser->append("TERCOM匹配导航RMS: "+QString::number(my.finalRMS) + " km");
    QCoreApplication::processEvents();
}

void MainWindow::on_actionICCP_triggered()
{
    if(navPara_form_->backGFile.isEmpty() ||
       navPara_form_->INSFile.isEmpty() ||
       navPara_form_->realFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请确认选择文件!");
        return;
    }
    //
    double dx,dy;
    QDialog dialog;
    QFormLayout form(&dialog);
    dialog.setWindowTitle("ICCP-输入参数: ");
    QLabel *label1 = new QLabel("经度/X分辨率") ;
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    QLabel *label2 = new QLabel("纬度/Y分辨率") ;
    QDoubleSpinBox *spinbox2 = new QDoubleSpinBox(&dialog);
    spinbox1->setDecimals(2);
    spinbox2->setDecimals(2);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(label1);
    horizontalLayout->addWidget(spinbox1);
    horizontalLayout->addWidget(label2);
    horizontalLayout->addWidget(spinbox2);
    form.addRow("格网分辨率:    ", horizontalLayout);
    // Add Cancel and OK button
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        dx=spinbox1->value();
        dy=spinbox2->value();
    }
    else
        return;
    if (dx==0 || dy==0)
    {
        return;
    }
    ICCP my;
    // 设置背景图的分辨率，根据out.txt
    my.dx=dx;
    my.dy=dy;
    ui->textBrowser->append("ICCP匹配导航计算开始...");
    QCoreApplication::processEvents();
    QVector<QPointF> X = my.cal(navPara_form_->backGFile,
                                navPara_form_->INSFile,
                                navPara_form_->realFile,0.001);
    ui->textBrowser->append("ICCP匹配导航计算完毕!");
    QCoreApplication::processEvents();
    ui->textBrowser->append("ICCP匹配导航RMS: "+QString::number(my.finalRMS) + " km");
    QCoreApplication::processEvents();
}

void MainWindow::on_actionSITAN_triggered()
{
    QMessageBox::warning(this,"警告","请确认选择文件1!");
    if(navPara_form_->backGFile.isEmpty() ||
       navPara_form_->INSFile.isEmpty() ||
       navPara_form_->realFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请确认选择文件!");
        return;
    }
    // 设置背景图的分辨率，根据out.txt
    ui->textBrowser->append("SITAN匹配导航计算开始...");
    QCoreApplication::processEvents();
//    Geomagnetic::SitanMatching st;
//    st.SITANAlgorithm(navPara_form_->backGFile,
//                      navPara_form_->INSFile,
//                      navPara_form_->realFile);
//    ui->textBrowser->append("SITAN匹配导航计算完毕!");
//    QCoreApplication::processEvents();
//    ui->textBrowser->append("TERCOM匹配导航RMS: "+QString::number(my.finalRMS) + " km");
//    QCoreApplication::processEvents();
}

void MainWindow::on_action_TERCOM_ICCP_triggered()
{
    if(navPara_form_->backGFile.isEmpty() ||
       navPara_form_->INSFile.isEmpty() ||
       navPara_form_->realFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请确认选择文件!");
        return;
    }
    // 设置背景图的分辨率，根据out.txt
    ui->textBrowser->append("TERCOM与ICCP联合匹配导航计算开始...");
    QCoreApplication::processEvents();
    Geomagnetic::TercomMatching tm;
    tm.ReadBackground(navPara_form_->backGFile);
    tm.ReadINS(navPara_form_->INSFile);
    tm.ReadTruePath(navPara_form_->realFile);
    Geomagnetic::Datapoint result = tm.matchWithAdaptiveRotation();
    QFile file(QDir::currentPath()+"/tercom_ins.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "fail" << file;
    }
    QTextStream out(&file);
    for (int i = 0;i<result.size();i++)
    {
        out << result.at(i).X <<"," <<result.at(i).Y << ","<< tm.insData.at(i).magnetic<<endl;
    }
    file.close();
    // ICCP
    ICCP my;
    // 设置背景图的分辨率，根据out.txt
    my.dx=0.5;
    my.dy=0.5;
    QVector<QPointF> X = my.cal(navPara_form_->backGFile,
                                navPara_form_->INSFile,
                                QDir::currentPath()+"/tercom_ins.csv",
                                navPara_form_->realFile,0.001);
    ui->textBrowser->append("ICCP匹配导航计算完毕!");
    QCoreApplication::processEvents();
    ui->textBrowser->append("ICCP匹配导航RMS: "+QString::number(my.finalRMS) + " km");
    QCoreApplication::processEvents();
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
//    sub.subareaAll(datapoints,datainfo,datapoint_sparse,x_step,y_step,outfile);
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
    draw_form_1->set_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->set_ContourView(filename);
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
    QDate date0;
    QDate date1;
    int data_index;
    int useGeoid;
    double height;
    QString dir;
    QDialog dialog(this);
    int ret = inputPara_correct(dialog,date0,date1,useGeoid,height,data_num,geomag_proj_->nameList_real,data_index,dir);
    if (ret == -1)
        return;
    TimeTongHua my;
    QString str = geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index];
    dir = geomag_proj_->Path()+"/Processed/"+dir;
    ui->textBrowser->append("通化处理开始...");
    QCoreApplication::processEvents();
    my.CalMag(str,useGeoid,height,date0,date1,dir);
    ui->textBrowser->append("通化处理完毕!");
    QCoreApplication::processEvents();
    //
    ui->textBrowser->append("绘制图像准备中...");
    QCoreApplication::processEvents();
    QTabWidget *tabwidget = new QTabWidget();
    double step1 = 0.5;
    // tab1
    QWidget *tab1 = new QWidget();
    QVBoxLayout *layout1 = new QVBoxLayout();
    draw_Form *draw_form_1 = new draw_Form;
    QVector<double> xx1,yy1,zz1;
    draw_form_1->create_xyz_f(str,xx1,yy1,zz1);
    draw_form_1->set_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->set_ContourView(str);
    layout1->addWidget(draw_form_1);
    tab1 ->setLayout(layout1);
    // tab2
    QWidget *tab2 = new QWidget();
    QVBoxLayout *layout2 = new QVBoxLayout();
    draw_Form *draw_form_2 = new draw_Form;
    QVector<double> xx2,yy2,zz2;
    draw_form_2->create_xyz_f(dir,xx2,yy2,zz2);
    draw_form_2->set_heatMapView(xx2,yy2,zz2);
    if(draw_form_2->magWarn == false)
        return;
    draw_form_2->set_ContourView(dir);
    layout2->addWidget(draw_form_2);
    tab2 ->setLayout(layout2);
    //
    tabwidget->addTab(tab1,"通化前");
    tabwidget->addTab(tab2,"通化后");
    tabwidget->setWindowTitle("通化");
    tabwidget->resize(1100,700);
    tabwidget->show();
    ui->textBrowser->append("绘制图像完毕!\n");
    QCoreApplication::processEvents();
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
    draw_form_->set_heatMapView(xx,yy,zz);
    if(draw_form_->magWarn == false)
        return;
    draw_form_->set_ContourView(filename);
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
    readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
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
    draw_form_1->set_heatMapView(xx1,yy1,zz1);
    if(draw_form_1->magWarn == false)
        return;
    draw_form_1->set_ContourView(filename);
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
    database_form->proPath = geomag_proj_->Path();
    database_form->setWindowState(Qt::WindowMaximized);
    auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    database_form->show();
    auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    std::chrono::duration<double> elapsed = end - start;
    QString outstr = "关于 - 页面切换时间: " + QString::number(elapsed.count()) + "s";
    ui->textBrowser->append(outstr+"\n");
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
    navPara_form_->setWindowState(Qt::WindowMaximized);
    navPara_form_->show();
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
    MagneticComplexityAnalyzer *mca = new MagneticComplexityAnalyzer;
    GridData gridData;
    QString fileNames = QFileDialog::getOpenFileName(this, QStringLiteral("选择背景场文件"));
    if(fileNames.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
        return;
    }
    ReadData readdata;
    Datapoint datapoints1;
    readdata.readGridFromFile(fileNames.toStdString(),datapoints1);
    double gridSize = 0.0045;
    int jumpSize = 15;
    ui->textBrowser->append("水面复杂度处理中...");
    QCoreApplication::processEvents();
    ComplexityResult result = mca->analyzeComplexity(datapoints1,gridSize);
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("csv");
    QString filename = dialog.getSaveFileName(this, tr("保存分析结果"), "", tr("CSV文件 (*.csv)"));

    if (!filename.isEmpty()) {
        if (mca->exportToFile(result, filename)) {
            QMessageBox::information(this, tr("导出成功"), tr("分析结果已保存到 %1").arg(filename));
        } else {
            QMessageBox::warning(this, tr("导出失败"), tr("无法写入文件 %1").arg(filename));
        }
    }

    // 显示复杂度热图
    mca->showComplexityMap(result,jumpSize);

    // 显示测线间距热图
    mca->showSpacingMap(result,jumpSize);
    // std::vector<double> result = mca->processData(datapoints1, gridSize,gridData);
    // auto [minIt, maxIt] = std::minmax_element(result.begin(), result.end());
    // if (!result.empty()) {
    //     std::cout << "Minimum value: " << *minIt << std::endl;
    //     std::cout << "Maximum value: " << *maxIt << std::endl;
    // } else {
    //     std::cout << "Result vector is empty." << std::endl;
    // }
    ui->textBrowser->append("水面复杂度处理完毕!");
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
    MagneticComplexityAnalyzer *mca = new MagneticComplexityAnalyzer;
    GridData gridData;
    QString fileNames = QFileDialog::getOpenFileName(this, QStringLiteral("选择背景场文件"));
    if(fileNames.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
        return;
    }
    ReadData readdata;
    Datapoint datapoints1;
    bool readSuccess = readdata.readGridFromFile(fileNames.toStdString(),datapoints1);
    if(!readSuccess || datapoints1.empty()) {
        QMessageBox::warning(this, "读取失败", "无法读取文件或文件不包含有效数据");
        return;
    }
    ui->textBrowser->append(QString("数据点数量: %1").arg(datapoints1.size()));
    double gridSize = 0.00045;
    int jumpSize = 150;        // 跳跃大小
    int chunkSize = 1000;       // 降低块大小
    int stepSize = 1;        // 步长
    mca->setSUBAREA_SIZE(jumpSize);
    ui->textBrowser->append("空中复杂度处理中...");
    QCoreApplication::processEvents();
    ComplexityResult result = mca->analyzeComplexityChunked(datapoints1,gridSize,jumpSize,chunkSize,true);
    ui->textBrowser->append("数据处理完成，准备导出结果...");
    QCoreApplication::processEvents();

    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("csv");
    QString filename = dialog.getSaveFileName(this, tr("保存分析结果"), "", tr("CSV文件 (*.csv)"));

    if (!filename.isEmpty()) {
        if (mca->exportToFile(result, filename)) {
            QMessageBox::information(this, tr("导出成功"), tr("分析结果已保存到 %1").arg(filename));
        } else {
            QMessageBox::warning(this, tr("导出失败"), tr("无法写入文件 %1").arg(filename));
        }
    }

    // 显示复杂度热图
    ui->textBrowser->append("生成复杂度热图...");
    QCoreApplication::processEvents();
    mca->showComplexityMap(result,jumpSize);

    ui->textBrowser->append("生成测线间距热图...");
    QCoreApplication::processEvents();
    mca->showSpacingMap(result,jumpSize);
    // std::vector<double> result = mca->processData(datapoints1, gridSize,gridData);
    // auto [minIt, maxIt] = std::minmax_element(result.begin(), result.end());
    // if (!result.empty()) {
    //     std::cout << "Minimum value: " << *minIt << std::endl;
    //     std::cout << "Maximum value: " << *maxIt << std::endl;
    // } else {
    //     std::cout << "Result vector is empty." << std::endl;
    // }
    ui->textBrowser->append("空中复杂度处理完毕!");
    QCoreApplication::processEvents();

    delete mca;
}



void MainWindow::on_action_xishushuixia_triggered()
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

    double totalRMS = myAccuracy.computeCheckLineAccuracy(backgroundPath, checkPath,complexityDataFile,8.5,0.1);

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

