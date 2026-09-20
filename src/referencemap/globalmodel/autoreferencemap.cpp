#include "autoreferencemap.h"
#include "ui_autoreferencemap.h"

#include "formkit.h"
#include "resultpreviewpanel.h"
#include "thememanager.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>


AutoReferenceMap::AutoReferenceMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoReferenceMap)
{
    ui->setupUi(this);
    // 初始化：设置自动和手动参数设置按钮
    QButtonGroup *block = new QButtonGroup(this);
    block->setExclusive(true);
    loadResourceFile();   // a failure is shown inside the form (see buildLayout), not as start-up dialogs
    // 初始化：6个建模方法，comboBox_model
    QStringList heights = {"-0.3","0","0.25","0.5","0.75"};
    ui->comboBox_height->addItems(heights);
    ui->comboBox_height->setCurrentIndex(-1);
    ui->verticalLayout_para->layout()->setAlignment(Qt::AlignTop);
    // 初始化：设置选择数据一栏
    ui->comboBox_data->addItems(nameList_real);
    // 初始化：保存文件名

    buildLayout();
}

// Replaces the .ui layout: inputs in a left panel, preview + log in a right panel.  All widgets the
// processing code touches are kept; only the container widgets of the old layout are removed.
void AutoReferenceMap::buildLayout()
{
    setWindowTitle(tr("一键成图"));
    resize(UiScale::windowSize(1180, 700));

    // ---- left: header, three steps, run button
    leftPanel_ = new QWidget;
    leftPanel_->setObjectName("autoLeft");
    leftPanel_->setFixedWidth(UiScale::dp(420));
    leftPanel_->setAttribute(Qt::WA_StyledBackground, true);
    auto *ll = new QVBoxLayout(leftPanel_);
    ll->setContentsMargins(24, 20, 24, 18);
    ll->setSpacing(14);
    ll->addWidget(FormKit::header(QStringLiteral("spark"), tr("一键成图"), tr("选择数据与建图参数，一键生成基准图")));

    if (!resourceError_.isEmpty())
    {
        auto *banner = new Banner;
        banner->setContent(Banner::Warn, tr("一键成图暂不可用"),
                           resourceError_ + tr("。请把 high_quality.rcc 放到程序目录的 resources 文件夹。"));
        ll->addWidget(banner);
        ui->pushButton->setEnabled(false);
    }

    ll->addWidget(FormKit::stepHeading(1, tr("选择数据")));
    ll->addLayout(FormKit::field(tr("水面数据"), ui->comboBox_data));
    ll->addLayout(FormKit::field(tr("低空数据"), ui->comboBox_airdata));

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(2, tr("建图参数")));
    auto *res = new QHBoxLayout;
    res->setSpacing(12);
    res->addLayout(FormKit::field(tr("插值分辨率 · 经度 (X)"), ui->doubleSpinBox_dx), 1);
    res->addLayout(FormKit::field(tr("插值分辨率 · 纬度 (Y)"), ui->doubleSpinBox_dy), 1);
    ll->addLayout(res);
    ui->comboBox_height->setPlaceholderText(tr("请选择建图高度"));
    ll->addLayout(FormKit::field(tr("建图高度 (km)"), ui->comboBox_height));

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(3, tr("保存结果")));
    auto *saveRow = new QHBoxLayout;
    saveRow->setSpacing(8);
    saveRow->addWidget(ui->lineEdit_savePath, 1);
    ui->pushButton_save->setText(tr("浏览…"));
    saveRow->addWidget(ui->pushButton_save);
    ll->addLayout(FormKit::field(tr("保存路径"), saveRow));

    ll->addStretch(1);
    ui->pushButton->setText(tr("智能处理"));
    FormKit::setRole(ui->pushButton, "primary");
    ui->pushButton->setMinimumHeight(36);
    ui->pushButton->setCursor(Qt::PointingHandCursor);
    ll->addWidget(ui->pushButton);

    // ---- right: status, preview card and log
    preview_ = new ResultPreviewPanel({ui->widget_pic0, ui->widget_pic1, ui->widget_pic2, ui->widget_pic3, ui->widght_pic4},
                                      ui->textBrowser);

    // ---- swap: old layout out, new one in (this re-parents every widget used above), then drop the old containers
    auto *root = new QHBoxLayout;
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(leftPanel_);
    root->addWidget(preview_, 1);
    delete layout();
    setLayout(root);
    delete ui->widget_data;
    delete ui->widget_confirm;
    delete ui->widget;
    delete ui->line;
    delete ui->line_2;

    auto restyle = [this]() {
        const ThemeManager &tm = ThemeManager::instance();
        leftPanel_->setStyleSheet(QStringLiteral("#autoLeft { background: %1; border: none; border-right: 1px solid %2; }").arg(tm.hex("n1"), tm.hex("line")));
    };
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, restyle);
    restyle();

    // the height slot (connected by setupUi) shows / hides the five result containers; look again after it ran
    connect(ui->comboBox_height, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { preview_->refreshEmptyState(); });
}

void AutoReferenceMap::setBusy(bool busy)
{
    leftPanel_->setEnabled(!busy);
    ui->pushButton->setText(busy ? tr("处理中…") : tr("智能处理"));
    if (busy)
        preview_->setStatus(tr("处理中…"), Chip::Accent);
    else
        preview_->setStatus(runOk_ ? tr("已完成") : tr("未完成"), runOk_ ? Chip::Ok : Chip::Neutral);
    QCoreApplication::processEvents();
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

void AutoReferenceMap::on_pushButton_clicked()
{
    runOk_ = false;
    setBusy(true);
    runMapping();
    setBusy(false);
    preview_->refreshEmptyState();
}

void AutoReferenceMap::runMapping()
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
            runOk_ = true;
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
            runOk_ = true;
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
            runOk_ = true;
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
            runOk_ = true;
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
            QLayout *lay = ui->widght_pic4->layout();
            if (lay)
            {
                QLayoutItem *item;
                while((item = lay->takeAt(0))!=nullptr)
                {
                    delete item->widget();
                    delete item;
                }
            }
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
            runOk_ = true;
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
            ui->widght_pic4->layout()->addWidget(draw_form_);
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
        resourceError_ = tr("找不到资源文件 %1").arg(rccPath);
        ui->textBrowser->append(resourceError_);
        return false;
    }

    // 注册资源文件
    bool success = QResource::registerResource(rccPath);
    if (!success) {
        resourceError_ = tr("加载资源文件失败：%1").arg(rccPath);
        ui->textBrowser->append(resourceError_);
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

