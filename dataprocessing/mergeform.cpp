#include "mergeform.h"
#include "ui_mergeform.h"


mergeForm::mergeForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mergeForm)
{
    ui->setupUi(this);
    //
    QStandardItemModel *model = new QStandardItemModel(3,2,this);
    model->setHorizontalHeaderLabels({"filepath","error"});
    ui->tableView->setModel(model);
    ui->tableView->setColumnWidth(1,60);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);

}

mergeForm::~mergeForm()
{
    delete ui;
}

void mergeForm::on_chooseFiles_clicked()
{
    // 选择多个文件
    QStringList fileNames;
    fileNames = QFileDialog::getOpenFileNames(this, tr("打开数据"),
                                                  QCoreApplication::applicationFilePath(),
                                                  tr("All Files (*.*);;文本文件 (*.txt *.dat *.csv)"));
    if(fileNames.isEmpty() || fileNames.size() == 1)
    {
       QMessageBox::warning(this,"警告","请选择多个文件");
    }
    //
    QStandardItemModel *model = new QStandardItemModel(fileNames.size(),2,this);
    model->setHorizontalHeaderLabels({"文件","中误差"});
    for (int i =0 ;i<fileNames.size();i++)
    {
        QStandardItem *item = new QStandardItem(fileNames.at(i));
        model->setItem(i,0,item);
        myMerge.doc.push_back(fileNames.at(i).toStdString());
    }
    ui->tableView->setModel(model);
    ui->tableView->setColumnWidth(1,60);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
}

void mergeForm::readTable()
{
    myMerge.doc.clear();
    QAbstractItemModel *model = ui->tableView->model();
    if(!model) return;
    filecount = model->rowCount();
    for (int i=0;i<filecount;i++)
    {
        QModelIndex index1 = model->index(i,0);
        QModelIndex index2 = model->index(i,1);
        myMerge.doc.push_back(model->data(index1).toString().toStdString());
        myMerge.doc_m.push_back(model->data(index2).toDouble());
    }
}

void mergeForm::on_confirm_clicked()
{

    myMerge.doc.clear();
    myMerge.doc_m.clear();
    myMerge.doc_P.clear();
    myMerge.point0.clear();
    myMerge.doc_points.clear();
    myMerge.data.clear();
    myMerge.all_point.clear();

    double p1;
    double p2;

    std::vector<double>T1;
    std::vector<double>T2;
    emit textUpdated("数据及参数读取开始...");
    QCoreApplication::processEvents();
    readTable();
    for (int i=0;i<filecount;i++)
    {
        if(myMerge.doc_m[i]==0)
        {
            QMessageBox::warning(this,"警告","请检查文件对应中误差");
            return;
        }
    }
    if (ui->lineEdit->text().isEmpty())
    {
        QMessageBox::warning(this,"警告","请输入保存的文件名");
        return;
    }
    double window_size1 = ui->lineEdit_lonx->text().toDouble();
    double window_size2 = ui->lineEdit_laty->text().toDouble();
    double min_B = ui->lineEdit_lat_min->text().toDouble();
    double min_L = ui->lineEdit_lon_min->text().toDouble();
    double max_B = ui->lineEdit_lat_max->text().toDouble();
    double max_L = ui->lineEdit_lon_max->text().toDouble();
    double Bint = ui->lineEdit_lat_step->text().toDouble();
    double Lint = ui->lineEdit_lon_step->text().toDouble();
    filename = ui->lineEdit->text();
    close();
    emit textUpdated("数据及参数读取完毕!");
    QCoreApplication::processEvents();
    emit textUpdated("数据融合处理开始...");
    QCoreApplication::processEvents();
    myMerge.rongHe_run2(window_size1,window_size2,min_B,min_L,max_B,max_L,Bint,Lint);
    emit textUpdated("数据融合处理完毕!");
    myMerge.outResult(filepath+"/Processed/"+filename);
    emit textUpdated("结果输出至本地完毕!");
    QCoreApplication::processEvents();
    emit textUpdated("绘制图像准备中...");
    QCoreApplication::processEvents();
    //
    QTabWidget *tabwidget = new QTabWidget();
    double step1 = 1.0/30.0;
    for(int i = 0;i<myMerge.doc.size();i++)
    {
        QString file_i = QString::fromStdString(myMerge.doc[i]);
        // tab1
        QWidget *tab1 = new QWidget();
        QVBoxLayout *layout1 = new QVBoxLayout();
        draw_Form *draw_form_1 = new draw_Form;
        QVector<double> xx1,yy1,zz1;
        draw_form_1->create_xyz_f(file_i,xx1,yy1,zz1);
        draw_form_1->set_heatMapView(xx1,yy1,zz1);
        draw_form_1->set_ContourView(file_i);
        layout1->addWidget(draw_form_1);
        tab1 ->setLayout(layout1);
        QString str;
        int lastSlashIndex = file_i.lastIndexOf('/');
        int lastDotIndex = file_i.lastIndexOf('.');
        if (lastSlashIndex != -1 && lastDotIndex != -1 && lastSlashIndex < lastDotIndex)
                str = file_i.mid(lastSlashIndex + 1, lastDotIndex - lastSlashIndex - 1);
        else
            str = "file_"+QString::number(i);
        tabwidget->addTab(tab1,str);
    }
    // tab3
    QWidget *tab3 = new QWidget();
    QVBoxLayout *layout3 = new QVBoxLayout();
    draw_Form *draw_form_3 = new draw_Form;
    QVector<double> xx3,yy3,zz3;
    draw_form_3->create_xyz_f(filepath+"/Processed/"+filename,xx3,yy3,zz3);
    draw_form_3->set_heatMapView(xx3,yy3,zz3);
    draw_form_3->set_ContourView(filepath+"/Processed/"+filename);
    layout3->addWidget(draw_form_3);
    tab3 ->setLayout(layout3);
    //
    tabwidget->addTab(tab3,"merge");
    tabwidget->setWindowTitle("数据融合");
    tabwidget->resize(1100,700);
    tabwidget->show();
    emit textUpdated("绘制图像完毕!");
    QCoreApplication::processEvents();
    // 更新主界面的树
    emit treeUpdated(1);
}

//void mergeForm::on_confirm_clicked()
//{

//    myMerge.doc.clear();
//    myMerge.doc_m.clear();
//    myMerge.doc_P.clear();
//    myMerge.point0.clear();
//    myMerge.doc_points.clear();
//    myMerge.data.clear();
//    myMerge.all_point.clear();

//    double p1;
//    double p2;

//    vector<double>T1;
//    vector<double>T2;
//    emit textUpdated("数据及参数读取开始...");
//    QCoreApplication::processEvents();
//    readTable();
//    for (int i=0;i<filecount;i++)
//    {
//        if(myMerge.doc_m[i]==0)
//        {
//            QMessageBox::warning(this,"警告","请检查文件对应中误差");
//            return;
//        }
//    }
//    if (ui->lineEdit->text().isEmpty())
//    {
//        QMessageBox::warning(this,"警告","请输入保存的文件名");
//        return;
//    }
//    double window_size1 = ui->lineEdit_lonx->text().toDouble();
//    double window_size2 = ui->lineEdit_laty->text().toDouble();
//    double min_B = ui->lineEdit_lat_min->text().toDouble();
//    double min_L = ui->lineEdit_lon_min->text().toDouble();
//    double max_B = ui->lineEdit_lat_max->text().toDouble();
//    double max_L = ui->lineEdit_lon_max->text().toDouble();
//    double Bint = ui->lineEdit_lat_step->text().toDouble();
//    double Lint = ui->lineEdit_lon_step->text().toDouble();
//    filename = ui->lineEdit->text();
//    close();
//    emit textUpdated("数据及参数读取完毕!");
//    QCoreApplication::processEvents();
//    emit textUpdated("数据融合处理开始...");
//    QCoreApplication::processEvents();
//    myMerge.rongHe_run2(window_size1,window_size2,min_B,min_L,max_B,max_L,Bint,Lint);
//    emit textUpdated("数据融合处理完毕!");
//    myMerge.outResult(filepath+"/Processed/"+filename);
//    emit textUpdated("结果输出至本地完毕!");
//    QCoreApplication::processEvents();
//    emit textUpdated("绘制图像准备中...");
//    QCoreApplication::processEvents();
//    //
//    QTabWidget *tabwidget = new QTabWidget();
//    double step1 = 1.0/30.0;
//    // tab1
//    QWidget *tab1 = new QWidget();
//    QVBoxLayout *layout1 = new QVBoxLayout();
//    draw_Form *draw_form_1 = new draw_Form;
//    QVector<double> xx1,yy1,zz1;
//    draw_form_1->create_xyz_f(QString::fromStdString(myMerge.doc[0]),xx1,yy1,zz1);
//    draw_form_1->set_heatMapView(xx1,yy1,zz1,step1,step1);
//    draw_form_1->set_ContourView(QString::fromStdString(myMerge.doc[0]),filepath+"/Processed/");
//    layout1->addWidget(draw_form_1);
//    tab1 ->setLayout(layout1);
//    // tab2
//    QWidget *tab2 = new QWidget();
//    QVBoxLayout *layout2 = new QVBoxLayout();
//    draw_Form *draw_form_2 = new draw_Form;
//    QVector<double> xx2,yy2,zz2;
//    draw_form_2->create_xyz_f(QString::fromStdString(myMerge.doc[1]),xx2,yy2,zz2);
//    draw_form_2->set_heatMapView(xx2,yy2,zz2,step1,step1);
//    draw_form_2->set_ContourView(QString::fromStdString(myMerge.doc[1]),filepath+"/Processed/");
//    layout2->addWidget(draw_form_2);
//    tab2 ->setLayout(layout2);
//    // tab3
//    QWidget *tab3 = new QWidget();
//    QVBoxLayout *layout3 = new QVBoxLayout();
//    draw_Form *draw_form_3 = new draw_Form;
//    QVector<double> xx3,yy3,zz3;
//    draw_form_3->create_xyz_f(filepath+"/Processed/"+filename,xx3,yy3,zz3);
//    draw_form_3->set_heatMapView(xx3,yy3,zz3,0.073,0.073);
//    draw_form_3->set_ContourView(filepath+"/Processed/"+filename,filepath+"/Processed/");
//    layout3->addWidget(draw_form_3);
//    tab3 ->setLayout(layout3);
//    //
//    tabwidget->addTab(tab1,"file_1");
//    tabwidget->addTab(tab2,"file_2");
//    tabwidget->addTab(tab3,"merge");
//    tabwidget->setWindowTitle("数据融合");
//    tabwidget->resize(1100,700);
//    tabwidget->show();
//    emit textUpdated("绘制图像完毕!");
//    QCoreApplication::processEvents();
//    // 更新主界面的树
//    emit treeUpdated(1);
//}
