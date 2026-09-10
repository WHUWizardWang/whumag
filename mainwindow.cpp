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
    taskList = ui->taskListWidget;
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
    // ui->textBrowser->append("正在初始化...");
    // QCoreApplication::processEvents();
    // taylor.init(datainfo);
    // ui->textBrowser->append("正在计算M矩阵...");
    // QCoreApplication::processEvents();
    // taylor.CalculateM(datapoint_sparse);
    // ui->textBrowser->append("正在计算AQ矩阵...");
    // QCoreApplication::processEvents();
    // taylor.CalculateAQ(datapoint_sparse);
    // ui->textBrowser->append("正在计算结果...");
    // QCoreApplication::processEvents();
    // taylor.Result(datapoint_result);
    // ui->textBrowser->append("结果正在保存与输出...");
    // QCoreApplication::processEvents();
    // readdata.resultOut(datapoint_result, s);
    // QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
    // double rms = calculateRMS(datapoints,datapoint_result);
    // ui->textBrowser->append("基于泰勒多项式方法结果计算完成\n");
    // ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
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
    try {
        // 1. 获取输入参数
        double step_x_deg, step_y_deg, h;
        Datapoint datapoints;
        bool useBL;
        int data_index;
        QString subDir;
        QDialog dialog(this);

        int ret = inputPara_up(
            dialog,
            h,
            useBL,
            data_num,
            geomag_proj_->nameList_real,
            data_index,
            subDir,
            step_x_deg,
            step_y_deg
            );

        if (ret == -1)
            return;

        // 2. 构造文件路径
        const QString inputFileName = geomag_proj_->nameList_real[data_index];
        const QString inputFilePath = geomag_proj_->Path() + "/Measured/" + inputFileName;
        const QString outDir = geomag_proj_->Path() + "/Processed/";
        QDir().mkpath(outDir);

        // 确保输出文件名有正确的扩展名
        QString outputFilePath = outDir + subDir;
        if (!outputFilePath.endsWith(".txt") && !outputFilePath.endsWith(".dat")) {
            outputFilePath += ".txt";
        }

        qDebug() << "=== 向上延拓处理参数 ===";
        qDebug() << "输入文件:" << inputFilePath;
        qDebug() << "输出文件:" << outputFilePath;
        qDebug() << "使用BL:" << useBL;
        qDebug() << "步长:" << step_x_deg << "," << step_y_deg;
        qDebug() << "高度:" << h;

        ui->textBrowser->append("向上延拓处理开始...");
        ui->textBrowser->append(QString("输出文件: %1").arg(outputFilePath));
        QCoreApplication::processEvents();

        // 验证输入文件
        if (!QFile::exists(inputFilePath)) {
            ui->textBrowser->append("错误: 输入文件不存在");
            return;
        }

        // 先同步读入数据
        ReadData readdata;
        readdata.readGridFromFile(inputFilePath.toStdString(), datapoints);

        if (datapoints.empty()) {
            ui->textBrowser->append("错误: 输入数据为空");
            return;
        }

        qDebug() << "读取数据点数:" << datapoints.size();

        // 3. 创建任务项
        QString taskName = QString("向上延拓: %1").arg(inputFileName);
        QListWidgetItem *item = new QListWidgetItem(taskName + " （进行中…）");
        taskList->addItem(item);

        // 4. 创建线程安全的数据拷贝
        auto datapointsCopy = std::make_shared<Datapoint>(datapoints);

        // 5. 使用 QtConcurrent::run 执行后台任务
        auto future = QtConcurrent::run([=]() -> bool {
            try {
                qDebug() << "后台线程开始处理...";

                yanTuo yantuo;

                // 使用拷贝的数据
                Datapoint localDatapoints = *datapointsCopy;

                if (useBL) {
                    yantuo.up_run_BL(localDatapoints,
                                     step_x_deg,
                                     step_y_deg,
                                     h,
                                     outputFilePath.toStdString());
                } else {
                    yantuo.up_run(localDatapoints,
                                  step_x_deg,
                                  step_y_deg,
                                  h,
                                  outputFilePath.toStdString());
                }

                // 验证输出文件是否成功创建
                if (!QFile::exists(outputFilePath)) {
                    qWarning() << "输出文件创建失败:" << outputFilePath;
                    return false;
                }

                QFileInfo outputInfo(outputFilePath);
                if (outputInfo.size() == 0) {
                    qWarning() << "输出文件为空:" << outputFilePath;
                    return false;
                }

                qDebug() << "后台处理完成，输出文件大小:" << outputInfo.size() << "字节";
                return true;

            } catch (const std::exception& e) {
                qCritical() << "后台处理异常:" << e.what();
                return false;
            } catch (...) {
                qCritical() << "后台处理未知异常";
                return false;
            }
        });

        // 6. 监控任务完成
        QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
        watcher->setFuture(future);

        connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
            try {
                bool success = future.result();

                if (!success) {
                    ui->textBrowser->append("向上延拓处理失败！");
                    item->setText(taskName + " （处理失败）");
                    watcher->deleteLater();
                    return;
                }

                ui->textBrowser->append("向上延拓处理完成！");
                item->setText(taskName + " （已完成）");
                QCoreApplication::processEvents();

                // 验证文件存在后再创建图表
                if (!QFile::exists(outputFilePath)) {
                    ui->textBrowser->append("错误: 输出文件不存在，无法显示结果");
                    watcher->deleteLater();
                    return;
                }

                // 7. 创建标签页并绘图
                QTabWidget *tabWidget = new QTabWidget();

                // —— 延拓前 ——
                {
                    QWidget *tab1 = new QWidget(tabWidget);
                    QVBoxLayout *layout1 = new QVBoxLayout(tab1);
                    draw_Form *draw_form1 = new draw_Form(tab1);

                    QVector<double> xx1, yy1, zz1;
                    for (const auto& elem : datapoints) {
                        xx1.push_back(elem.second.X);
                        yy1.push_back(elem.second.Y);
                        zz1.push_back(elem.second.tMagnetic);
                    }

                    if (!xx1.isEmpty()) {
                        draw_form1->autoset_heatMapView(xx1, yy1, zz1);
                        if (draw_form1->magWarn) {
                            draw_form1->autoset_contourView(xx1, yy1, zz1);
                        }
                    }

                    layout1->addWidget(draw_form1);
                    tabWidget->addTab(tab1, QStringLiteral("延拓前"));
                }

                // —— 延拓后 ——
                {
                    QWidget *tab2 = new QWidget(tabWidget);
                    QVBoxLayout *layout2 = new QVBoxLayout(tab2);
                    draw_Form *draw_form2 = new draw_Form(tab2);

                    try {
                        QVector<double> xx2, yy2, zz2;
                        draw_form2->create_xyz_f(outputFilePath, xx2, yy2, zz2);

                        if (!xx2.isEmpty()) {
                            draw_form2->autoset_heatMapView(xx2, yy2, zz2);
                            if (draw_form2->magWarn) {
                                draw_form2->autoset_contourView(xx2, yy2, zz2);
                            }
                        } else {
                            ui->textBrowser->append("警告: 延拓后数据为空");
                        }
                    } catch (const std::exception& e) {
                        ui->textBrowser->append(QString("延拓后数据读取失败: %1").arg(e.what()));
                    }

                    layout2->addWidget(draw_form2);
                    tabWidget->addTab(tab2, QStringLiteral("延拓后"));
                }

                tabWidget->setWindowTitle(QStringLiteral("向上延拓"));
                tabWidget->resize(1100, 700);
                tabWidget->show();

                ProjectChanged_processed();

            } catch (const std::exception& e) {
                ui->textBrowser->append(QString("结果显示失败: %1").arg(e.what()));
            }

            watcher->deleteLater();
        });

    } catch (const std::exception& e) {
        ui->textBrowser->append(QString("向上延拓初始化失败: %1").arg(e.what()));
    } catch (...) {
        ui->textBrowser->append("向上延拓初始化失败: 未知错误");
    }
}

void MainWindow::on_action_down_triggered()
{
    /************* 1. 收集输入参数（保持原逻辑） *************/
    double step_x, step_y, h;
    int    data_index;
    int    type;
    bool   useBL;
    QString subDir;
    QDialog dialog(this);

    int ret = inputPara_down(dialog, h, type, useBL,
                             data_num, geomag_proj_->nameList_real,
                             data_index, subDir,
                             step_x, step_y);
    if (ret == -1) return;

    /************* 2. 组装路径 *************/
    const QString inputFileName  = geomag_proj_->nameList_real[data_index];
    const QString inputFilePath  = geomag_proj_->Path() + "/Measured/"  + inputFileName;
    const QString outDir         = geomag_proj_->Path() + "/Processed/";
    QDir().mkpath(outDir);                       // 确保目录存在
    const QString outputFilePath = outDir + subDir;

    ui->textBrowser->append("向下延拓处理开始（后台线程）...");
    QString taskName = QString("向下延拓: %1").arg(inputFileName);
    QListWidgetItem *item = new QListWidgetItem(taskName + " （进行中…）");
    taskList->addItem(item);
    QCoreApplication::processEvents();

    /************* 3. 创建 QFutureWatcher *************/
    // 返回值用 std::optional<QString> 保存潜在的错误信息；空 => 成功
    using TaskResult = std::optional<QString>;
    auto *watcher = new QFutureWatcher<TaskResult>(this);


    /************* 4. 启动后台任务 *************/
    QFuture<TaskResult> future = QtConcurrent::run([=]() -> TaskResult {
        try {
            /* 4.1 读取数据 */
            ReadData   readfile;
            Datapoint  datapoints;
            if (!readfile.readGridFromFile(inputFilePath.toStdString(), datapoints))
                return QStringLiteral("读取原始数据失败！");

            /* 4.2 延拓计算 */
            yanTuo yantuo;
            if (useBL) {
                yantuo.down_run_BL(datapoints,
                                   step_x, step_y,
                                   h,
                                   outputFilePath.toStdString(),
                                   type + 1);
            } else {
                yantuo.down_run(datapoints,
                                step_x, step_y,
                                h,
                                outputFilePath.toStdString(),
                                type + 1);
            }
            return std::nullopt;        // 成功
        } catch (std::exception &e) {   // 捕获所有 C++ 异常
            return QString::fromLocal8Bit(e.what());
        }
    });

    watcher->setFuture(future);

    /************* 5. 任务完成后回到 UI 线程 *************/
    connect(watcher, &QFutureWatcher<TaskResult>::finished,
            this, [=]() {
                TaskResult err = watcher->future().result();

                watcher->deleteLater();         // 释放 watcher

                if (err) {                      // 任务失败
                    ui->textBrowser->append("向下延拓失败: " + *err);
                    item->setText(taskName + " （处理失败）");
                    return;
                }

                ui->textBrowser->append("向下延拓处理完成！");
                item->setText(taskName + " （已完成）");
                QCoreApplication::processEvents();
                /***** 5.1 绘图与界面更新（必须在 GUI 线程执行） *****/
                QTabWidget *tabWidget = new QTabWidget();

                // —— 延拓前
                {
                    QWidget *tab1 = new QWidget(tabWidget);
                    QVBoxLayout *layout1 = new QVBoxLayout(tab1);
                    auto *draw_form1 = new draw_Form(tab1);
                    QVector<double> xx1, yy1, zz1;
                    draw_form1->create_xyz_f(inputFilePath, xx1, yy1, zz1);
                    draw_form1->autoset_heatMapView(xx1, yy1, zz1);
                    if (draw_form1->magWarn)
                        draw_form1->autoset_contourView(xx1, yy1, zz1);
                    layout1->addWidget(draw_form1);
                    tabWidget->addTab(tab1, QStringLiteral("延拓前"));
                }

                // —— 延拓后
                {
                    QWidget *tab2 = new QWidget(tabWidget);
                    QVBoxLayout *layout2 = new QVBoxLayout(tab2);
                    auto *draw_form2 = new draw_Form(tab2);
                    QVector<double> xx2, yy2, zz2;
                    draw_form2->create_xyz_f(outputFilePath, xx2, yy2, zz2);
                    draw_form2->autoset_heatMapView(xx2, yy2, zz2);
                    if (draw_form2->magWarn)
                        draw_form2->autoset_contourView(xx2, yy2, zz2);
                    layout2->addWidget(draw_form2);
                    tabWidget->addTab(tab2, QStringLiteral("延拓后"));
                }

                tabWidget->setWindowTitle(QStringLiteral("向下延拓"));
                tabWidget->resize(1100, 700);
                tabWidget->show();

                ProjectChanged_processed();
                ui->textBrowser->append("向下延拓处理完成！");
            });
}

void MainWindow::on_action_evaluate_triggered()
{
    /************* 1. 收集参数（与原逻辑相同） *************/
    double step_x, step_y, h;
    int    data_index;
    int    type;
    bool   useBL;
    QString subDir;
    QDialog dialog(this);

    int ret = inputPara_down(dialog, h, type, useBL,
                             data_num, geomag_proj_->nameList_real,
                             data_index, subDir,
                             step_x, step_y);
    if (ret == -1) return;

    /************* 2. 组装路径 *************/
    const QString inputFileName  = geomag_proj_->nameList_real[data_index];
    const QString inputFilePath  = geomag_proj_->Path() + "/Measured/"  + inputFileName;
    const QString outDir         = geomag_proj_->Path() + "/Processed/";
    QDir().mkpath(outDir);
    const QString outputFilePath = outDir + subDir;

    ui->textBrowser->append("延拓精度检验开始（后台线程）...");
    QCoreApplication::processEvents();

    /************* 3. 创建 QFutureWatcher *************/
    struct EvalResult {
        std::optional<QString>  error;     // 有错误则存错误信息
        QString                 yantuoOut; // 评估报告
    };

    auto *watcher = new QFutureWatcher<EvalResult>(this);
    QString taskName = QString("延拓精度评估: %1").arg(inputFileName);
    QListWidgetItem *item = new QListWidgetItem(taskName + " （进行中…）");
    taskList->addItem(item);
    QCoreApplication::processEvents();
    /************* 4. 启动后台任务 *************/
    QFuture<EvalResult> future = QtConcurrent::run([=]() -> EvalResult {
        EvalResult res;

        try {
            /* 4.1 读取数据 */
            ReadData  reader;
            Datapoint datapoints;
            if (!reader.readGridFromFile(inputFilePath.toStdString(), datapoints)) {
                res.error = QStringLiteral("读取原始数据失败！");
                return res;
            }

            /* 4.2 精度评估 */
            yanTuo yantuo;
            yantuo.evaluatePrecision(datapoints,
                                     useBL,
                                     step_x, step_y,
                                     h,
                                     type + 1,
                                     outputFilePath.toStdString());

            res.yantuoOut = yantuo.out;
            return res;                // 成功
        } catch (std::exception &e) {
            res.error = QString::fromLocal8Bit(e.what());
            return res;
        }
    });

    watcher->setFuture(future);

    /************* 5. 完成后回到 UI 线程 *************/
    connect(watcher, &QFutureWatcher<EvalResult>::finished,
            this, [=]() {
                EvalResult r = watcher->future().result();
                watcher->deleteLater();

                if (r.error) {                         // 评估失败
                    ui->textBrowser->append("延拓精度检验失败: " + *r.error);
                    item->setText(taskName + " （处理失败）");
                    return;
                }

                /***** 5.1 显示评估报告 *****/
                item->setText(taskName + " （已完成）");
                ui->textBrowser->append(r.yantuoOut);

                /***** 5.2 绘图 TabWidget *****/
                QTabWidget *tabWidget = new QTabWidget();

                // —— 延拓前
                {
                    QWidget *tab1 = new QWidget(tabWidget);
                    QVBoxLayout *layout1 = new QVBoxLayout(tab1);
                    auto *draw_form1 = new draw_Form(tab1);
                    QVector<double> xx1, yy1, zz1;
                    draw_form1->create_xyz_f(inputFilePath, xx1, yy1, zz1);
                    draw_form1->autoset_heatMapView(xx1, yy1, zz1);
                    if (draw_form1->magWarn)
                        draw_form1->autoset_contourView(xx1, yy1, zz1);
                    layout1->addWidget(draw_form1);
                    tabWidget->addTab(tab1, QStringLiteral("延拓前"));
                }

                // —— 延拓后
                {
                    QWidget *tab2 = new QWidget(tabWidget);
                    QVBoxLayout *layout2 = new QVBoxLayout(tab2);
                    auto *draw_form2 = new draw_Form(tab2);
                    QVector<double> xx2, yy2, zz2;
                    draw_form2->create_xyz_f(outputFilePath, xx2, yy2, zz2);
                    draw_form2->autoset_heatMapView(xx2, yy2, zz2);
                    if (draw_form2->magWarn)
                        draw_form2->autoset_contourView(xx2, yy2, zz2);
                    layout2->addWidget(draw_form2);
                    tabWidget->addTab(tab2, QStringLiteral("延拓后"));
                }

                tabWidget->setWindowTitle(QStringLiteral("精度评估"));
                tabWidget->resize(1100, 700);
                tabWidget->show();

                ProjectChanged_processed();
                ui->textBrowser->append("延拓精度检验完成！");
            });
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
        out << point.second.X<< "," << point.second.Y << Qt::endl;
    }
    file.close();
//    ui->textBrowser->append("TERCOM匹配导航RMS: "+QString::number(my.finalRMS) + " km");
    QCoreApplication::processEvents();
}

void MainWindow::on_actionICCP_triggered()
{
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
        out << result.at(i).X <<"," <<result.at(i).Y << ","<< tm.insData.at(i).magnetic<<Qt::endl;
    }
    file.close();
    // ICCP
    ICCP my;
    // 设置背景图的分辨率，根据out.txt
    QVector<QPointF> X = my.cal(navPara_form_->backGFile,
                                navPara_form_->INSFile,
                                QDir::currentPath()+"/tercom_ins.csv",
                                navPara_form_->realFile,0.001);
    ui->textBrowser->append("ICCP匹配导航计算完毕!");
    QCoreApplication::processEvents();
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
    // 定义变量：两个日期、数据索引、是否使用大地水准面、测量高度、输出目录等
    QDate date0;
    QDate date1;
    int data_index;
    int useGeoid;
    double height;
    QString dir;
    QDialog dialog(this);

    // 弹出对话框，获取纠正（“通化”）所需的各项参数
    // 参数列表含义：
    //  dialog: 由 Qt 创建的模态对话框
    //  date0, date1: 用户选择的起始/结束日期
    //  useGeoid: 是否使用大地水准面（0/1）
    //  height: 测量高度（单位通常是米）
    //  data_num: （假设为成员变量）可供选择的数据个数
    //  geomag_proj_->nameList_real: 已加载的实测数据文件名列表
    //  data_index: 选中的某条测线在 nameList_real 中的下标
    //  dir: 用户指定的输出文件子目录
    int ret = inputPara_correct(dialog,date0,date1,useGeoid,height,data_num,geomag_proj_->nameList_real,data_index,dir);
    if (ret == -1)
        return;

    TimeTongHua my;

    QString str = geomag_proj_->Path()+"/Measured/"+geomag_proj_->nameList_real[data_index];
    dir = geomag_proj_->Path()+"/Processed/"+dir;

    // 在文本浏览器（textBrowser）中输出状态，表示“通化”过程开始
    ui->textBrowser->append("通化处理开始...");
    QCoreApplication::processEvents();

    QString taskName = QString("通化: %1").arg(geomag_proj_->nameList_real[data_index]);
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(taskName + " （进行中...）");
    taskList->addItem(item);

    // 创建一个 QFutureWatcher<void>，用来监控后台任务何时结束
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

    // 当 watcher 收到 finished() 信号时，说明后台任务跑完了
    connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
        // 注意：这里是槽函数，运行在主线程，UI 可以直接更新

        ui->textBrowser->append("通化处理完毕!");
        item->setText(taskName + " （已完成）");
        QCoreApplication::processEvents();


        // —— 进入绘图部分 ——
        ui->textBrowser->append("绘制图像准备中...");
        QCoreApplication::processEvents();


        // 新建一个 tabwidget 并在其中绘制“通化前/后”的图
        QTabWidget *tabwidget = new QTabWidget;
        // tab1：通化前
        {
            QWidget *tab1 = new QWidget;
            QVBoxLayout *layout1 = new QVBoxLayout(tab1);
            draw_Form *draw1 = new draw_Form;
            QVector<double> xx1, yy1, zz1;
            draw1->create_xyz_f(str, xx1, yy1, zz1);
            draw1->autoset_heatMapView(xx1, yy1, zz1);
            if (!draw1->magWarn) {
                delete tabwidget;
                return;  // 如果出现 magWarn 警告，则不继续绘制
            }
            draw1->autoset_contourView(xx1, yy1, zz1);
            layout1->addWidget(draw1);
            tabwidget->addTab(tab1, "通化前");
        }

        // tab2：通化后
        {
            QWidget *tab2 = new QWidget;
            QVBoxLayout *layout2 = new QVBoxLayout(tab2);
            draw_Form *draw2 = new draw_Form;
            QVector<double> xx2, yy2, zz2;
            draw2->create_xyz_f(dir, xx2, yy2, zz2);
            draw2->autoset_heatMapView(xx2, yy2, zz2);
            if (!draw2->magWarn) {
                delete tabwidget;
                return;
            }
            draw2->autoset_contourView(xx2, yy2, zz2);
            layout2->addWidget(draw2);
            tabwidget->addTab(tab2, "通化后");
        }

        tabwidget->setWindowTitle("通化");
        tabwidget->resize(1100, 700);
        tabwidget->show();

        ui->textBrowser->append("绘制图像完毕!\n");
        QCoreApplication::processEvents();

        // 任务结束后，记得 delete watcher
        watcher->deleteLater();
    });

    // 使用 QtConcurrent::run 在后台线程执行 CalMag
    QFuture<void> future = QtConcurrent::run([=]() {
        // 这里写后台计算逻辑，和原来 my.CalMag 一模一样
        TimeTongHua my;
        my.CalMag(str, useGeoid, height, date0, date1, dir);
    });
    watcher->setFuture(future);
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

    MagneticComplexityAnalyzer *mca = new MagneticComplexityAnalyzer;
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
//    std::vector<double> zz = interpolator.interpolateGridWithEigen(x, y, z, xx, yy);
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
    ComplexityResult result = mca->analyzeComplexityChunked(datapoints1,gridSize,jumpSize);
    QFileDialog dialog_save(this);
    dialog_save.setAcceptMode(QFileDialog::AcceptSave);
    dialog_save.setDefaultSuffix("csv");
    QString filename = dialog_save.getSaveFileName(this, tr("保存分析结果"), "", tr("CSV文件 (*.csv)"));

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
    // mca->showSpacingMap(result,jumpSize);

    ui->textBrowser->append("复杂度处理完毕!");
    QCoreApplication::processEvents();

}

