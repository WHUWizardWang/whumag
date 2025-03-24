#include "referencemap.h"
#include "ui_referencemap.h"


ReferenceMap::ReferenceMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReferenceMap)
    , taylorlegendre_form_ (new maptaylorlegendreform)
    , polyhedral_form_ (new mappolyhedralform)
    , spline_form_ (new mapsplineform)
    , compress_form_ (new mapcompressform)
    , lssvmpso_form_ (new maplssvmpsoform)
{
    ui->setupUi(this);
    // 初始化：设置自动和手动参数设置按钮
    QButtonGroup *block = new QButtonGroup(this);
    block->addButton(ui->radioButton_auto,0);
    block->addButton(ui->radioButton_self,1);
    ui->radioButton_auto->setChecked(1);
    block->setExclusive(true);
    connect(block, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &ReferenceMap::on_radioButtonGroup_toggled);
    setFormsEditable(false);

    // 初始化：6个建模方法，comboBox_model
    QStringList models = {"泰勒多项式","勒让德多项式","多面函数","样条函数","压缩感知","深度学习"};
    ui->comboBox_model->addItems(models);
    ui->comboBox_model->setCurrentIndex(-1);
    ui->verticalLayout_para->addWidget(taylorlegendre_form_);
    ui->verticalLayout_para->addWidget(polyhedral_form_);
    ui->verticalLayout_para->addWidget(spline_form_);
    ui->verticalLayout_para->addWidget(compress_form_);
    ui->verticalLayout_para->addWidget(lssvmpso_form_);
    // 初始化：设置建模界面不可见
    taylorlegendre_form_->setVisible(false);
    polyhedral_form_->setVisible(false);
    spline_form_->setVisible(false);
    compress_form_->setVisible(false);
    lssvmpso_form_->setVisible(false);
    ui->verticalLayout_para->layout()->setAlignment(Qt::AlignTop);
    // 初始化：设置选择数据一栏
    ui->comboBox_data->addItems(nameList_real);
    // 初始化：保存文件名

}

ReferenceMap::~ReferenceMap()
{
    delete ui;
}
void ReferenceMap::on_radioButtonGroup_toggled(int id)
{
    // id=0 对应 radioButton_auto
    // id=1 对应 radioButton_self
    bool isEditable = (id == 1);
    setFormsEditable(isEditable);
}

void ReferenceMap::setFormsEditable(bool editable)
{
    // 设置所有表单的可编辑状态
    taylorlegendre_form_->setEnabled(editable);
    polyhedral_form_->setEnabled(editable);
    spline_form_->setEnabled(editable);
    compress_form_->setEnabled(editable);
    lssvmpso_form_->setEnabled(editable);
}
void ReferenceMap::updateComboxData()
{
    // 初始化：设置选择数据一栏
    ui->comboBox_data->clear();
    ui->comboBox_data->addItems(nameList_real);
}

void ReferenceMap::on_comboBox_model_currentIndexChanged(int index)
{
    modelIndex = index;

    switch (index)
    {
    case 0:  // taylor
    {
        taylorlegendre_form_->setVisible(true);
        polyhedral_form_->setVisible(false);
        spline_form_->setVisible(false);
        compress_form_->setVisible(false);
        lssvmpso_form_->setVisible(false);
        ui->widget_pic0->setVisible(true);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widget_pic4->setVisible(false);
        ui->widget_pic5->setVisible(false);
        break;
    }
    case 1:  // legendre
    {
        taylorlegendre_form_->setVisible(true);
        polyhedral_form_->setVisible(false);
        spline_form_->setVisible(false);
        compress_form_->setVisible(false);
        lssvmpso_form_->setVisible(false);
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(true);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widget_pic4->setVisible(false);
        ui->widget_pic5->setVisible(false);
        break;
    }
    case 2:  // polyhedral
    {
        taylorlegendre_form_->setVisible(false);
        polyhedral_form_->setVisible(true);
        spline_form_->setVisible(false);
        compress_form_->setVisible(false);
        lssvmpso_form_->setVisible(false);
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(true);
        ui->widget_pic3->setVisible(false);
        ui->widget_pic4->setVisible(false);
        ui->widget_pic5->setVisible(false);
        break;
    }
    case 3:  // spline
    {
        taylorlegendre_form_->setVisible(false);
        polyhedral_form_->setVisible(false);
        spline_form_->setVisible(true);
        compress_form_->setVisible(false);
        lssvmpso_form_->setVisible(false);
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(true);
        ui->widget_pic4->setVisible(false);
        ui->widget_pic5->setVisible(false);
        break;
    }
    case 4:  // compress
    {
        taylorlegendre_form_->setVisible(false);
        polyhedral_form_->setVisible(false);
        spline_form_->setVisible(false);
        compress_form_->setVisible(true);
        lssvmpso_form_->setVisible(false);
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widget_pic4->setVisible(true);
        ui->widget_pic5->setVisible(false);
        break;
    }
    case 5:  // lssvmpso
    {
        taylorlegendre_form_->setVisible(false);
        polyhedral_form_->setVisible(false);
        spline_form_->setVisible(false);
        compress_form_->setVisible(false);
        lssvmpso_form_->setVisible(true);
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widget_pic4->setVisible(false);
        ui->widget_pic5->setVisible(true);
        break;
    }
    default:
    {
        taylorlegendre_form_->setVisible(false);
        polyhedral_form_->setVisible(false);
        spline_form_->setVisible(false);
        compress_form_->setVisible(false);
        lssvmpso_form_->setVisible(false);
        ui->widget_pic0->setVisible(true);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widget_pic4->setVisible(false);
        ui->widget_pic5->setVisible(false);
    }
    }
}

double ReferenceMap::calculateRMS(const Geomagnetic::Datapoint &datapoints,
                                const Geomagnetic::Datapoint &dataresults)
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

    if (count == 0) return 0.0;
    return std::sqrt(sumSquaredError / count);
}


void ReferenceMap::on_pushButton_clicked()
{
    sparse_para = this->ui->doubleSpinBox_sparse->value();
    dx = this->ui->doubleSpinBox_dx->value();
    dy = this->ui->doubleSpinBox_dy->value();
    switch (modelIndex)
    {
    case 0: // taylor
    {
        auto start = std::chrono::high_resolution_clock::now(); // 获取当前时间点

        QLayout *lay = ui->widget_pic0->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0))!=nullptr)
            {
                delete item->widget();
                delete item;
            }
        }
        int para0 = taylorlegendre_form_->getspinbox();
        int data_index = ui->comboBox_data->currentIndex();
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        // 设置参数
        Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
        Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
        Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
        Geomagnetic::ReadData readdata;
        Geomagnetic::Datainfo datainfo;

        readdata.readGridFromFile(str,datapoints);
        datainfo.Cutoff = para0;
        readdata.interval = sparse_para * 10;

        Geomagnetic::TaylorModel taylor;
        std::string s = projectPath.toStdString()+"/Processed/"+filename.toStdString();
        readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
        datapoint_result = datapoints;
        readdata.DataSet(datapoint_sparse, datainfo);
        for (auto& elem : datapoint_result)
            elem.second.tMagnetic = 0;
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
        ui->textBrowser->append("计算完成！");
        QCoreApplication::processEvents();
        auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
        std::chrono::duration<double> elapsed = end - start;
        QString outstr = "处理时间: " + QString::number(elapsed.count()) + "s";
        ui->textBrowser->append(outstr+"\n");
        ui->textBrowser->append("结果正在保存与输出...");
        QCoreApplication::processEvents();
        readdata.resultOut(datapoint_result, s);
        QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
        double rms = calculateRMS(datapoints,datapoint_result);
        ui->textBrowser->append("基于泰勒多项式方法结果计算完成\n");
        ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        // 更新主界面的树
        emit treeUpdated(1);
        //
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
        draw_form_->set_heatMapView(xx,yy,zz);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->set_ContourView(QString::fromStdString(s));
        ui->widget_pic0->layout()->addWidget(draw_form_);



        break;
    }
    case 1: // legendre
    {
        QLayout *lay = ui->widget_pic1->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0))!=nullptr)
            {
                delete item->widget();
                delete item;
            }
        }
        int para1 = taylorlegendre_form_->getspinbox();
        int data_index = ui->comboBox_data->currentIndex();
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        // 设置参数
        Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
        Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
        Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
        Geomagnetic::ReadData readdata;
        Geomagnetic::Datainfo datainfo;

        readdata.readGridFromFile(str,datapoints);
        readdata.interval = sparse_para * 10;

        Geomagnetic::LegendreModel legendre;
        std::string s = projectPath.toStdString()+"/Processed/"+filename.toStdString();
        datainfo.N = para1;
        readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
        datapoint_result = datapoints;
        readdata.DataSet(datapoint_sparse, datainfo);
        for (auto& elem : datapoint_result)
            elem.second.tMagnetic = 0;
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
        if(draw_form_->magWarn == false)
            return;
        draw_form_->set_ContourView(QString::fromStdString(s));
        ui->widget_pic1->layout()->addWidget(draw_form_);
        // 更新主界面的树
        emit treeUpdated(1);
        break;
    }
    case 2: // polyhedral
    {
        QLayout *lay = ui->widget_pic2->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0))!=nullptr)
            {
                delete item->widget();
                delete item;
            }
        }
        int paraModel = polyhedral_form_->getModelType();
        double para2 = polyhedral_form_->getdoubleSpinBoxPara();
        int data_index = ui->comboBox_data->currentIndex();
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (paraModel == -1)
        {
            QMessageBox::warning(nullptr, "错误", "未选择函数!");
            return;
        }
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }
        // 设置参数
        Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
        Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
        Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
        Geomagnetic::ReadData readdata;
        Geomagnetic::Datainfo datainfo;

        readdata.readGridFromFile(str,datapoints);
        readdata.interval = sparse_para * 10;

        Geomagnetic::Polyhedral poly;
        std::string s=projectPath.toStdString()+"/Processed/"+filename.toStdString();
        datainfo.PolyQ = paraModel;
        datainfo.sigma2 = para2;
        readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
        datapoint_result = datapoints;
        readdata.DataSet(datapoint_sparse, datainfo);
        for (auto& elem : datapoint_result)
            elem.second.tMagnetic = 0;
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
        if(draw_form_->magWarn == false)
            return;
        draw_form_->set_ContourView(QString::fromStdString(s));
        ui->widget_pic2->layout()->addWidget(draw_form_);
        //
        QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
        ui->textBrowser->append("基于多面函数方法结果计算完成\n");
        QCoreApplication::processEvents();
        double rms = calculateRMS(datapoints,datapoint_result);
        ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        // 更新主界面的树
        emit treeUpdated(1);
        break;
    }
    case 3: // spline
    {
        QLayout *lay = ui->widget_pic3->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0))!=nullptr)
            {
                delete item->widget();
                delete item;
            }
        }
        double para3_0 = spline_form_->getdoubleSpinBox();
        int para3_1 = spline_form_->getspinBox();
        int data_index = ui->comboBox_data->currentIndex();
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        // 设置参数
        Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
        Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
        Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
        Geomagnetic::ReadData readdata;
        Geomagnetic::Datainfo datainfo;

        readdata.readGridFromFile(str,datapoints);
        readdata.interval = sparse_para * 10;
        Geomagnetic::Splinecurve spline;
        std::string s=projectPath.toStdString()+"/Processed/"+filename.toStdString();

        datainfo.E= para3_0;
        datainfo.C = para3_1;
        readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
        datapoint_result = datapoints;
        readdata.DataSet(datapoint_sparse, datainfo);
        for (auto& elem : datapoint_result)
            elem.second.tMagnetic = 0;
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
        if(draw_form_->magWarn == false)
            return;
        draw_form_->set_ContourView(QString::fromStdString(s));
        ui->widget_pic3->layout()->addWidget(draw_form_);
        //
        QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
        ui->textBrowser->append("基于样条曲线方法结果计算完成");
        QCoreApplication::processEvents();
        double rms = calculateRMS(datapoints,datapoint_result);
        ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        // 更新主界面的树
        emit treeUpdated(1);
        break;
    }
    case 4: // compress
    {
        QLayout *lay = ui->widget_pic4->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0))!=nullptr)
            {
                delete item->widget();
                delete item;
            }
        }
        int para4 = compress_form_->getspinBox();
        int data_index = ui->comboBox_data->currentIndex();
        QString filename = ui->lineEdit_savePath->text();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        int sampling_factor = sparse_para;
        QString datafile = projectPath+"/Measured/"+nameList_real[data_index];
        QString out_dir = projectPath+"/Processed";
        ReconstructionManager manager;
        ui->textBrowser->append("基于压缩感知方法结果计算开始...");
        QCoreApplication::processEvents();
        bool result = manager.processData(datafile, para4, sampling_factor, out_dir, filename);
        if (!result)
        {
            QMessageBox::warning(this, "Failure", "Geophysical reconstruction failed.");
            return;
        }
        // QString s = projectPath+"/Processed/"+filename;
        // QString script_path = "/home/greatwall/whumag/cs.py";
        // QString pythonPath = "/home/greatwall/mag/bin/python";
        // QString command = QString("%1 %2 %3 %4 %5 %6 %7")
        //                         .arg(pythonPath)
        //                         .arg(script_path)
        //                         .arg(datafile)
        //                         .arg(para4)
        //                         .arg(sampling_factor)
        //                         .arg(out_dir)
        //                         .arg(filename);
        // int result = system(command.toStdString().c_str());
        else
        {
            ui->textBrowser->append("基于压缩感知方法结果计算完成");
            QCoreApplication::processEvents();;
        }
        QString s = projectPath+"/Processed/"+filename;
        // bool flag = manager.saveResults(s.toStdString());
        // //
        // Geomagnetic::Datapoint datapoints1,datapoints2;
        // Geomagnetic::ReadData readdata1,readdata2;
        // readdata1.readGridFromFile(datafile.toStdString(),datapoints1);
        // readdata2.readGridFromFile(s.toStdString(),datapoints2);
        // double rms = calculateRMS(datapoints1,datapoints2);
        // ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        ui->textBrowser->append("RMS:" + QString::number(manager.RMS));
        QCoreApplication::processEvents();
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        draw_form_->create_xyz_f(s,xx,yy,zz);
        draw_form_->set_heatMapView(xx,yy,zz);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->set_ContourView(s);
        ui->widget_pic4->layout()->addWidget(draw_form_);
        // 更新主界面的树
        emit treeUpdated(1);
        break;
    }
    case 5: // lssvmpso
    {
        QLayout *lay = ui->widget_pic5->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0))!=nullptr)
            {
                delete item->widget();
                delete item;
            }
        }
        int p1, p8;
        double p2, p3, p4, p5, p6, p7, p9, p10, p11;
        int ret = lssvmpso_form_->getPara(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11);
        int data_index = ui->comboBox_data->currentIndex();
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        // 设置参数
        Geomagnetic::Datapoint datapoints;
        Geomagnetic::ReadData readdata;

        readdata.readGridFromFile(str,datapoints);
        QString s = projectPath+"/Processed/"+filename;
        Geomagnetic::LSSVMPSO lssvmpso;
        lssvmpso.readPara(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11);
        readdata.interval = sparse_para *10;
        Geomagnetic::Datapoint alldatapoint;
        Geomagnetic::Datapoint Traindatapoint;
        readdata.selectLineData(datapoints, Traindatapoint, alldatapoint, readdata.interval); // 10\20 50
        ui->textBrowser->append("数据加载完毕...");
        QCoreApplication::processEvents();
        ui->textBrowser->append("开始迭代计算...");
        QCoreApplication::processEvents();
        lssvmpso.run(alldatapoint,Traindatapoint,s);
        ui->textBrowser->append("rms: "+QString::number(lssvmpso.rms,'f',2));
        //
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        draw_form_->create_xyz_f(s,xx,yy,zz);
        draw_form_->set_heatMapView(xx,yy,zz);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->set_ContourView(s);
        ui->widget_pic5->layout()->addWidget(draw_form_);
        ui->textBrowser->append("计算完成！");
        QCoreApplication::processEvents();

        // 更新主界面的树
        emit treeUpdated(1);
        break;
    }
    default:
        QMessageBox::information(nullptr, "提醒", "请选择基准图插值模型。",
                                     QMessageBox::Ok);
    }
}



