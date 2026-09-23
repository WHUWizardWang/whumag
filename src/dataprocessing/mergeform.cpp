#include "mergeform.h"
#include "ui_mergeform.h"

#include "formkit.h"
#include "uiscale.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent>


mergeForm::mergeForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mergeForm)
{
    ui->setupUi(this);
    //
    QStandardItemModel *model = new QStandardItemModel(0,2,this);
    model->setHorizontalHeaderLabels({"文件","中误差"});
    ui->tableView->setModel(model);
    ui->tableView->setColumnWidth(1,60);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    buildLayout();
}

mergeForm::~mergeForm()
{
    delete ui;
}

// Replaces the .ui layout with a sectioned one.  The .ui only has captions besides the widgets used
// by the code, and no code refers to those captions, so they are simply replaced.
void mergeForm::buildLayout()
{
    qDeleteAll(findChildren<QLabel *>());
    setWindowTitle(tr("数据融合"));
    resize(UiScale::windowSize(720, 740));
    setMinimumWidth(UiScale::dp(620));

    auto *root = new QVBoxLayout;
    root->setContentsMargins(26, 22, 26, 18);
    root->setSpacing(12);
    root->addWidget(FormKit::header(QStringLiteral("merge"), tr("数据融合"), tr("把多条测线融合为一个网格数据")));

    // 1  files
    ui->chooseFiles->setText(tr("选择文件…"));
    auto *filesHead = new QHBoxLayout;
    filesHead->addWidget(FormKit::stepHeading(1, tr("选择测线文件")), 1);
    filesHead->addWidget(ui->chooseFiles);
    root->addLayout(filesHead);
    ui->tableView->setMinimumHeight(UiScale::dp(140));
    FormKit::emptyStateFor(ui->tableView, tr("尚未选择文件"), tr("点击右上角“选择文件…”，一次选择多个测线文件"));
    root->addWidget(ui->tableView, 1);
    root->addWidget(FormKit::hint(tr("请选择多个文件，并在“中误差”列填写每个文件的中误差（须大于 0），中误差越小权越大。")));

    // 2  range and grid
    root->addSpacing(2);
    root->addWidget(FormKit::stepHeading(2, tr("融合范围与网格")));
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);
    auto light = [](const QString &text, Qt::Alignment align) {
        auto *l = new QLabel(text);
        l->setProperty("role", QStringLiteral("field"));
        l->setAlignment(align | Qt::AlignVCenter);
        return l;
    };
    const QStringList heads = {tr("从"), tr("到"), tr("间隔"), tr("窗口大小")};
    for (int c = 0; c < 4; ++c)
        grid->addWidget(light(heads[c], Qt::AlignHCenter), 0, c + 1);
    grid->addWidget(light(tr("经度 / X"), Qt::AlignLeft), 1, 0);
    grid->addWidget(light(tr("纬度 / Y"), Qt::AlignLeft), 2, 0);
    QLineEdit *lon[] = {ui->lineEdit_lon_min, ui->lineEdit_lon_max, ui->lineEdit_lon_step, ui->lineEdit_lonx};
    QLineEdit *lat[] = {ui->lineEdit_lat_min, ui->lineEdit_lat_max, ui->lineEdit_lat_step, ui->lineEdit_laty};
    for (int c = 0; c < 4; ++c)
    {
        for (QLineEdit *edit : {lon[c], lat[c]})
        {
            edit->setMinimumWidth(0);
            edit->setMaximumWidth(QWIDGETSIZE_MAX);
            edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        grid->addWidget(lon[c], 1, c + 1);
        grid->addWidget(lat[c], 2, c + 1);
        grid->setColumnStretch(c + 1, 1);
    }
    root->addLayout(grid);

    root->addWidget(FormKit::hint(tr("窗口大小为插值窗口的全宽（与坐标同单位），窗口内的测点按距离（Shepard 权）和 1/中误差² 加权；"
                                     "窗口内没有测点的格网点不输出。")));

    // 3  robustness
    root->addSpacing(2);
    root->addWidget(FormKit::stepHeading(3, tr("数据检核")));
    removeBias_ = new QCheckBox(tr("消除各数据源的系统偏差（以中误差最小的数据为基准，取重叠区差值的中位数）"), this);
    rejectOutliers_ = new QCheckBox(tr("剔除粗差（与同一数据源相邻测点的中位数相差超过 3.5 倍稳健中误差）"), this);
    removeBias_->setChecked(true);
    rejectOutliers_->setChecked(true);
    root->addWidget(removeBias_);
    root->addWidget(rejectOutliers_);

    // 4  output file
    root->addSpacing(2);
    root->addWidget(FormKit::stepHeading(4, tr("保存结果")));
    ui->lineEdit->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->pushButton->setText(tr("选择…"));
    auto *saveRow = new QHBoxLayout;
    saveRow->setSpacing(8);
    saveRow->addWidget(ui->lineEdit, 1);
    saveRow->addWidget(ui->pushButton);
    root->addLayout(FormKit::field(tr("保存文件名"), saveRow));

    ui->confirm->setText(tr("开始融合"));
    FormKit::setRole(ui->confirm, "primary");
    ui->confirm->setMinimumSize(UiScale::dp(120), 36);
    ui->confirm->setCursor(Qt::PointingHandCursor);
    auto *footer = new QHBoxLayout;
    footer->addStretch(1);
    footer->addWidget(ui->confirm);
    root->addLayout(footer);

    delete layout();
    setLayout(root);
}

void mergeForm::on_chooseFiles_clicked()
{
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("打开数据"),
                                                                QCoreApplication::applicationFilePath(),
                                                                tr("All Files (*.*);;文本文件 (*.txt *.dat *.csv)"));
    if (fileNames.isEmpty())
        return;   // dialog cancelled
    if (fileNames.size() == 1)
    {
       QMessageBox::warning(this,"警告","请选择多个文件");
       return;
    }
    // keep one model for the table's lifetime (the empty-table hint follows it)
    auto *model = qobject_cast<QStandardItemModel *>(ui->tableView->model());
    model->clear();
    model->setColumnCount(2);
    model->setRowCount(fileNames.size());
    model->setHorizontalHeaderLabels({"文件","中误差"});
    for (int i = 0; i < fileNames.size(); i++)
        model->setItem(i, 0, new QStandardItem(fileNames.at(i)));
    ui->tableView->setColumnWidth(1,60);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
}

// Reads and checks the form; shows a warning and returns false when something is missing.
bool mergeForm::readJob(Proc::FusionJob &job)
{
    QAbstractItemModel *model = ui->tableView->model();
    const int rows = model ? model->rowCount() : 0;
    if (rows < 2)
    {
        QMessageBox::warning(this,"警告","请先选择多个测线文件");
        return false;
    }
    for (int i = 0; i < rows; i++)
    {
        bool ok = false;
        const double sigma = model->data(model->index(i, 1)).toString().trimmed().toDouble(&ok);
        if (!ok || !(sigma > 0))
        {
            QMessageBox::warning(this,"警告",QStringLiteral("第 %1 个文件的中误差须为大于 0 的数").arg(i + 1));
            return false;
        }
        job.files << model->data(model->index(i, 0)).toString();
        job.sigmas << sigma;
    }

    struct Field { QLineEdit *edit; QString name; double *value; bool positive; };
    const Field fields[] = {
        {ui->lineEdit_lon_min,  tr("经度/X 起点"),  &job.grid.xMin, false},
        {ui->lineEdit_lon_max,  tr("经度/X 终点"),  &job.grid.xMax, false},
        {ui->lineEdit_lon_step, tr("经度/X 间隔"),  &job.grid.dx, true},
        {ui->lineEdit_lonx,     tr("经度/X 窗口"),  &job.options.windowX, true},
        {ui->lineEdit_lat_min,  tr("纬度/Y 起点"),  &job.grid.yMin, false},
        {ui->lineEdit_lat_max,  tr("纬度/Y 终点"),  &job.grid.yMax, false},
        {ui->lineEdit_lat_step, tr("纬度/Y 间隔"),  &job.grid.dy, true},
        {ui->lineEdit_laty,     tr("纬度/Y 窗口"),  &job.options.windowY, true},
    };
    for (const Field &f : fields)
    {
        bool ok = false;
        *f.value = f.edit->text().trimmed().toDouble(&ok);
        if (!ok || (f.positive && !(*f.value > 0)))
        {
            QMessageBox::warning(this,"警告",f.positive ? QStringLiteral("%1须为大于 0 的数").arg(f.name)
                                                          : QStringLiteral("请填写%1").arg(f.name));
            f.edit->setFocus();
            return false;
        }
    }
    if (job.grid.xMax < job.grid.xMin || job.grid.yMax < job.grid.yMin)
    {
        QMessageBox::warning(this,"警告","融合范围的“到”不能小于“从”");
        return false;
    }
    job.outputFile = ui->lineEdit->text().trimmed();
    if (job.outputFile.isEmpty())
    {
        QMessageBox::warning(this,"警告","请输入保存的文件名");
        return false;
    }
    if (QFileInfo(job.outputFile).isRelative() && !filepath.isEmpty())
    {
        const QString dir = filepath + "/Processed/";
        QDir().mkpath(dir);
        job.outputFile = dir + job.outputFile;
        if (QFileInfo(job.outputFile).suffix().isEmpty())
            job.outputFile += ".txt";
    }
    job.options.removeBias = removeBias_->isChecked();
    job.options.rejectOutliers = rejectOutliers_->isChecked();
    return true;
}

void mergeForm::on_confirm_clicked()
{
    if (running_)
        return;
    Proc::FusionJob job;
    if (!readJob(job))
        return;
    close();
    running_ = true;
    emit textUpdated("数据融合处理开始...");
    auto *watcher = new QFutureWatcher<Proc::FusionOutcome>(this);
    connect(watcher, &QFutureWatcher<Proc::FusionOutcome>::finished, this, [this, watcher, job]() {
        const Proc::FusionOutcome out = watcher->result();
        watcher->deleteLater();
        running_ = false;
        for (const QString &line : out.log)
            emit textUpdated(QStringLiteral("  ") + line);
        if (!out.ok())
        {
            emit textUpdated(QStringLiteral("错误: 数据融合失败：%1").arg(out.error));
            return;
        }
        emit textUpdated("数据融合处理完毕!");
        showResult(job);
        emit treeUpdated(1);
    });
    watcher->setFuture(QtConcurrent::run([job]() { return Proc::runFusion(job); }));
}

// One tab per input file and one for the fused grid.
void mergeForm::showResult(const Proc::FusionJob &job)
{
    auto *tabs = new QTabWidget;
    tabs->setAttribute(Qt::WA_DeleteOnClose);
    auto addMap = [tabs](const QString &file, const QString &name) {
        auto *page = new QWidget(tabs);
        auto *layout = new QVBoxLayout(page);
        auto *form = new draw_Form(page);
        QVector<double> xx, yy, zz;
        form->create_xyz_f(file, xx, yy, zz);
        if (!xx.isEmpty())
        {
            form->autoset_heatMapView(xx, yy, zz);
            if (form->magWarn)
                form->autoset_contourView(xx, yy, zz);
        }
        layout->addWidget(form);
        tabs->addTab(page, name);
    };
    for (const QString &file : job.files)
        addMap(file, QFileInfo(file).completeBaseName());
    addMap(job.outputFile, QStringLiteral("融合结果"));
    tabs->setWindowTitle("数据融合");
    tabs->resize(1100,700);
    tabs->show();
}

void mergeForm::on_pushButton_clicked()
{
    // 选择背景文件
    QString tmp = QFileDialog::getSaveFileName(this, tr("请选择保存文件"),
                                               QCoreApplication::applicationFilePath(),"*.*");
    if (!tmp.isEmpty())
        ui->lineEdit->setText(tmp);
}

