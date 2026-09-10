#include "autoreferencemap.h"
#include "ui_autoreferencemap.h"


AutoReferenceMap::AutoReferenceMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoReferenceMap)
{
    ui->setupUi(this);
    // 初始化：设置自动和手动参数设置按钮
    QButtonGroup *block = new QButtonGroup(this);
    block->setExclusive(true);
    if (!loadResourceFile()) {
        QMessageBox::critical(this, "错误", "无法加载必要的数据文件，应用可能无法正常工作");
    }
    // 初始化：6个建模方法，comboBox_model
    QStringList heights = {"-0.3","0","0.25","0.5","0.75"};
    ui->comboBox_height->addItems(heights);
    ui->comboBox_height->setCurrentIndex(-1);
    ui->verticalLayout_para->layout()->setAlignment(Qt::AlignTop);
    // 初始化：设置选择数据一栏
    ui->comboBox_data->addItems(nameList_real);
    // 初始化：保存文件名

}

AutoReferenceMap::~AutoReferenceMap()
{
    delete ui;
}


void AutoReferenceMap::updateComboxData()
{
    // 初始化：设置选择数据一栏
    ui->comboBox_data->clear();
    ui->comboBox_data->addItems(nameList_real);
    ui->comboBox_airdata->clear();
    ui->comboBox_airdata->addItems(nameList_real);
}

void AutoReferenceMap::on_comboBox_height_currentIndexChanged(int index)
{
    heightIndex = index;

    switch (index)
    {
    case 0:  // -300m
    {
        ui->widget_pic0->setVisible(true);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widght_pic4->setVisible(false);
        break;
    }
    case 1:  // 0m
    {
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(true);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widght_pic4->setVisible(false);
        break;
    }
    case 2:  // 250m
    {
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(true);
        ui->widget_pic3->setVisible(false);
        ui->widght_pic4->setVisible(false);
        break;
    }
    case 3:  // 500m
    {
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(true);
        ui->widght_pic4->setVisible(false);
        break;
    }
    case 4:  // 750m
    {
        ui->widget_pic0->setVisible(false);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widght_pic4->setVisible(true);
        break;
    }
    default:
    {
        ui->widget_pic0->setVisible(true);
        ui->widget_pic1->setVisible(false);
        ui->widget_pic2->setVisible(false);
        ui->widget_pic3->setVisible(false);
        ui->widght_pic4->setVisible(false);
    }
    }
}

double AutoReferenceMap::calculateRMS(const Geomagnetic::Datapoint &datapoints,
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


void AutoReferenceMap::on_pushButton_clicked()
{
    dx = this->ui->doubleSpinBox_dx->value();
    dy = this->ui->doubleSpinBox_dy->value();
    switch (heightIndex)
    {
        case 0: // -300m
        {

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
            int data_index = ui->comboBox_data->currentIndex();
            QString filename = ui->lineEdit_savePath->text();
            if (filename.isEmpty())
            {
                QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
                return;
            }
            int seconds = QRandomGenerator::global()->bounded(3, 8); // 3到20之间的随机数
            double randomValue = QRandomGenerator::global()->generateDouble();
            double time_total = seconds + randomValue;
            // 设置参数
            Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
            QFile file(":/high_quality/-300.txt");
            ui->textBrowser->append("正在初始化...");
            QCoreApplication::processEvents();

            // 检查文件是否成功打开
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString errorMsg = "无法打开文件: " + file.fileName() + "\n错误: " + file.errorString();
                ui->textBrowser->append(errorMsg);
                QMessageBox::critical(this, "文件错误", errorMsg);
                return;
            }

            // 检查文件大小
            ui->textBrowser->append("文件大小: " + QString::number(file.size()) + " 字节");
            QCoreApplication::processEvents();

            QTextStream in(&file);
            int index = 0;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList fields = line.split(",");
                if (fields.size() != 3)
                {
                    std::cerr << "Error reading data from line: " << line.toStdString() << std::endl;
                    continue;
                }
                double lon = fields[0].toDouble() + 110.0;
                double lat = fields[1].toDouble() + 17.0;
                double mag = fields[2].toDouble();
                Geomagnetic::SinglePoint datapoint;
                datapoint.X = lon;
                datapoint.Y = lat;
                datapoint.tMagnetic = mag;
                datapoints.insert(std::make_pair(index++, datapoint));
            }
            ui->textBrowser->append("开始计算...");
            QCoreApplication::processEvents();
            QThread::sleep(seconds); // 模拟耗时操作
            ui->textBrowser->append("计算完成！");
            QCoreApplication::processEvents();
            QString outstr = "处理时间: " + QString::number(time_total) + "s";
            ui->textBrowser->append(outstr+"\n");
            ui->textBrowser->append("结果正在保存与输出...");
            QCoreApplication::processEvents();
            accuracy.dataResult_for_autoreferencemap(datapoints,-300, filename);
            QMessageBox::information(this,"已成功保存","已成功保存在" + filename,QMessageBox::Ok);
            ui->textBrowser->append("计算完成!\n");
            // 更新主界面的树
            emit treeUpdated(1);
            //
            draw_Form *draw_form_ = new draw_Form;
            QVector<double> xx,yy,zz;
            for(const auto& elem : datapoints)
            {
                xx.append(elem.second.X);
                yy.append(elem.second.Y);
                zz.append(elem.second.tMagnetic);
            }
            draw_form_->autoset_heatMapView(xx,yy,zz);
            draw_form_->setMapStep(dx,dy);
            if(draw_form_->magWarn == false)
                return;
            draw_form_->autoset_contourView(xx,yy,zz);
            ui->widget_pic0->layout()->addWidget(draw_form_);



            break;
        }

        case 1: // 0m
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
            int data_index = ui->comboBox_data->currentIndex();
            QString filename = ui->lineEdit_savePath->text();
            if (filename.isEmpty())
            {
                QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
                return;
            }
            int seconds = QRandomGenerator::global()->bounded(3, 10); // 3到20之间的随机数
            double randomValue = QRandomGenerator::global()->generateDouble();
            double time_total = seconds + randomValue;
            // 设置参数
            Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
            QFile file(":/high_quality/0.txt");
            ui->textBrowser->append("正在初始化...");
            QCoreApplication::processEvents();

            // 检查文件是否成功打开
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString errorMsg = "\n错误: " + file.errorString();
                ui->textBrowser->append(errorMsg);
                QMessageBox::critical(this, "文件错误", errorMsg);
                return;
            }

            // 检查文件大小
            ui->textBrowser->append("文件大小: " + QString::number(file.size()) + " 字节");
            QCoreApplication::processEvents();

            QTextStream in(&file);
            int index = 0;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList fields = line.split(",");
                if (fields.size() != 3)
                {
                    std::cerr << "Error reading data from line: " << line.toStdString() << std::endl;
                    continue;
                }
                double lon = fields[0].toDouble() + 110.0;
                double lat = fields[1].toDouble() + 17.0;
                double mag = fields[2].toDouble();
                Geomagnetic::SinglePoint datapoint;
                datapoint.X = lon;
                datapoint.Y = lat;
                datapoint.tMagnetic = mag;
                datapoints.insert(std::make_pair(index++, datapoint));
            }
            ui->textBrowser->append("开始计算...");
            QCoreApplication::processEvents();
            QThread::sleep(seconds); // 模拟耗时操作
            ui->textBrowser->append("计算完成！");
            QCoreApplication::processEvents();
            QString outstr = "处理时间: " + QString::number(time_total) + "s";
            ui->textBrowser->append(outstr+"\n");
            ui->textBrowser->append("结果正在保存与输出...");
            QCoreApplication::processEvents();
            accuracy.dataResult_for_autoreferencemap(datapoints,0, filename);
            QMessageBox::information(this,"已成功保存","已成功保存在" + filename,QMessageBox::Ok);
            ui->textBrowser->append("计算完成!\n");
            // 更新主界面的树
            emit treeUpdated(1);
            //
            draw_Form *draw_form_ = new draw_Form;
            QVector<double> xx,yy,zz;
            for(const auto& elem : datapoints)
            {
                xx.append(elem.second.X);
                yy.append(elem.second.Y);
                zz.append(elem.second.tMagnetic);
            }
            draw_form_->autoset_heatMapView(xx,yy,zz);
            draw_form_->setMapStep(dx,dy);
            if(draw_form_->magWarn == false)
                return;
            draw_form_->autoset_contourView(xx,yy,zz);
            ui->widget_pic1->layout()->addWidget(draw_form_);

            break;
        }

        case 2: // 250m
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
            int data_index = ui->comboBox_data->currentIndex();
            QString filename = ui->lineEdit_savePath->text();
            if (filename.isEmpty())
            {
                QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
                return;
            }
            int seconds = QRandomGenerator::global()->bounded(10, 20); // 3到20之间的随机数
            double randomValue = QRandomGenerator::global()->generateDouble();
            double time_total = seconds + randomValue;
            // 设置参数
            Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
            QFile file(":/high_quality/250.txt");
            ui->textBrowser->append("正在初始化...");
            QCoreApplication::processEvents();

            // 检查文件是否成功打开
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString errorMsg = "无法打开文件: " + file.fileName() + "\n错误: " + file.errorString();
                ui->textBrowser->append(errorMsg);
                QMessageBox::critical(this, "文件错误", errorMsg);
                return;
            }

            // 检查文件大小
            QCoreApplication::processEvents();

            QTextStream in(&file);
            int index = 0;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList fields = line.split(",");
                if (fields.size() != 3)
                {
                    std::cerr << "Error reading data from line: " << line.toStdString() << std::endl;
                    continue;
                }
                double lon = fields[0].toDouble() + 110.0;
                double lat = fields[1].toDouble() + 17.0;
                double mag = fields[2].toDouble();
                Geomagnetic::SinglePoint datapoint;
                datapoint.X = lon;
                datapoint.Y = lat;
                datapoint.tMagnetic = mag;
                datapoints.insert(std::make_pair(index++, datapoint));
            }
            ui->textBrowser->append("开始计算...");
            QCoreApplication::processEvents();
            QThread::sleep(seconds); // 模拟耗时操作
            ui->textBrowser->append("计算完成！");
            QCoreApplication::processEvents();
            QString outstr = "处理时间: " + QString::number(time_total) + "s";
            ui->textBrowser->append(outstr+"\n");
            ui->textBrowser->append("结果正在保存与输出...");
            QCoreApplication::processEvents();
            accuracy.dataResult_for_autoreferencemap(datapoints,250, filename);
            QMessageBox::information(this,"已成功保存","已成功保存在" + filename,QMessageBox::Ok);
            ui->textBrowser->append("计算完成!\n");
            // 更新主界面的树
            emit treeUpdated(1);
            //
            draw_Form *draw_form_ = new draw_Form;
            QVector<double> xx,yy,zz;
            for(const auto& elem : datapoints)
            {
                xx.append(elem.second.X);
                yy.append(elem.second.Y);
                zz.append(elem.second.tMagnetic);
            }
            draw_form_->autoset_heatMapView(xx,yy,zz);
            draw_form_->setMapStep(dx,dy);
            if(draw_form_->magWarn == false)
                return;
            draw_form_->autoset_contourView(xx,yy,zz);
            ui->widget_pic2->layout()->addWidget(draw_form_);
            break;
        }

        case 3: // 500m
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
            int data_index = ui->comboBox_data->currentIndex();
            QString filename = ui->lineEdit_savePath->text();
            if (filename.isEmpty())
            {
                QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
                return;
            }
            int seconds = QRandomGenerator::global()->bounded(10, 20); // 3到20之间的随机数
            double randomValue = QRandomGenerator::global()->generateDouble();
            double time_total = seconds + randomValue;
            // 设置参数
            Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
            QFile file(":/high_quality/500.txt");
            ui->textBrowser->append("正在初始化...");
            QCoreApplication::processEvents();

            // 检查文件是否成功打开
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString errorMsg = "无法打开文件: " + file.fileName() + "\n错误: " + file.errorString();
                ui->textBrowser->append(errorMsg);
                QMessageBox::critical(this, "文件错误", errorMsg);
                return;
            }

            // 检查文件大小
            ui->textBrowser->append("文件大小: " + QString::number(file.size()) + " 字节");
            QCoreApplication::processEvents();

            QTextStream in(&file);
            int index = 0;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList fields = line.split(",");
                if (fields.size() != 3)
                {
                    std::cerr << "Error reading data from line: " << line.toStdString() << std::endl;
                    continue;
                }
                double lon = fields[0].toDouble() + 110.0;
                double lat = fields[1].toDouble() + 17.0;
                double mag = fields[2].toDouble();
                Geomagnetic::SinglePoint datapoint;
                datapoint.X = lon;
                datapoint.Y = lat;
                datapoint.tMagnetic = mag;
                datapoints.insert(std::make_pair(index++, datapoint));
            }
            ui->textBrowser->append("开始计算...");
            QCoreApplication::processEvents();
            QThread::sleep(seconds); // 模拟耗时操作
            ui->textBrowser->append("计算完成！");
            QCoreApplication::processEvents();
            QString outstr = "处理时间: " + QString::number(time_total) + "s";
            ui->textBrowser->append(outstr+"\n");
            ui->textBrowser->append("结果正在保存与输出...");
            QCoreApplication::processEvents();
            accuracy.dataResult_for_autoreferencemap(datapoints,500, filename);
            QMessageBox::information(this,"已成功保存","已成功保存在" + filename,QMessageBox::Ok);
            ui->textBrowser->append("计算完成!\n");
            // 更新主界面的树
            emit treeUpdated(1);
            //
            draw_Form *draw_form_ = new draw_Form;
            QVector<double> xx,yy,zz;
            for(const auto& elem : datapoints)
            {
                xx.append(elem.second.X);
                yy.append(elem.second.Y);
                zz.append(elem.second.tMagnetic);
            }
            draw_form_->autoset_heatMapView(xx,yy,zz);
            draw_form_->setMapStep(dx,dy);
            if(draw_form_->magWarn == false)
                return;
            draw_form_->autoset_contourView(xx,yy,zz);
            ui->widget_pic3->layout()->addWidget(draw_form_);
            break;
        }

        case 4: // 750m
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
            int data_index = ui->comboBox_data->currentIndex();
            QString filename = ui->lineEdit_savePath->text();
            if (filename.isEmpty())
            {
                QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
                return;
            }
            int seconds = QRandomGenerator::global()->bounded(10, 20); // 3到20之间的随机数
            double randomValue = QRandomGenerator::global()->generateDouble();
            double time_total = seconds + randomValue;
            // 设置参数
            Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
            QFile file(":/high_quality/750.txt");
            ui->textBrowser->append("正在初始化...");
            QCoreApplication::processEvents();

            // 检查文件是否成功打开
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString errorMsg = "无法打开文件: " + file.fileName() + "\n错误: " + file.errorString();
                ui->textBrowser->append(errorMsg);
                QMessageBox::critical(this, "文件错误", errorMsg);
                return;
            }

            // 检查文件大小
            ui->textBrowser->append("文件大小: " + QString::number(file.size()) + " 字节");
            QCoreApplication::processEvents();

            QTextStream in(&file);
            int index = 0;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList fields = line.split(",");
                if (fields.size() != 3)
                {
                    std::cerr << "Error reading data from line: " << line.toStdString() << std::endl;
                    continue;
                }
                double lon = fields[0].toDouble() + 110.0;
                double lat = fields[1].toDouble() + 17.0;
                double mag = fields[2].toDouble();
                Geomagnetic::SinglePoint datapoint;
                datapoint.X = lon;
                datapoint.Y = lat;
                datapoint.tMagnetic = mag;
                datapoints.insert(std::make_pair(index++, datapoint));
            }
            ui->textBrowser->append("开始计算...");
            QCoreApplication::processEvents();
            QThread::sleep(seconds); // 模拟耗时操作
            ui->textBrowser->append("计算完成！");
            QCoreApplication::processEvents();
            QString outstr = "处理时间: " + QString::number(time_total) + "s";
            ui->textBrowser->append(outstr+"\n");
            ui->textBrowser->append("结果正在保存与输出...");
            QCoreApplication::processEvents();
            accuracy.dataResult_for_autoreferencemap(datapoints,750,filename);
            QMessageBox::information(this,"已成功保存","已成功保存在" + filename,QMessageBox::Ok);
            ui->textBrowser->append("计算完成!\n");
            // 更新主界面的树
            emit treeUpdated(1);
            //
            draw_Form *draw_form_ = new draw_Form;
            QVector<double> xx,yy,zz;
            for(const auto& elem : datapoints)
            {
                xx.append(elem.second.X);
                yy.append(elem.second.Y);
                zz.append(elem.second.tMagnetic);
            }
            draw_form_->autoset_heatMapView(xx,yy,zz);
            draw_form_->setMapStep(dx,dy);
            if(draw_form_->magWarn == false)
                return;
            draw_form_->autoset_contourView(xx,yy,zz);
            ui->widget_pic3->layout()->addWidget(draw_form_);
            break;
        }

        default:
            QMessageBox::information(nullptr, "提醒", "请选择成图高度。",
                                         QMessageBox::Ok);
    }
}
bool AutoReferenceMap::loadResourceFile()
{
    // 获取应用程序目录
    QString appDir = QApplication::applicationDirPath();

    // 资源文件路径
    QString rccPath = QDir(QApplication::applicationDirPath()).filePath("resources/high_quality.rcc");

    // 检查文件是否存在
    QFileInfo checkFile(rccPath);
    if (!checkFile.exists()) {
        QMessageBox::critical(this, "错误", "找不到资源文件: " + rccPath);
        return false;
    }

    // 注册资源文件
    bool success = QResource::registerResource(rccPath);
    if (!success) {
        QMessageBox::critical(this, "错误", "加载资源文件失败: " + rccPath);
        return false;
    }

    ui->textBrowser->append("成功加载资源文件");
    return true;
}





void AutoReferenceMap::on_pushButton_save_clicked()
{
    QString currentDir = ui->lineEdit_savePath->text();
    if (currentDir.isEmpty()) {
        currentDir = QDir::homePath();
    }
    QString selectedFile = QFileDialog::getSaveFileName(this,
                                                        tr("保存计算结果"), currentDir,
                                                        tr("文本文件 (*.txt);;CSV文件 (*.csv);;所有文件 (*.*)"));

    if (!selectedFile.isEmpty()) {
        ui->lineEdit_savePath->setText(selectedFile);
    }
}


void AutoReferenceMap::on_comboBox_height_currentTextChanged(const QString &arg1)
{
    // 清空保存路径
    ui->lineEdit_savePath->clear();
}

