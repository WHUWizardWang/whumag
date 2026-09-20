#include "anoqueryform.h"
#include "ui_anoqueryform.h"

#include "qcustomplot.h"
#include "queryformshell.h"
#include "thememanager.h"
#include "uiwidgets.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QHeaderView>
#include <QPushButton>
#include <QTimer>
#include <cmath>

AnoQueryForm::AnoQueryForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AnoQueryForm)
    , point_form_(new AnoQueryFromPointSetForm)
    , grid_form_(new AnoQueryFromGridSetForm)
    , table_model_(new QStandardItemModel)
{
    ui->setupUi(this);
    init_comboBox_error();
    init_tableView();

    ui->verticalLayout_4->addWidget(point_form_);
    ui->verticalLayout_4->addWidget(grid_form_);
    point_form_->setVisible(false);
    grid_form_->setVisible(false);
    ui->radioButton_point->setChecked(true);
    ui->radioButton_file->hide();
    ui->tableView->setModel(table_model_);
    ui->pushButton_2->setVisible(false);
    ui->pushButton_export->setText(tr("导出到文件"));

    // new layout: parameters left, status / stats / preview / table right
    ui->radioButton_file->setParent(this);   // unused mode, kept alive so the .ui pointers stay valid
    QueryFormShell::Parts parts;
    parts.form = this;
    parts.product = ui->comboBox_error;
    parts.radioPoint = ui->radioButton_point;
    parts.radioGrid = ui->radioButton_grid;
    parts.subforms = {point_form_, grid_form_};
    parts.query = ui->pushButton_3;
    parts.table = ui->tableView;
    parts.exportButton = ui->pushButton_export;
    parts.importButton = ui->pushButton_2;
    parts.log = ui->textBrowser;
    parts.oldLeft = ui->frame;
    parts.oldRight = ui->frame_2;
    const QueryFormShell::Shell shell = QueryFormShell::apply(parts, tr("产品"));
    statusChip_ = shell.statusChip;
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);   // three even, right-aligned columns
    ui->tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    buildResultViews(shell.middle, shell.leftLayout, shell.leftInsertIndex);

    countTimer_ = new QTimer(this);
    countTimer_->setSingleShot(true);
    countTimer_->setInterval(250);
    connect(countTimer_, &QTimer::timeout, this, &AnoQueryForm::updateCountBanner);
    connect(grid_form_, &AnoQueryFromGridSetForm::changed, countTimer_, QOverload<>::of(&QTimer::start));
    connect(ui->comboBox_error, &QComboBox::currentTextChanged, countTimer_, QOverload<>::of(&QTimer::start));
    updateCountBanner();
}

AnoQueryForm::~AnoQueryForm()
{
    delete point_form_;
    delete grid_form_;
    delete ui;
}

void AnoQueryForm::init_comboBox_error()
{
    QStringList name_list{"EMAG2", "MAMEA"};
    ui->comboBox_error->addItems(name_list);
    ui->comboBox_error->setCurrentIndex(0);
}

void AnoQueryForm::init_tableView()
{
    table_model_->clear();
    table_model_->setRowCount(0);
    table_model_->setColumnCount(3);
    table_model_->setHorizontalHeaderItem(0, new QStandardItem("纬度 X (°N)"));
    table_model_->setHorizontalHeaderItem(1, new QStandardItem("经度 Y (°E)"));
    table_model_->setHorizontalHeaderItem(2, new QStandardItem("ΔT (nT)"));
}

void AnoQueryForm::fill_item_model(QVector<AnoPoint> &ano_pnts, const QVector<char> &matched)
{
    init_tableView();
    QFont mono;
    mono.setFamilies({QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"), QStringLiteral("Courier New")});
    mono.setStyleHint(QFont::Monospace);
    mono.setPixelSize(12);
    const QColor muted = ThemeManager::instance().color("t3");

    table_model_->setRowCount(ano_pnts.size());
    for (int i = 0; i < ano_pnts.size(); ++i)
    {
        const bool has = matched.isEmpty() || (i < matched.size() && matched[i]);
        const QString values[3] = {QString::number(ano_pnts[i].x, 'f', 6), QString::number(ano_pnts[i].y, 'f', 6),
                                   QString::number(ano_pnts[i].z, 'f', 6)};
        for (int c = 0; c < 3; ++c)
        {
            auto *item = new QStandardItem(has || c < 2 ? values[c] : QStringLiteral("无数据"));
            item->setData(values[c], Qt::UserRole);          // what the export writes
            item->setFont(mono);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (!has && c == 2)
                item->setForeground(muted);
            table_model_->setItem(i, c, item);
        }
    }
}

void AnoQueryForm::on_radioButton_point_toggled(bool checked)
{
    point_form_->setVisible(checked);
    if (countTimer_)
        countTimer_->start();
}

void AnoQueryForm::on_radioButton_file_toggled(bool checked)
{
    Q_UNUSED(checked)
}

void AnoQueryForm::on_radioButton_grid_toggled(bool checked)
{
    grid_form_->setVisible(checked);
    if (countTimer_)
        countTimer_->start();
}

void AnoQueryForm::stepsForProduct(const QString &product, double *lonStep, double *latStep)
{
    if (product == "MAMEA")
    {
        *lonStep = (160.0 - 93.0) / 2011.0;
        *latStep = (46.0 + 12.0) / 1741.0;
    }
    else
    {
        *lonStep = 360.0 / 10800.0;
        *latStep = 180.0 / 5400.0;
    }
}

// ---------------------------------------------------------------- result views
void AnoQueryForm::buildResultViews(QVBoxLayout *middle, QVBoxLayout *leftLayout, int leftInsertIndex)
{
    // left: how many grid points the current ranges will query
    countBanner_ = new Banner;
    countBanner_->hide();
    leftLayout->insertWidget(leftInsertIndex, countBanner_);

    // right: banner for partial / failed queries
    resultBanner_ = new Banner;
    resultBanner_->hide();
    middle->addWidget(resultBanner_);

    // right: four numbers (shown once there is a result)
    statsHost_ = new QWidget;
    auto *stats = new QHBoxLayout(statsHost_);
    stats->setContentsMargins(0, 0, 0, 0);
    stats->setSpacing(10);
    statMin_ = new StatCard(tr("最小值"));
    statMax_ = new StatCard(tr("最大值"));
    statValid_ = new StatCard(tr("有效格点"));
    statTime_ = new StatCard(tr("用时"));
    for (StatCard *c : {statMin_, statMax_, statValid_, statTime_})
        stats->addWidget(c, 1);
    statsHost_->hide();
    middle->addWidget(statsHost_);

    // right: preview of the queried grid
    chartCard_ = new QFrame;
    chartCard_->setProperty("role", QStringLiteral("card"));
    auto *cl = new QVBoxLayout(chartCard_);
    cl->setContentsMargins(14, 10, 14, 10);
    cl->setSpacing(6);
    auto *ch = new QHBoxLayout;
    auto *chTitle = new QLabel(tr("格网预览"));
    chTitle->setProperty("role", QStringLiteral("section"));
    chartInfo_ = new QLabel;
    chartInfo_->setProperty("role", QStringLiteral("mono"));
    chartInfo_->setStyleSheet("font-size: 11px;");
    auto *detail = new QPushButton(tr("在新窗口查看"));
    detail->setProperty("role", QStringLiteral("link"));
    detail->setCursor(Qt::PointingHandCursor);
    connect(detail, &QPushButton::clicked, this, &AnoQueryForm::openDetailWindow);
    ch->addWidget(chTitle);
    ch->addStretch(1);
    ch->addWidget(chartInfo_);
    ch->addWidget(detail);
    cl->addLayout(ch);

    plot_ = new QCustomPlot;
    plot_->setMinimumHeight(190);
    plot_->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    heat_ = new QCPColorMap(plot_->xAxis, plot_->yAxis);
    heat_->setInterpolate(false);
    heat_->setTightBoundary(true);
    scale_ = new QCPColorScale(plot_);
    plot_->plotLayout()->addElement(0, 1, scale_);
    scale_->setType(QCPAxis::atRight);
    heat_->setColorScale(scale_);
    scale_->axis()->setLabel(QStringLiteral("ΔT (nT)"));
    // same diverging scale as the rest of the application: blue - cream (0 nT) - orange
    QCPColorGradient gradient;
    gradient.clearColorStops();
    const struct { double pos; const char *hex; } stops[] = {
        {0.00, "#22367E"}, {0.15, "#2F6DB5"}, {0.30, "#6FAFD6"}, {0.425, "#BFDDE8"}, {0.50, "#F2F0E6"},
        {0.575, "#F4D9A8"}, {0.70, "#EDA35A"}, {0.85, "#D4552F"}, {1.00, "#8F1B2B"}};
    for (const auto &s : stops)
        gradient.setColorStopAt(s.pos, QColor(s.hex));
    gradient.setLevelCount(256);
    heat_->setGradient(gradient);
    QCPMarginGroup *group = new QCPMarginGroup(plot_);
    plot_->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, group);
    scale_->setMarginGroup(QCP::msBottom | QCP::msTop, group);
    cl->addWidget(plot_, 1);
    chartCard_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    chartCard_->hide();
    middle->addWidget(chartCard_);

    applyPlotTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &AnoQueryForm::applyPlotTheme);
}

void AnoQueryForm::applyPlotTheme()
{
    const ThemeManager &tm = ThemeManager::instance();
    const QColor bg = tm.color("n2"), axis = tm.color("line2"), text = tm.color("t2");
    plot_->setBackground(QBrush(bg));
    plot_->axisRect()->setBackground(QBrush(bg));
    QFont f = plot_->font();
    f.setPixelSize(11);
    for (QCPAxis *a : {plot_->xAxis, plot_->yAxis, scale_->axis()})
    {
        a->setBasePen(QPen(axis));
        a->setTickPen(QPen(axis));
        a->setSubTickPen(QPen(axis));
        a->setTickLabelColor(text);
        a->setLabelColor(text);
        a->setTickLabelFont(f);
        a->setLabelFont(f);
        a->grid()->setPen(QPen(tm.color("line"), 1, Qt::DotLine));
    }
    plot_->replot();
}

void AnoQueryForm::updateCountBanner()
{
    if (!countBanner_)
        return;
    if (!ui->radioButton_grid->isChecked())
    {
        countBanner_->hide();
        return;
    }
    double lonStep, latStep;
    stepsForProduct(ui->comboBox_error->currentText(), &lonStep, &latStep);
    const int n = grid_form_->getQueryNum(lonStep, latStep);
    if (n < 1)
    {
        countBanner_->setContent(Banner::Warn, tr("查询范围无效"), tr("请检查范围：纬度 −90 ~ 90，经度 −180 ~ 180，且“到”不小于“从”。"));
    }
    else
    {
        const double mb = n * static_cast<double>(sizeof(AnoPoint)) / 1e6;
        countBanner_->setContent(Banner::Info, tr("将查询 %1 × %2 = %3 个格点").arg(grid_form_->latCount()).arg(grid_form_->lonCount()).arg(n),
                                 tr("预计内存约 %1 MB；超过 1,000 点时会先请你确认。").arg(mb, 0, 'f', 2));
    }
    countBanner_->show();
}

void AnoQueryForm::updatePlot(bool gridMode)
{
    if (!gridMode || lastLatCount_ < 2 || lastLonCount_ < 2 || lastPoints_.size() != lastLatCount_ * lastLonCount_)
    {
        chartCard_->hide();
        return;
    }
    heat_->data()->setSize(lastLonCount_, lastLatCount_);
    heat_->data()->setRange(QCPRange(lastLonMin_, lastLonMin_ + lastLonStep_ * (lastLonCount_ - 1)),
                            QCPRange(lastLatMin_, lastLatMin_ + lastLatStep_ * (lastLatCount_ - 1)));
    double lo = 1e300, hi = -1e300;
    for (int iy = 0; iy < lastLatCount_; ++iy)
    {
        for (int ix = 0; ix < lastLonCount_; ++ix)
        {
            const int i = iy * lastLonCount_ + ix;
            if (i < lastMatched_.size() && lastMatched_[i])
            {
                heat_->data()->setCell(ix, iy, lastPoints_[i].z);
                lo = std::min(lo, lastPoints_[i].z);
                hi = std::max(hi, lastPoints_[i].z);
            }
            else
            {
                heat_->data()->setCell(ix, iy, 0);
                heat_->data()->setAlpha(ix, iy, 0);   // no data: transparent
            }
        }
    }
    if (lo > hi)
    {
        chartCard_->hide();
        return;
    }
    // symmetric about 0 nT so the neutral colour means "no anomaly"
    double m = std::max(std::fabs(lo), std::fabs(hi));
    m = m < 10 ? std::ceil(m) : std::ceil(m / 10) * 10;
    if (m < 1)
        m = 1;
    heat_->setDataRange(QCPRange(-m, m));
    plot_->xAxis->setRange(lastLonMin_ - lastLonStep_ / 2, lastLonMin_ + lastLonStep_ * (lastLonCount_ - 0.5));
    plot_->yAxis->setRange(lastLatMin_ - lastLatStep_ / 2, lastLatMin_ + lastLatStep_ * (lastLatCount_ - 0.5));
    plot_->xAxis->setLabel(QStringLiteral("经度 (°E)"));
    plot_->yAxis->setLabel(QStringLiteral("纬度 (°N)"));
    chartInfo_->setText(QStringLiteral("%1 × %2 · %3°").arg(lastLatCount_).arg(lastLonCount_).arg(lastLatStep_, 0, 'f', 4));
    chartCard_->show();
    plot_->replot();
}

void AnoQueryForm::showResults(int ret, qint64 elapsedMs, bool gridMode)
{
    statTime_->setValue(elapsedMs < 1000 ? QStringLiteral("%1 ms").arg(elapsedMs) : QStringLiteral("%1 s").arg(elapsedMs / 1000.0, 0, 'f', 1));
    if (ret != 0 && ret != -2)
    {
        statusChip_->setText(tr("失败"));
        statusChip_->setKind(Chip::Error);
        resultBanner_->setContent(Banner::Error, tr("无法连接数据库"), tr("请检查数据库服务及连接配置；也可以重新启动程序并再次登录。"));
        resultBanner_->show();
        chartCard_->hide();
        statsHost_->hide();
        return;
    }

    statsHost_->show();
    int valid = 0;
    double lo = 1e300, hi = -1e300;
    for (int i = 0; i < lastPoints_.size(); ++i)
    {
        if (i < lastMatched_.size() && lastMatched_[i])
        {
            ++valid;
            lo = std::min(lo, lastPoints_[i].z);
            hi = std::max(hi, lastPoints_[i].z);
        }
    }
    statMin_->setValue(valid ? QString::number(lo, 'f', 1) + QStringLiteral(" nT") : QStringLiteral("—"));
    statMax_->setValue(valid ? QString::number(hi, 'f', 1) + QStringLiteral(" nT") : QStringLiteral("—"));
    statValid_->setValue(QStringLiteral("%1 / %2").arg(valid).arg(lastPoints_.size()));

    if (ret == -2)
    {
        statusChip_->setText(tr("部分完成"));
        statusChip_->setKind(Chip::Warn);
        resultBanner_->setContent(Banner::Warn, tr("查询部分完成"), tr("数据库连接在查询过程中中断，结果可能不完整，建议重新查询。"));
        resultBanner_->show();
    }
    else if (valid < lastPoints_.size())
    {
        statusChip_->setText(tr("已完成"));
        statusChip_->setKind(Chip::Ok);
        resultBanner_->setContent(Banner::Info, tr("%1 个格点在数据表中没有记录").arg(lastPoints_.size() - valid),
                                  tr("这些点在结果列表中显示为“无数据”，预览图中留空。"));
        resultBanner_->show();
    }
    else
    {
        statusChip_->setText(tr("已完成"));
        statusChip_->setKind(Chip::Ok);
        resultBanner_->hide();
    }
    updatePlot(gridMode);
}

void AnoQueryForm::openDetailWindow()
{
    if (lastPoints_.size() <= 4)
        return;
    draw_Form *draw_form_ = new draw_Form;
    draw_form_->setAttribute(Qt::WA_DeleteOnClose);
    QVector<double> xx, yy, zz;
    draw_form_->create_xyz_p(lastPoints_, xx, yy, zz);
    draw_form_->autoset_heatMapView(xx, yy, zz);
    draw_form_->autoset_contourView(xx, yy, zz);
    draw_form_->show();
}

// ---------------------------------------------------------------- query
void AnoQueryForm::on_pushButton_3_clicked()
{
    QVector<AnoPoint> ano_pnts;
    int length = 0;
    double step_lon = 0, step_lat = 0;
    const bool gridMode = ui->radioButton_grid->isChecked();
    if (ui->radioButton_point->isChecked())
    {
        length = 1;
        point_form_->toCoordGeodeticArray(ano_pnts);
    }
    else if (gridMode)
    {
        stepsForProduct(ui->comboBox_error->currentText(), &step_lon, &step_lat);
        length = grid_form_->getQueryNum(step_lon, step_lat);
        if (length < 1)
        {
            updateCountBanner();
            return;
        }
        else if (length > 1000)
        {
            int byte_num = length * (sizeof(AnoPoint));
            double mry_size = byte_num * 1.0 / 1E6;
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
        grid_form_->toCoordGeodeticArray(ano_pnts);
        lastLatCount_ = grid_form_->latCount();
        lastLonCount_ = grid_form_->lonCount();
        lastLatMin_ = grid_form_->latMinimum();
        lastLonMin_ = grid_form_->lonMinimum();
        lastLatStep_ = grid_form_->latStep();
        lastLonStep_ = grid_form_->lonStep();
    }

    if (length == 0 || ano_pnts.isEmpty())
        return;

    if (DatabaseManager::instance().isOffline())
    {
        statusChip_->setText(tr("离线"));
        statusChip_->setKind(Chip::Warn);
        resultBanner_->setContent(Banner::Warn, tr("离线工作模式"), tr("磁异常查询需要数据库。重新启动程序并连接数据库后即可使用。"));
        resultBanner_->show();
        return;
    }

    statusChip_->setText(tr("查询中…"));
    statusChip_->setKind(Chip::Accent);
    resultBanner_->hide();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    QElapsedTimer timer;
    timer.start();
    QVector<char> matched;
    int ret = -1;
    if (ui->comboBox_error->currentText() == "EMAG2")
    {
        ret = MagAnoQuery::omg_emag2(ano_pnts, &matched);
    }
    else if (ui->comboBox_error->currentText() == "MAMEA")
    {
        ret = MagAnoQuery::omg_mamea(ano_pnts, &matched);
    }
    const qint64 ms = timer.elapsed();
    QApplication::restoreOverrideCursor();

    // Only draw / report on an actual query.  ret == -1: no connection, nothing was queried;
    // ret == -2: connected, but the connection dropped part-way through.
    lastPoints_ = ano_pnts;
    lastMatched_ = matched;
    if (ret == 0 || ret == -2)
        fill_item_model(ano_pnts, matched);
    showResults(ret, ms, gridMode);
}

void AnoQueryForm::on_pushButton_export_clicked()
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
                QStandardItem *item = table_model_->item(i, j);
                const QVariant raw = item->data(Qt::UserRole);
                file.write((raw.isValid() ? raw.toString() : item->data(Qt::DisplayRole).toString()).toUtf8());
                file.write("  ");
            }
            file.write("\r\n");
        }
        resultBanner_->setContent(Banner::Ok, tr("已导出 %1 行").arg(table_model_->rowCount()), save_file_name);
        resultBanner_->show();
    }
    else
    {
        resultBanner_->setContent(Banner::Error, tr("无法写入文件"), save_file_name);
        resultBanner_->show();
    }
    file.close();
}

#ifdef WHUMAG_UI_TEST
// Test-only: fills the form with a synthetic grid so the preview / statistics can be checked
// without a database (called through WHUMAG_TEST_QUERY_DEMO).
void AnoQueryForm::testFillSynthetic()
{
    const int nLat = 24, nLon = 40;
    lastLatCount_ = nLat;
    lastLonCount_ = nLon;
    lastLatMin_ = 30.0;
    lastLonMin_ = 110.0;
    lastLatStep_ = 0.25;
    lastLonStep_ = 0.25;
    lastPoints_.clear();
    lastMatched_.clear();
    for (int iy = 0; iy < nLat; ++iy)
    {
        for (int ix = 0; ix < nLon; ++ix)
        {
            AnoPoint p;
            p.x = lastLatMin_ + iy * lastLatStep_;
            p.y = lastLonMin_ + ix * lastLonStep_;
            const double a = std::exp(-(std::pow(ix - 12, 2) + std::pow(iy - 8, 2)) / 40.0) * 62.0;
            const double b = std::exp(-(std::pow(ix - 28, 2) + std::pow(iy - 15, 2)) / 55.0) * -48.0;
            p.z = a + b + 3.0 * std::sin(ix * 0.7) * std::cos(iy * 0.5);
            lastPoints_.append(p);
            lastMatched_.append((ix > 33 && iy < 4) ? 0 : 1);   // a few cells without data
        }
    }
    fill_item_model(lastPoints_, lastMatched_);
    showResults(0, 231, true);
}
#endif
