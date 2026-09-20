#include "queryform.h"
#include "ui_queryform.h"

#include "queryformshell.h"
#include "thememanager.h"
#include "uiwidgets.h"

#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QHeaderView>
#include <QTimer>
#include <cfloat>

QueryForm::QueryForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QueryForm)
    , point_form_(new QueryFromPointSetForm)
    , file_form_(new QueryFromFileSetForm)
    , grid_form_(new QueryFromGridSetForm)
    , table_model_(new QStandardItemModel)
{
    ui->setupUi(this);
    init_comboBox_wmm();
    init_tableView();

    ui->verticalLayout_4->addWidget(point_form_) ;
    ui->verticalLayout_4->addWidget(file_form_ );
    ui->verticalLayout_4->addWidget(grid_form_);
    point_form_->setVisible(false);
    file_form_->setVisible(false);
    grid_form_->setVisible(false);
    ui->radioButton_point->setChecked(true);
    ui->tableView->setModel(table_model_);
    ui->pushButton_2->setVisible(false);

    // new layout: parameters left, status / summary / table right
    QueryFormShell::Parts parts;
    parts.form = this;
    parts.product = ui->comboBox_wmm;
    parts.radioPoint = ui->radioButton_point;
    parts.radioFile = ui->radioButton_file;
    parts.radioGrid = ui->radioButton_grid;
    parts.subforms = {point_form_, file_form_, grid_form_};
    parts.query = ui->pushButton_3;
    parts.table = ui->tableView;
    parts.exportButton = ui->pushButton_export;
    parts.importButton = ui->pushButton_2;
    parts.log = ui->textBrowser;
    parts.oldLeft = ui->frame;
    parts.oldRight = ui->frame_2;
    const QueryFormShell::Shell shell = QueryFormShell::apply(parts, tr("模型"));
    statusChip_ = shell.statusChip;
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setStretchLastSection(false);
    buildResultViews(shell.middle, shell.leftLayout, shell.leftInsertIndex);

    countTimer_ = new QTimer(this);
    countTimer_->setSingleShot(true);
    countTimer_->setInterval(250);
    connect(countTimer_, &QTimer::timeout, this, &QueryForm::updateCountBanner);
    connect(grid_form_, &QueryFromGridSetForm::changed, countTimer_, QOverload<>::of(&QTimer::start));
    updateCountBanner();
}

QueryForm::~QueryForm()
{
    delete point_form_;
    delete file_form_;
    delete grid_form_;
    delete ui;
}

void QueryForm::init_comboBox_wmm()
{
    QStringList name_list{"WMM", "EMM", "IGRF","WMMHR"};
    ui->comboBox_wmm->addItems(name_list);
    ui->comboBox_wmm->setCurrentIndex(0);
}

void QueryForm::init_tableView()
{
    table_model_->clear();
    table_model_->setRowCount(0);
    table_model_->setColumnCount(18);
    table_model_->setHorizontalHeaderItem(0, new QStandardItem("纬度"));
    table_model_->setHorizontalHeaderItem(1, new QStandardItem("经度"));
    table_model_->setHorizontalHeaderItem(2, new QStandardItem("高程"));
    table_model_->setHorizontalHeaderItem(3, new QStandardItem("日期"));
    table_model_->setHorizontalHeaderItem(4, new QStandardItem("F"));
    table_model_->setHorizontalHeaderItem(5, new QStandardItem("Fdot"));
    table_model_->setHorizontalHeaderItem(6, new QStandardItem("H"));
    table_model_->setHorizontalHeaderItem(7, new QStandardItem("Hdot"));
    table_model_->setHorizontalHeaderItem(8, new QStandardItem("X"));
    table_model_->setHorizontalHeaderItem(9, new QStandardItem("Xdot"));
    table_model_->setHorizontalHeaderItem(10, new QStandardItem("Y"));
    table_model_->setHorizontalHeaderItem(11, new QStandardItem("Ydot"));
    table_model_->setHorizontalHeaderItem(12, new QStandardItem("Z"));
    table_model_->setHorizontalHeaderItem(13, new QStandardItem("Zdot"));
    table_model_->setHorizontalHeaderItem(14, new QStandardItem("Decl"));
    table_model_->setHorizontalHeaderItem(15, new QStandardItem("Decldot"));
    table_model_->setHorizontalHeaderItem(16, new QStandardItem("Incl"));
    table_model_->setHorizontalHeaderItem(17, new QStandardItem("Incldot"));
}

void QueryForm::fill_item_model(MAGtype_CoordGeodetic *CoordGeodeticArr,
                                MAGtype_Date *UserDateArr,
                                MAGtype_GeoMagneticElements *GeoMagneticElementsArr,
                                int length)
{
    init_tableView();

    QFont mono;
    mono.setFamilies({QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"), QStringLiteral("Courier New")});
    mono.setStyleHint(QFont::Monospace);
    mono.setPixelSize(12);
    auto put = [this, &mono](int row, int col, const QString &text) {
        auto *item = new QStandardItem(text);
        item->setFont(mono);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table_model_->setItem(row, col, item);
    };

    table_model_->setRowCount(length);
    for (int i = 0; i < length; ++i)
    {
        put(i, 0, QString::number(CoordGeodeticArr[i].phi, 'f', 6));
        put(i, 1, QString::number(CoordGeodeticArr[i].lambda, 'f', 6));

        // Height
        QString height_str;
        if (CoordGeodeticArr[i].UseGeoid)
        {
            height_str = "M" + QString::number(CoordGeodeticArr[i].HeightAboveGeoid, 'f', 1);
        }
        else
        {
            height_str = "E" + QString::number(CoordGeodeticArr[i].HeightAboveEllipsoid, 'f', 1);
        }
        put(i, 2, height_str);

        // Date
        QDate date(UserDateArr[i].Year, UserDateArr[i].Month, UserDateArr[i].Day);
        put(i, 3, date.toString("yyyy/MM/dd"));

        // Magnetic elements
        const MAGtype_GeoMagneticElements &e = GeoMagneticElementsArr[i];
        const double values[] = {e.F, e.Fdot, e.H, e.Hdot, e.X, e.Xdot, e.Y, e.Ydot, e.Z, e.Zdot, e.Decl, e.Decldot, e.Incl, e.Incldot};
        for (int k = 0; k < 14; ++k)
            put(i, 4 + k, QString::number(values[k], 'f', 1));
    }
}

void QueryForm::on_radioButton_point_toggled(bool checked)
{
    point_form_->setVisible(checked);
    if (countTimer_)
        countTimer_->start();
}

void QueryForm::on_radioButton_file_toggled(bool checked)
{
    file_form_->setVisible(checked);
}

void QueryForm::on_radioButton_grid_toggled(bool checked)
{
    grid_form_->setVisible(checked);
    if (countTimer_)
        countTimer_->start();
}

// ---------------------------------------------------------------- result views
void QueryForm::buildResultViews(QVBoxLayout *middle, QVBoxLayout *leftLayout, int leftInsertIndex)
{
    // left: how many points the current grid ranges will query
    countBanner_ = new Banner;
    countBanner_->hide();
    leftLayout->insertWidget(leftInsertIndex, countBanner_);

    // right: banner for failures
    resultBanner_ = new Banner;
    resultBanner_->hide();
    middle->addWidget(resultBanner_);

    // right: summary tiles -- the seven field elements for a single point, otherwise a short overview
    summaryHost_ = new QWidget;
    auto *grid = new QGridLayout(summaryHost_);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    for (int i = 0; i < 7; ++i)
    {
        auto *card = new StatCard(QString());
        cards_.append(card);
        grid->addWidget(card, i / 4, i % 4);
    }
    for (int c = 0; c < 4; ++c)
        grid->setColumnStretch(c, 1);
    summaryHost_->hide();
    middle->addWidget(summaryHost_);
}

void QueryForm::updateCountBanner()
{
    if (!countBanner_)
        return;
    if (!ui->radioButton_grid->isChecked())
    {
        countBanner_->hide();
        return;
    }
    const int n = grid_form_->getQueryNum();
    if (n < 1)
    {
        countBanner_->setContent(Banner::Warn, tr("查询范围无效"),
                                 tr("请检查各项范围与步长：“到”不小于“从”，步长大于 0，日期格式正确。"));
    }
    else
    {
        const double mb = n * static_cast<double>(sizeof(MAGtype_CoordGeodetic) + sizeof(MAGtype_Date) + sizeof(MAGtype_GeoMagneticElements) * 2) / 1e6;
        countBanner_->setContent(Banner::Info, tr("将查询 %1 个点").arg(n),
                                 tr("预计内存约 %1 MB；超过 1,000 点时会先请你确认。").arg(mb, 0, 'f', 2));
    }
    countBanner_->show();
}

void QueryForm::showFailure(const QString &chipText, const QString &title, const QString &body)
{
    statusChip_->setText(chipText);
    statusChip_->setKind(Chip::Error);
    resultBanner_->setContent(Banner::Error, title, body);
    resultBanner_->show();
    summaryHost_->hide();
}

void QueryForm::showSummary(const MAGtype_GeoMagneticElements *elements, int length, bool pointMode, qint64 elapsedMs)
{
    if (pointMode)
    {
        const struct { const char *caption; double value; int digits; } items[] = {
            {"总强度 F (nT)", elements[0].F, 1},    {"水平强度 H (nT)", elements[0].H, 1}, {"北向 X (nT)", elements[0].X, 1},
            {"东向 Y (nT)", elements[0].Y, 1},      {"垂直 Z (nT)", elements[0].Z, 1},     {"磁偏角 D (°)", elements[0].Decl, 2},
            {"磁倾角 I (°)", elements[0].Incl, 2}};
        for (int i = 0; i < 7; ++i)
        {
            cards_[i]->setCaption(QString::fromUtf8(items[i].caption));
            cards_[i]->setValue(QString::number(items[i].value, 'f', items[i].digits));
            cards_[i]->show();
        }
    }
    else
    {
        double lo = DBL_MAX, hi = -DBL_MAX;
        for (int i = 0; i < length; ++i)
        {
            lo = qMin(lo, elements[i].F);
            hi = qMax(hi, elements[i].F);
        }
        cards_[0]->setCaption(tr("记录数"));
        cards_[0]->setValue(QString::number(length));
        cards_[1]->setCaption(tr("F 最小 (nT)"));
        cards_[1]->setValue(QString::number(lo, 'f', 1));
        cards_[2]->setCaption(tr("F 最大 (nT)"));
        cards_[2]->setValue(QString::number(hi, 'f', 1));
        cards_[3]->setCaption(tr("用时"));
        cards_[3]->setValue(elapsedMs < 1000 ? QStringLiteral("%1 ms").arg(elapsedMs) : QStringLiteral("%1 s").arg(elapsedMs / 1000.0, 0, 'f', 1));
        for (int i = 4; i < 7; ++i)
            cards_[i]->hide();
    }
    summaryHost_->show();
}

// ---------------------------------------------------------------- query
void QueryForm::on_pushButton_3_clicked()
{
    MAGtype_CoordGeodetic *CoordGeodeticArr;
    MAGtype_Date *UserDateArr;
    int length = 0;
    if (ui->radioButton_point->isChecked())
    {
        length = 1;
        CoordGeodeticArr = new MAGtype_CoordGeodetic[length];
        UserDateArr = new MAGtype_Date[length];

        point_form_->toCoordGeodeticArray(CoordGeodeticArr, UserDateArr, length);
    }
    else if (ui->radioButton_file->isChecked())
    {
        length = file_form_->getQueryNum();
        if (length < 1)
        {
            return;
        }
        CoordGeodeticArr = new MAGtype_CoordGeodetic[length];
        UserDateArr = new MAGtype_Date[length];
        file_form_->toCoordGeodeticArray(CoordGeodeticArr, UserDateArr, length);
    }
    else if (ui->radioButton_grid->isChecked())
    {
        length = grid_form_->getQueryNum();
        if (length < 1)
        {
            updateCountBanner();
            return;
        }
        else if (length > 1000)
        {
            int byte_num = length * (sizeof(MAGtype_CoordGeodetic) + sizeof(MAGtype_Date) + sizeof (MAGtype_GeoMagneticElements) * 2);
            double mry_size =  byte_num * 1.0 / 1E6;
            QMessageBox msg_box;
            msg_box.setText("查询数量：" + QString::number(length) + "，预计占用内存 " + QString::number(mry_size, 'f', 1) + " MB\n是否继续？");
            msg_box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msg_box.setDefaultButton(QMessageBox::No);
            int ret = msg_box.exec();
            if (ret == QMessageBox::No)
            {
                return;
            }
        }
        CoordGeodeticArr = new MAGtype_CoordGeodetic[length];
        UserDateArr = new MAGtype_Date[length];
        grid_form_->toCoordGeodeticArray(CoordGeodeticArr, UserDateArr, length);
    }

    if (length != 0)
    {
        MAGtype_GeoMagneticElements *GeoMagneticElementsArr = new MAGtype_GeoMagneticElements[length];
        MAGtype_GeoMagneticElements *ErrorsArr = new MAGtype_GeoMagneticElements[length];

        statusChip_->setText(tr("计算中…"));
        statusChip_->setKind(Chip::Accent);
        resultBanner_->hide();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();

        QElapsedTimer timer;
        timer.start();
        int ret = -1;
        if (ui->comboBox_wmm->currentText() == "WMM")
        {
            ret = omg_wmm(CoordGeodeticArr, UserDateArr, GeoMagneticElementsArr, ErrorsArr, length);
        }
        else if (ui->comboBox_wmm->currentText() == "EMM")
        {
            ret = omg_emm(CoordGeodeticArr, UserDateArr, GeoMagneticElementsArr, ErrorsArr, length);
        }
        else if (ui->comboBox_wmm->currentText() == "IGRF")
        {
            ret = omg_igrf(CoordGeodeticArr, UserDateArr, GeoMagneticElementsArr, ErrorsArr, length);
        }
        else if (ui->comboBox_wmm->currentText() == "WMMHR")
        {
            ret = omg_wmm_hr(CoordGeodeticArr, UserDateArr, GeoMagneticElementsArr, ErrorsArr, length);
        }
        const qint64 ms = timer.elapsed();
        QApplication::restoreOverrideCursor();

        if (ret == 0)
        {
            fill_item_model(CoordGeodeticArr, UserDateArr, GeoMagneticElementsArr, length);
            statusChip_->setText(tr("已完成"));
            statusChip_->setKind(Chip::Ok);
            showSummary(GeoMagneticElementsArr, length, ui->radioButton_point->isChecked(), ms);
        }
        else
        {
            showFailure(tr("失败"), tr("找不到系数文件"),
                        tr("无法读取 %1 的系数（COF）文件。请确认程序目录下的 COF 文件夹完整。").arg(ui->comboBox_wmm->currentText()));
        }

        delete [] CoordGeodeticArr;
        delete [] UserDateArr;
        delete [] GeoMagneticElementsArr;
        delete [] ErrorsArr;
        CoordGeodeticArr = nullptr;
        UserDateArr = nullptr;
        GeoMagneticElementsArr = nullptr;
        ErrorsArr = nullptr;
    }
}

void QueryForm::on_pushButton_export_clicked()
{
    if (table_model_->rowCount() == 0)
    {
        QMessageBox::information(this, tr("导出"), tr("还没有查询结果可以导出。"));
        return;
    }
    QString save_file_name = QFileDialog::getSaveFileName(this, "保存查询结果", "./query_results.txt", "TXT(*.txt)");
    if (save_file_name.isEmpty())
        return;
    QFile file(save_file_name);
    if (file.open(QIODevice::WriteOnly))
    {
        for (int i = 0; i < table_model_->columnCount() - 1; ++i)
        {
            file.write(table_model_->horizontalHeaderItem(i)->text().toUtf8());
            file.write("  ");
        }
        file.write(table_model_->horizontalHeaderItem(table_model_->columnCount() - 1)->text().toUtf8());
        file.write("\r\n");

        for (int i = 0; i < table_model_->rowCount(); ++i)
        {
            for (int j = 0; j < table_model_->columnCount(); ++j)
            {
                file.write(table_model_->item(i, j)->data(Qt::DisplayRole).toString().toUtf8());
                file.write("  ");
            }
            file.write("\r\n");
        }
    }
    const bool opened = file.isOpen();
    file.close();
    if (opened)
        resultBanner_->setContent(Banner::Ok, tr("已导出 %1 行").arg(table_model_->rowCount()), save_file_name);
    else
        resultBanner_->setContent(Banner::Error, tr("无法写入文件"), save_file_name);
    resultBanner_->show();
}
