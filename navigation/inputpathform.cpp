#include "inputpathform.h"
#include "ui_inputpathform.h"
#include <QFileDialog>
#include <QMessageBox>
#include "navigation/iccp.h"
#include "navigation/tercom.h"
#include "navigation/sitan.h"
#include "navigation/autonav.h"

InputPathForm::InputPathForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InputPathForm)
{
    ui->setupUi(this);
    ui->comboBox->addItem("TERCOM");
    ui->comboBox->addItem("ICCP");
    ui->comboBox->addItem("SITAN");
    ui->comboBox->addItem("TERCOM与ICCP联合");
    ui->comboBox->addItem("自动处理");
}

InputPathForm::~InputPathForm()
{
    delete ui;
}

void InputPathForm::on_pushButton_backG_clicked()
{
    // 选择背景文件
    QString tmp = QFileDialog::getOpenFileName(this, tr("请打开背景场文件"),
                                                 QCoreApplication::applicationFilePath(),"*.*");
    ui->lineEdit_backG->setText(tmp);
}

void InputPathForm::on_pushButton_INS_clicked()
{
    // 选择INS文件
    QString tmp = QFileDialog::getOpenFileName(this, tr("请打开INS文件"),
                                               QCoreApplication::applicationFilePath(),"*.*");
    ui->lineEdit_INS->setText(tmp);
}

void InputPathForm::on_pushButton_real_clicked()
{
    // 选择真实航迹文件
    QString tmp = QFileDialog::getOpenFileName(this, tr("请打开INS文件"),
                                               QCoreApplication::applicationFilePath(),"*.*");
    ui->lineEdit_real->setText(tmp);
}

void InputPathForm::on_pushButton_out_clicked()
{
    outputPath = QFileDialog::getExistingDirectory(this, tr("选择保存文件夹"),
                                                          QCoreApplication::applicationDirPath(),
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!outputPath.isEmpty())
    {
        ui->lineEdit_out->setText(outputPath);
    }
}


int InputPathForm::check()
{
    // 判别背景图
    backGFile = ui->lineEdit_backG->text();
    if(backGFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择背景图文件!");
        return -1;
    }
    QFile file1(backGFile);
    if(!file1.exists())
    {
        QMessageBox::warning(this,"警告","选择的背景图文件不存在！");
        return -1;
    }
    // 判别INS
    INSFile = ui->lineEdit_INS->text();
    if(INSFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择INS文件!");
        return -1;
    }
    QFile file2(INSFile);
    if(!file2.exists())
    {
        QMessageBox::warning(this,"警告","选择的INS文件不存在！");
        return -1;
    }
    // 判别real
    realFile = ui->lineEdit_real->text();
    if(realFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择真实航迹文件!");
        return -1;
    }
    QFile file3(realFile);
    if(!file3.exists())
    {
        QMessageBox::warning(this,"警告","选择的真实航迹文件不存在！");
        return -1;
    }
    // 判别输出文件
    outputPath = ui->lineEdit_out->text();
    if(outputPath.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择输出文件!");
        return -1;
    }
    return 0;
}

void InputPathForm::navTERCOM()
{
    if (check()!=0)
        return;
    QLayout *lay = ui->verticalLayout_tercom->layout();
    if (lay)
    {
        QLayoutItem *item;
        while((item = lay->takeAt(0))!=nullptr)
        {
            delete item->widget();
            delete item;
        }
    }
    ui->textBrowser->append("TERCOM匹配导航计算开始...");
    QCoreApplication::processEvents();
    Geomagnetic::TercomMatching tm(ui->doubleSpinBox->value(),
                                   ui->doubleSpinBox_2->value());
    int ret = tm.ReadBackground(backGFile);
    if (ret == -1)
    {
        ui->textBrowser->append("文件大小为：0  - 文件为空");
        QCoreApplication::processEvents();
        return;
    }
    ui->textBrowser->append(tm.error_str);
    QCoreApplication::processEvents();
    tm.ReadINS(INSFile);
    tm.ReadTruePath(realFile);
    Geomagnetic::Datapoint result = tm.matchWithAdaptiveRotation();
    ui->textBrowser->append("TERCOM匹配导航计算完毕!");
    ui->textBrowser->append("等待绘图...");
    QCoreApplication::processEvents();
    tm.drawResult(result);
    ui->verticalLayout_tercom->addWidget(tm.customPlot);
    tm.customPlot->saveBmp(QDir::currentPath()+"/tercom.bmp");
    // 输出结果到txt
    tm.saveResult(outputPath,result);
    ui->textBrowser->append("绘图完成!结果已保存为："+outputPath);
}

void InputPathForm::navICCP()
{
    if (check()!=0)
        return;
    QLayout *lay = ui->verticalLayout_iccp->layout();
    if (lay)
    {
        QLayoutItem *item;
        while((item = lay->takeAt(0))!=nullptr)
        {
            delete item->widget();
            delete item;
        }
    }
    ICCP my(ui->doubleSpinBox->value(),
            ui->doubleSpinBox_2->value());
    ui->textBrowser->append("ICCP匹配导航计算开始...");
    QCoreApplication::processEvents();
    QVector<QPointF> X = my.cal(backGFile,
                                INSFile,
                                realFile,0.000001);
    ui->verticalLayout_iccp->addWidget(my.customPlot);
    my.customPlot->saveBmp(QDir::currentPath()+"/iccp.bmp");
    ui->textBrowser->append("ICCP匹配导航计算完毕!");
    QCoreApplication::processEvents();
    // ui->textBrowser->append("ICCP匹配导航RMS: "+QString::number(my.finalRMS) + " km");
    QCoreApplication::processEvents();
}

void InputPathForm::navSITAN()
{
    try {
        qDebug() << "=== navSITAN 开始（内存优化版本）===";

        if (check() != 0) {
            qWarning() << "check() 验证失败";
            return;
        }

        // 清理布局
        QLayout *lay = ui->verticalLayout_sitan->layout();
        if (lay)
        {
            QLayoutItem *item;
            while((item = lay->takeAt(0)) != nullptr)
            {
                delete item->widget();
                delete item;
            }
        }

        ui->textBrowser->append("SITAN匹配导航计算开始...");
        QCoreApplication::processEvents();

        // 检查可用内存
        qDebug() << "检查系统资源...";
        ui->textBrowser->append("正在检查系统资源...");
        QCoreApplication::processEvents();

        // 验证输入文件并检查大小
        QFileInfo backFileInfo(backGFile);
        QFileInfo insFileInfo(INSFile);
        QFileInfo realFileInfo(realFile);

        // 如果背景场文件过大，给出警告
        if (backFileInfo.size() > 100 * 1024 * 1024) { // 100MB
            qWarning() << "背景场文件过大:" << backFileInfo.size() << "字节";
            ui->textBrowser->append("警告: 背景场文件较大，可能需要较长时间处理");
            QCoreApplication::processEvents();
        }

        // 步骤1：执行 TERCOM（内存控制）
        qDebug() << "执行 TERCOM 匹配...";
        ui->textBrowser->append("正在执行 TERCOM 匹配...");
        QCoreApplication::processEvents();

        Geomagnetic::Datapoint result;
        {
            // 使用局部作用域控制 TERCOM 对象的生命周期
            Geomagnetic::TercomMatching tm;
            try {
                tm.ReadBackground(backGFile);
                tm.ReadINS(INSFile);
                tm.ReadTruePath(realFile);
                result = tm.matchWithAdaptiveRotation();
            } catch (const std::bad_alloc& e) {
                qCritical() << "TERCOM 阶段内存分配失败";
                ui->textBrowser->append("错误: TERCOM 处理时内存不足");
                return;
            } catch (const std::exception& e) {
                qCritical() << "TERCOM 异常:" << e.what();
                ui->textBrowser->append(QString("TERCOM 失败: %1").arg(e.what()));
                return;
            }
            // tm 对象在这里被销毁，释放内存
        }

        if (result.empty()) {
            qWarning() << "TERCOM 结果为空";
            ui->textBrowser->append("错误: TERCOM 匹配失败");
            return;
        }

        qDebug() << "TERCOM 完成，结果点数:" << result.size();
        ui->textBrowser->append(QString("TERCOM 完成，获得 %1 个轨迹点").arg(result.size()));
        QCoreApplication::processEvents();

        // 步骤2：转换数据（内存优化）
        QVector<INSData> tercomInsData;
        try {
            tercomInsData.reserve(static_cast<int>(result.size()));

            for (auto &pr : result) {
                const SinglePoint &sp = pr.second;

                // 检查数据有效性
                if (std::isnan(sp.X) || std::isnan(sp.Y) || std::isnan(sp.tMagnetic) ||
                    std::isinf(sp.X) || std::isinf(sp.Y) || std::isinf(sp.tMagnetic)) {
                    qWarning() << "跳过无效的 TERCOM 结果点";
                    continue;
                }

                INSData id;
                id.x = sp.X;
                id.y = sp.Y;
                id.magnetic = sp.tMagnetic;
                tercomInsData.append(id);
            }
        } catch (const std::bad_alloc& e) {
            qCritical() << "数据转换时内存分配失败";
            ui->textBrowser->append("错误: 数据转换时内存不足");
            return;
        }

        if (tercomInsData.empty()) {
            qWarning() << "转换后的数据为空";
            ui->textBrowser->append("错误: 没有有效的 TERCOM 数据");
            return;
        }

        qDebug() << "数据转换完成，有效点数:" << tercomInsData.size();

        // 步骤3：分阶段创建 SITAN 对象和背景场
        qDebug() << "准备 SITAN 处理...";
        ui->textBrowser->append("正在准备 SITAN 算法...");
        QCoreApplication::processEvents();

        QVector<QVector<double>> grid;

        try {
            // 分步骤创建，便于内存管理
            qDebug() << "创建 SITAN 对象...";
            st = new Geomagnetic::SitanMatching;

            if (!st) {
                throw std::runtime_error("SITAN 对象创建失败");
            }
            grid = st->ReadBackground(backGFile);
            st->background = grid;
            qDebug() << "检查背景场数据...";
            if (st->background.isEmpty()) {
                throw std::runtime_error("背景场数据为空");
            }

            // 检查背景场大小是否合理
            int rows = st->background.size();
            int cols = st->background.isEmpty() ? 0 : st->background[0].size();
            qint64 estimatedMemory = static_cast<qint64>(rows) * cols * sizeof(double);

            qDebug() << "背景场网格:" << rows << "x" << cols;
            qDebug() << "预估内存使用:" << estimatedMemory / (1024*1024) << "MB";

            if (estimatedMemory > 500 * 1024 * 1024) { // 500MB
                qWarning() << "背景场数据过大，可能导致内存不足";
                ui->textBrowser->append("警告: 背景场数据较大，正在优化处理...");
                QCoreApplication::processEvents();
            }

            // 验证网格数据
            bool hasInvalidData = false;
            int sampleCount = 0;
            for (int i = 0; i < rows && sampleCount < 1000; i += std::max(1, rows/100)) {
                for (int j = 0; j < cols && sampleCount < 1000; j += std::max(1, cols/100)) {
                    double val = grid[i][j];
                    if (std::isnan(val) || std::isinf(val)) {
                        hasInvalidData = true;
                        break;
                    }
                    sampleCount++;
                }
                if (hasInvalidData) break;
            }

            if (hasInvalidData) {
                throw std::runtime_error("背景场数据包含无效值");
            }

        } catch (const std::bad_alloc& e) {
            qCritical() << "SITAN 对象创建时内存分配失败";
            ui->textBrowser->append("错误: SITAN 初始化时内存不足");
            if (st) {
                delete st;
                st = nullptr;
            }
            return;
        } catch (const std::exception& e) {
            qCritical() << "SITAN 准备阶段异常:" << e.what();
            ui->textBrowser->append(QString("SITAN 准备失败: %1").arg(e.what()));
            if (st) {
                delete st;
                st = nullptr;
            }
            return;
        }

        // 步骤4：执行 SITAN 算法（内存监控）
        try {
            qDebug() << "执行 SITAN 算法...";
            ui->textBrowser->append("正在执行 SITAN 算法（可能需要几分钟）...");
            QCoreApplication::processEvents();

            // 分批处理以减少内存压力
            QVector<QPointF> sitanResult;

            if (tercomInsData.size() > 1000) {
                // 对于大数据集，考虑分批处理
                ui->textBrowser->append("数据集较大，正在优化处理...");
                QCoreApplication::processEvents();
            }

            sitanResult = st->SITANAlgorithm(grid, tercomInsData);
            QVector<QPointF> real =  readPointsFromFile(realFile);
            st->drawResult(sitanResult,grid,real,backGFile,tercomInsData);
            if (sitanResult.isEmpty()) {
                qWarning() << "SITAN 算法结果为空";
                ui->textBrowser->append("警告: SITAN 算法未返回结果");
            } else {
                qDebug() << "SITAN 算法完成，结果点数:" << sitanResult.size();
                ui->textBrowser->append(QString("SITAN 算法完成，处理了 %1 个点").arg(sitanResult.size()));
            }

        } catch (const std::bad_alloc& e) {
            qCritical() << "SITAN 算法执行时内存分配失败";
            ui->textBrowser->append("错误: SITAN 算法执行时内存不足，请尝试减少数据量或重启程序");
            delete st;
            return;
        } catch (const std::exception& e) {
            qCritical() << "SITAN 算法异常:" << e.what();
            ui->textBrowser->append(QString("SITAN 算法失败: %1").arg(e.what()));
            delete st;
            return;
        }

        // 步骤5：显示结果
        try {
            if (st->customPlot) {
                ui->verticalLayout_sitan->addWidget(st->customPlot);
                ui->textBrowser->append("SITAN 结果显示成功");
            } else {
                qWarning() << "customPlot 为空";
                ui->textBrowser->append("警告: 无法显示 SITAN 结果图表");
            }

            // 不删除 st，因为 customPlot 仍在使用

        } catch (const std::exception& e) {
            qWarning() << "结果显示异常:" << e.what();
            ui->textBrowser->append("警告: 结果显示可能不完整");
        }

        ui->textBrowser->append("SITAN匹配导航计算完毕!");
        QCoreApplication::processEvents();

        qDebug() << "=== navSITAN 完成 ===";

    } catch (const std::bad_alloc& e) {
        qCritical() << "严重内存错误:" << e.what();
        ui->textBrowser->append("严重错误: 系统内存不足，请关闭其他程序后重试");
    } catch (const std::exception& e) {
        qCritical() << "navSITAN 发生异常:" << e.what();
        ui->textBrowser->append(QString("SITAN计算失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "navSITAN 发生未知异常";
        ui->textBrowser->append("SITAN计算失败: 未知错误");
    }
}

void InputPathForm::navTERCOM_ICCP()
{
    if (check()!=0)
        return;
    QLayout *lay = ui->verticalLayout_tercomiccp->layout();
    if (lay)
    {
        QLayoutItem *item;
        while((item = lay->takeAt(0))!=nullptr)
        {
            delete item->widget();
            delete item;
        }
    }
    // 设置背景图的分辨率，根据out.txt
    ui->textBrowser->append("TERCOM与ICCP联合匹配导航计算开始...");
    QCoreApplication::processEvents();
    Geomagnetic::TercomMatching tm;
    tm.ReadBackground(backGFile);
    tm.ReadINS(INSFile);
    tm.ReadTruePath(realFile);
    Geomagnetic::Datapoint result = tm.match();
    QFile file(QDir::currentPath()+"/tercom_ins.csv");
    QCoreApplication::processEvents();
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
    ICCP my(ui->doubleSpinBox->value(),
            ui->doubleSpinBox_2->value());
    // 设置背景图的分辨率，根据out.txt
    QVector<QPointF> X = my.cal(backGFile,
                                INSFile,
                                QDir::currentPath()+"/tercom_ins.csv",
                                realFile,0.001);
    ui->verticalLayout_tercomiccp->addWidget(my.customPlot);
    my.customPlot->saveBmp(QDir::currentPath()+"/tercom_iccp.bmp");
    ui->textBrowser->append("TERCOM与ICCP联合匹配导航计算完毕!");
    QCoreApplication::processEvents();
//    ui->textBrowser->append("TERCOM与ICCP联合匹配导航RMS: "+QString::number(my.finalRMS) + " km");
    QCoreApplication::processEvents();
}
void InputPathForm::navAUTO()
{
    if (check() != 0) {
        return;
    }

    // 清空上一次自动计算时动态添加的控件
    QLayout *lay = ui->verticalLayout_auto->layout();
    if (lay) {
        QLayoutItem *item;
        while ((item = lay->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    ui->textBrowser->append("开始自动计算");
    QCoreApplication::processEvents();

    try {

        // 直接调用 runAll，让它自动完成：读数据 → TERCOM → SITAN（用 TERCOM 结果）→ ICCP → drawResult
        anav = new Geomagnetic::AUTONAV();
        anav->x_step = ui->doubleSpinBox->value(); // 设置背景图的分辨率
        anav->y_step = ui->doubleSpinBox_2->value(); // 设置背景图的分辨率
        anav->runAll(backGFile, INSFile, realFile);

        ui->textBrowser->append("所有匹配计算及绘图已完成");
        QCoreApplication::processEvents();

        // 把生成的 customPlot 加到界面上
        ui->verticalLayout_auto->addWidget(anav->customPlot);

        // 保存一张 BMP 文件
        QString bmpPath = QDir::currentPath() + "/auto.bmp";
        anav->customPlot->saveBmp(bmpPath, 800, 600);

        ui->textBrowser->append("绘图已保存到：" + bmpPath);
        QCoreApplication::processEvents();
    }
    catch (const std::bad_alloc &e) {
        ui->textBrowser->append("内存分配失败: " + QString::fromStdString(e.what()));
        qWarning() << "内存分配失败: " << e.what();
    }
    catch (const std::exception &e) {
        ui->textBrowser->append("发生异常: " + QString::fromStdString(e.what()));
        qWarning() << "发生异常: " << e.what();
    }
    catch (...) {
        ui->textBrowser->append("发生未知异常");
        qWarning() << "发生未知异常";
    }
}

void InputPathForm::on_pushButton_confirm_clicked()
{
    if (check()!=0)
        return;
    switch(ui->comboBox->currentIndex())
    {
    case 0:
    {
        navTERCOM();
        break;
    }
    case 1:
    {
        navICCP();
        break;
    }
    case 2:
    {
        navSITAN();
        break;
    }
    case 3:
    {
        navTERCOM_ICCP();
        break;
    }
    case 4:
    {
        navAUTO();
        break;
    }
    default:
        break;
    }
}

void InputPathForm::on_pushButton_cancel_clicked()
{
    ui->lineEdit_backG->setText(backGFile);
    ui->lineEdit_INS->setText(INSFile);
    ui->lineEdit_real->setText(realFile);
    close();
}

void InputPathForm::on_comboBox_currentIndexChanged(int index)
{
    switch(index)
    {
    case 0://tercom
    {
        ui->line_2->setVisible(true);
        ui->line_3->setVisible(true);
        ui->gridWidget->setVisible(true);
        ui->verticalWidget_tercom->setVisible(true);
        ui->verticalWidget_iccp->setVisible(false);
        ui->verticalWidget_sitan->setVisible(false);
        ui->verticalWidget_tercomiccp->setVisible(false);
        ui->verticalWidget_auto->setVisible(false);
        break;
    }
    case 1:
    {
        ui->line_2->setVisible(true);
        ui->line_3->setVisible(true);
        ui->gridWidget->setVisible(true);
        ui->verticalWidget_tercom->setVisible(false);
        ui->verticalWidget_iccp->setVisible(true);
        ui->verticalWidget_sitan->setVisible(false);
        ui->verticalWidget_tercomiccp->setVisible(false);
        ui->verticalWidget_auto->setVisible(false);
        break;
    }
    case 2:
    {
        ui->line_2->setVisible(true);
        ui->line_3->setVisible(true);
        ui->gridWidget->setVisible(true);
        ui->verticalWidget_tercom->setVisible(false);
        ui->verticalWidget_iccp->setVisible(false);
        ui->verticalWidget_sitan->setVisible(true);
        ui->verticalWidget_tercomiccp->setVisible(false);
        ui->verticalWidget_auto->setVisible(false);
        break;
    }
    case 3:
    {
        ui->line_2->setVisible(true);
        ui->line_3->setVisible(true);
        ui->gridWidget->setVisible(true);
        ui->verticalWidget_tercom->setVisible(false);
        ui->verticalWidget_iccp->setVisible(false);
        ui->verticalWidget_sitan->setVisible(false);
        ui->verticalWidget_tercomiccp->setVisible(true);
        ui->verticalWidget_auto->setVisible(false);
        break;
    }
    case 4:
    {
        ui->line_2->setVisible(true);
        ui->line_3->setVisible(true);
        ui->gridWidget->setVisible(true);
        ui->verticalWidget_tercom->setVisible(false);
        ui->verticalWidget_iccp->setVisible(false);
        ui->verticalWidget_sitan->setVisible(false);
        ui->verticalWidget_tercomiccp->setVisible(false);
        ui->verticalWidget_auto->setVisible(true);
        break;
    }
    default:
        break;
    }
}


