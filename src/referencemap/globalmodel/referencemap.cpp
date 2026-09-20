#include "referencemap.h"
#include "ui_referencemap.h"

#include "formkit.h"
#include "resultpreviewpanel.h"
#include "thememanager.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>


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

    buildLayout();
}

ReferenceMap::~ReferenceMap()
{
    delete ui;
}

// Replaces the .ui layout: parameters (scrollable, the model forms can be long) in a left panel,
// preview + log in a right panel.  Every widget the modelling code touches is kept.
void ReferenceMap::buildLayout()
{
    setWindowTitle(tr("整图建模"));
    resize(UiScale::windowSize(1180, 720));

    // the mode radios stay alive (the existing slots and button group use them) but hidden
    for (QWidget *w : {static_cast<QWidget *>(ui->radioButton_auto), static_cast<QWidget *>(ui->radioButton_self)})
    {
        w->setParent(this);
        w->hide();
    }

    // ---- left: scrollable content + pinned run button
    auto *content = new QWidget;
    auto *ll = new QVBoxLayout(content);
    ll->setContentsMargins(24, 20, 24, 12);
    ll->setSpacing(14);
    ll->addWidget(FormKit::header(QStringLiteral("grid"), tr("整图建模"), tr("对整块实测数据建立基准图")));

    ll->addWidget(FormKit::stepHeading(1, tr("选择数据")));
    ll->addLayout(FormKit::field(tr("实测数据"), ui->comboBox_data));

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(2, tr("建图参数")));
    auto *res = new QHBoxLayout;
    res->setSpacing(12);
    res->addLayout(FormKit::field(tr("插值分辨率 · 经度 (X)"), ui->doubleSpinBox_dx), 1);
    res->addLayout(FormKit::field(tr("插值分辨率 · 纬度 (Y)"), ui->doubleSpinBox_dy), 1);
    ll->addLayout(res);
    ll->addLayout(FormKit::field(tr("抽稀参数 (km)"), ui->doubleSpinBox_sparse));

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(3, tr("建模方法")));
    ui->comboBox_model->setPlaceholderText(tr("请选择插值模型"));
    ll->addLayout(FormKit::field(tr("插值模型"), ui->comboBox_model));

    auto *mode = new SegmentedControl({tr("智能计算最优参数"), tr("自定义参数")});
    mode->setCurrentIndex(ui->radioButton_self->isChecked() ? 1 : 0);
    connect(mode, &SegmentedControl::currentChanged, this, [this](int index) {
        (index == 0 ? ui->radioButton_auto : ui->radioButton_self)->click();   // click() also fires the button group
    });
    ll->addLayout(FormKit::field(tr("参数设置"), mode, tr("选择“自定义参数”后，才能修改下方的模型参数。")));
    ll->addWidget(ui->widget_para);

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(4, tr("保存结果")));
    ui->pushButton_2->setText(tr("浏览…"));
    auto *saveRow = new QHBoxLayout;
    saveRow->setSpacing(8);
    saveRow->addWidget(ui->lineEdit_savePath, 1);
    saveRow->addWidget(ui->pushButton_2);
    ll->addLayout(FormKit::field(tr("保存路径"), saveRow));
    ll->addStretch(1);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(content);
    content->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);

    leftPanel_ = new QWidget;
    leftPanel_->setObjectName("refLeft");
    leftPanel_->setFixedWidth(UiScale::dp(440));
    leftPanel_->setAttribute(Qt::WA_StyledBackground, true);
    auto *lp = new QVBoxLayout(leftPanel_);
    lp->setContentsMargins(0, 0, 0, 0);
    lp->setSpacing(0);
    lp->addWidget(scroll, 1);

    ui->pushButton->setText(tr("开始建模"));
    FormKit::setRole(ui->pushButton, "primary");
    ui->pushButton->setMinimumHeight(36);
    ui->pushButton->setCursor(Qt::PointingHandCursor);
    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(24, 10, 24, 18);
    footer->addWidget(ui->pushButton, 1);
    lp->addLayout(footer);

    // ---- right: status, preview card and log
    preview_ = new ResultPreviewPanel({ui->widget_pic0, ui->widget_pic1, ui->widget_pic2, ui->widget_pic3, ui->widget_pic4, ui->widget_pic5},
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
    delete ui->widget_3;
    delete ui->widget_2;
    delete ui->widget_model;
    delete ui->widget_4;
    delete ui->widget;
    delete ui->widget_confirm;
    delete ui->line;
    delete ui->line_2;

    auto restyle = [this]() {
        const ThemeManager &tm = ThemeManager::instance();
        leftPanel_->setStyleSheet(QStringLiteral("#refLeft { background: %1; border: none; border-right: 1px solid %2; }"
                                                 "#refLeft QScrollArea { background: transparent; }")
                                      .arg(tm.hex("n1"), tm.hex("line")));
    };
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, restyle);
    restyle();

    // the model slot (connected by setupUi) shows / hides the six result containers; look again after it ran
    connect(ui->comboBox_model, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { preview_->refreshEmptyState(); });
}

void ReferenceMap::setBusy(bool busy)
{
    leftPanel_->setEnabled(!busy);
    ui->pushButton->setText(busy ? tr("处理中…") : tr("开始建模"));
    if (busy)
        preview_->setStatus(tr("处理中…"), Chip::Accent);
    else
        preview_->setStatus(runOk_ ? tr("已完成") : tr("未完成"), runOk_ ? Chip::Ok : Chip::Neutral);
    QCoreApplication::processEvents();
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

void ReferenceMap::on_pushButton_clicked()
{
    runOk_ = false;
    setBusy(true);
    runModeling();
    setBusy(false);
    preview_->refreshEmptyState();
}

void ReferenceMap::runModeling()
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
        if (data_index < 0 || data_index >= nameList_real.size())
        {
            QMessageBox::warning(nullptr, "错误", "请先导入实测数据后再进行该操作!");
            return;
        }
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        // 设置参数
        Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
//        Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
        Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
        Geomagnetic::ReadData readdata;
        Geomagnetic::Datainfo datainfo;

        readdata.readGridFromFile(str,datapoints);
        readdata.interval = sparse_para * 10;

        // 创建泰勒模型并设置参数
        Geomagnetic::TaylorModel taylorModel;
        std::string s = filename.toStdString();
        datainfo.Cutoff = para0;  // 设置泰勒多项式截止阶数
        readdata.createGridData(datapoints, datapoint_result, dx, dy);

        // 设置最小最大值并归一化数据
        taylorModel.setMinMax(datapoints);
        taylorModel.Normalize(datapoints);
        taylorModel.Normalize(datapoint_result);

        // 设置数据信息
        readdata.DataSet(datapoints, datainfo);

        // 设置中心点（默认为数据范围的中心点）
        taylorModel.setCenter(datainfo.Cx, datainfo.Cy);

        // 进行泰勒插值
        taylorModel.applyInterpolation(datapoints, datapoint_result, datainfo.Cutoff);

        // 反归一化数据
        taylorModel.Denormalize(datapoints);
        taylorModel.Denormalize(datapoint_result);
        ui->textBrowser->append("计算完成！");
        QCoreApplication::processEvents();
        auto end = std::chrono::high_resolution_clock::now(); // 获取当前时间点
        std::chrono::duration<double> elapsed = end - start;
        QString outstr = "处理时间: " + QString::number(elapsed.count()) + "s";
        ui->textBrowser->append(outstr+"\n");
        ui->textBrowser->append("结果正在保存与输出...");
        QCoreApplication::processEvents();
        // taylor.DeNormalize(datapoint_result);
        readdata.resultOut(datapoint_result, s);
        QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
//        double rms = calculateRMS(datapoints,datapoint_result);
        ui->textBrowser->append("基于泰勒多项式方法结果计算完成\n");
        QCoreApplication::processEvents();
//        ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        // 更新主界面的树
        emit treeUpdated(1);
        runOk_ = true;
        //
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
        draw_form_->autoset_heatMapView(xx,yy,zz);
        draw_form_->setMapStep(dx,dy);
        if(draw_form_->magWarn == false) {
            delete draw_form_;
            return;
        }
        draw_form_->autoset_contourView(xx,yy,zz);
        ui->widget_pic0->layout()->addWidget(draw_form_);
        ui->textBrowser->append("结果图已生成\n");
        QCoreApplication::processEvents();


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
        if (data_index < 0 || data_index >= nameList_real.size())
        {
            QMessageBox::warning(nullptr, "错误", "请先导入实测数据后再进行该操作!");
            return;
        }
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
        std::string s = filename.toStdString();
        datainfo.N = para1;
        readdata.createGridData(datapoints,datapoint_result,dx,dy);
        legendre.setMinMax(datapoints);
        legendre.Normalize(datapoints);
        legendre.Normalize(datapoint_result);
        // readdata.selectLineData(datapoints, datapoint_sparse, readdata.interval);
        // datapoint_result = datapoints;
        readdata.DataSet(datapoints, datainfo);
        for (auto& elem : datapoint_result)
            elem.second.tMagnetic = 0;
        ui->textBrowser->append("正在初始化...");
        QCoreApplication::processEvents();
        legendre.applyInterpolation(datapoints, datapoint_result, datainfo.N);
        ui->textBrowser->append("正在进行归一化处理...");
        QCoreApplication::processEvents();
        legendre.Denormalize(datapoints);
        legendre.Denormalize(datapoint_result);
        ui->textBrowser->append("正在计算勒让德矩阵...");
        QCoreApplication::processEvents();
        ui->textBrowser->append("正在计算结果...");
        QCoreApplication::processEvents();
        ui->textBrowser->append("结果正在保存与输出...");
        QCoreApplication::processEvents();
        readdata.resultOut(datapoint_result, s);
        QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
        ui->textBrowser->append("基于勒让德多项式方法结果计算完成\n");
        QCoreApplication::processEvents();
        // double rms = calculateRMS(datapoints,datapoint_result);
        // ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        //
        emit treeUpdated(1);
        runOk_ = true;
        //
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        draw_form_->create_xyz_f(QString::fromStdString(s),xx,yy,zz);
        draw_form_->autoset_heatMapView(xx,yy,zz);
        draw_form_->setMapStep(dx,dy);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->autoset_contourView(xx,yy,zz);
        ui->widget_pic1->layout()->addWidget(draw_form_);
        ui->textBrowser->append("结果图已生成\n");
        QCoreApplication::processEvents();

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
        if (data_index < 0 || data_index >= nameList_real.size())
        {
            QMessageBox::warning(nullptr, "错误", "请先导入实测数据后再进行该操作!");
            return;
        }
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
        try {
            // 设置参数
            Geomagnetic::Datapoint datapoints;                      // 原始数据
            Geomagnetic::Datapoint datapoint_sparse;
            Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
            Geomagnetic::ReadData readdata;
            Geomagnetic::Datainfo datainfo;
            Geomagnetic::Polyhedral poly;
            Geomagnetic::OptimizedCubicInterpolator oci;
            // 读取数据
            ui->textBrowser->append("正在读取数据...");
            QCoreApplication::processEvents();
            readdata.readGridFromFile(str, datapoints);
            // 数据集大小检查与警告
            if (datapoints.size() > 10000) {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, "大数据集警告",
                                              QString("数据点数量(%1)较大，计算可能需要较长时间和大量内存。是否继续?").arg(datapoints.size()),
                                              QMessageBox::Yes|QMessageBox::No);

                if (reply == QMessageBox::No) {
                    ui->textBrowser->append("<p style='color:red;'>操作已取消</p>");
                    return;
                }
                else {
                    readdata.selectRandomData(datapoints, datapoint_sparse, sparse_para);
                }
            }
            else
                datapoint_sparse = datapoints;

            // 设置参数
            readdata.interval = sparse_para * 10;
            datainfo.PolyQ = paraModel;
            datainfo.sigma2 = para2;
            readdata.createGridData(datapoint_sparse, datapoint_result, dx, dy);
            readdata.DataSet(datapoint_sparse, datainfo);

            // 初始化结果点磁场值
            for (auto& elem : datapoint_result)
                elem.second.tMagnetic = 0;

            // 计时开始
            auto start = std::chrono::high_resolution_clock::now();


            // 执行计算流程
            QThread::msleep(100);
            ui->textBrowser->append("正在初始化...");
            QCoreApplication::processEvents();
            //休眠100ms
            QThread::msleep(100);
            poly.init(datainfo, datapoint_sparse);

            ui->textBrowser->append("正在计算Q矩阵...");
            QCoreApplication::processEvents();
            QThread::msleep(100);
            poly.ComputeQ(datainfo, datapoint_sparse);

            QThread::msleep(100);
            ui->textBrowser->append("正在计算X矩阵...");
            QCoreApplication::processEvents();
            QThread::msleep(100);
            try {
                poly.ComputeX(datainfo, datapoint_sparse);
            }
            catch (const std::bad_alloc& e) {
                ui->textBrowser->append("<p style='color:red;'>内存不足，无法完成计算。尝试减少数据量或增加系统内存。</p>");
                QMessageBox::critical(nullptr, "内存错误", "计算过程中内存不足，请减少数据量或增加系统内存。");
                return;
            }

            ui->textBrowser->append("正在计算结果...");
            QCoreApplication::processEvents();
            poly.Result(datainfo, datapoint_result, datapoint_sparse);

            // 计时结束
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            QString outstr = "多面函数处理时间: " + QString::number(elapsed.count()) + "s";
            ui->textBrowser->append("<p style='color:blue;'>"+outstr+"</p>");
            QCoreApplication::processEvents();

            // 保存结果
            ui->textBrowser->append("结果正在保存与输出...");
            QCoreApplication::processEvents();
            std::string s = filename.toStdString();
            // poly.DeNormalize(datapoint_result);
            readdata.resultOut(datapoint_result, s);
            ui->textBrowser->append("结果已保存至: " + QString::fromStdString(s));

            // 更新树
            emit treeUpdated(1);
        runOk_ = true;

            // 可视化结果
            ui->textBrowser->append("正在生成可视化结果...");
            QCoreApplication::processEvents();
            draw_Form *draw_form_ = new draw_Form;
            QVector<double> xx, yy, zz;
            draw_form_->create_xyz_f(QString::fromStdString(s), xx, yy, zz);
            draw_form_->autoset_heatMapView(xx, yy, zz);
            draw_form_->setMapStep(dx, dy);

            if(draw_form_->magWarn == false) {
                ui->textBrowser->append("<p style='color:red;'>警告：可视化过程出现问题</p>");
                return;
            }

            draw_form_->autoset_contourView(xx, yy, zz);
            ui->widget_pic2->layout()->addWidget(draw_form_);
            ui->textBrowser->append("<p style='color:green;'>计算完成，结果图已生成</p>");
            QCoreApplication::processEvents();
        }
        catch (const std::exception& e) {
            ui->textBrowser->append("<p style='color:red;'>处理过程中出现错误: " + QString(e.what()) + "</p>");
            QMessageBox::critical(nullptr, "错误", "处理过程中出现异常: " + QString(e.what()));
        }
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
        if (data_index < 0 || data_index >= nameList_real.size())
        {
            QMessageBox::warning(nullptr, "错误", "请先导入实测数据后再进行该操作!");
            return;
        }
        QString filename = ui->lineEdit_savePath->text();
        std::string str = (projectPath+"/Measured/"+nameList_real[data_index]).toStdString();
        if (filename.isEmpty())
        {
            QMessageBox::warning(nullptr, "错误", "未选择保存路径!");
            return;
        }

        // 设置参数
        Geomagnetic::Datapoint datapoints;                      // 当前选中的数据,原始数据
        Geomagnetic::Datapoint datapoint_result;                // 计算后的数据
        Geomagnetic::Datapoint datapoint_sparse;                // 抽稀后的数据
        Geomagnetic::ReadData readdata;
        Geomagnetic::Datainfo datainfo;

        readdata.readGridFromFile(str,datapoints);
        if (datapoints.size() > 10000) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, "大数据集警告",
                                          QString("数据点数量(%1)较大，计算可能需要较长时间和大量内存。是否继续?").arg(datapoints.size()),
                                          QMessageBox::Yes|QMessageBox::No);

            if (reply == QMessageBox::No) {
                ui->textBrowser->append("<p style='color:red;'>操作已取消</p>");
                return;
            }
            else {
                readdata.selectRandomData(datapoints, datapoint_sparse, sparse_para);
            }
        }
        else
            datapoint_sparse = datapoints;

        readdata.interval = sparse_para * 10;
        Geomagnetic::Splinecurve spline;  // 50个最近邻点，epsilon=1e-10
        std::string s=filename.toStdString();

        datainfo.E= para3_0;
        datainfo.C = para3_1;
        readdata.createGridData(datapoint_sparse, datapoint_result, dx, dy);
        readdata.DataSet(datapoint_sparse, datainfo);

        // 初始化结果点磁场值
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
        draw_form_->autoset_heatMapView(xx,yy,zz);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->autoset_contourView(xx,yy,zz);
        ui->widget_pic3->layout()->addWidget(draw_form_);
        //
        QMessageBox::information(this,"已成功保存","已成功保存在" + QString::fromStdString(s),QMessageBox::Ok);
        ui->textBrowser->append("基于样条曲线方法结果计算完成");
        QCoreApplication::processEvents();
        // double rms = calculateRMS(datapoints,datapoint_result);
        // ui->textBrowser->append("rms: "+QString::number(rms,'f',2));
        // 更新主界面的树
        emit treeUpdated(1);
        runOk_ = true;
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
        if (data_index < 0 || data_index >= nameList_real.size())
        {
            QMessageBox::warning(nullptr, "错误", "请先导入实测数据后再进行该操作!");
            return;
        }
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
        else
        {
            ui->textBrowser->append("基于压缩感知方法结果计算完成");
            QCoreApplication::processEvents();;
        }
        QString s = projectPath+"/Processed/"+filename;
        ui->textBrowser->append("RMS:" + QString::number(manager.RMS));
        QCoreApplication::processEvents();
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        draw_form_->create_xyz_f(s,xx,yy,zz);
        draw_form_->autoset_heatMapView(xx,yy,zz);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->autoset_contourView(xx,yy,zz);
        ui->widget_pic4->layout()->addWidget(draw_form_);
        // 更新主界面的树
        emit treeUpdated(1);
        runOk_ = true;
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
        if (data_index < 0 || data_index >= nameList_real.size())
        {
            QMessageBox::warning(nullptr, "错误", "请先导入实测数据后再进行该操作!");
            return;
        }
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
        draw_form_->autoset_heatMapView(xx,yy,zz);
        if(draw_form_->magWarn == false)
            return;
        draw_form_->autoset_contourView(xx,yy,zz);
        ui->widget_pic5->layout()->addWidget(draw_form_);
        ui->textBrowser->append("计算完成！");
        QCoreApplication::processEvents();

        // 更新主界面的树
        emit treeUpdated(1);
        runOk_ = true;
        break;
    }
    default:
        QMessageBox::information(nullptr, "提醒", "请选择基准图插值模型。",
                                     QMessageBox::Ok);
    }
}




void ReferenceMap::on_pushButton_2_clicked()
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

