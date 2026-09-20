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
    resize(UiScale::windowSize(720, 660));
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
    root->addWidget(FormKit::hint(tr("请选择多个文件，并在“中误差”列填写每个文件的中误差（不能为 0）。")));

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

    // 3  output file
    root->addSpacing(2);
    root->addWidget(FormKit::stepHeading(3, tr("保存结果")));
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
    // 选择多个文件
    QStringList fileNames;
    fileNames = QFileDialog::getOpenFileNames(this, tr("打开数据"),
                                                  QCoreApplication::applicationFilePath(),
                                                  tr("All Files (*.*);;文本文件 (*.txt *.dat *.csv)"));
    if (fileNames.isEmpty())
        return;   // dialog cancelled
    if (fileNames.size() == 1)
    {
       QMessageBox::warning(this,"警告","请选择多个文件");
       return;
    }
    //
    // keep one model for the table's lifetime (the empty-table hint follows it)
    auto *model = qobject_cast<QStandardItemModel *>(ui->tableView->model());
    model->clear();
    model->setColumnCount(2);
    model->setRowCount(fileNames.size());
    model->setHorizontalHeaderLabels({"文件","中误差"});
    for (int i =0 ;i<fileNames.size();i++)
    {
        QStandardItem *item = new QStandardItem(fileNames.at(i));
        model->setItem(i,0,item);
        myMerge.doc.push_back(fileNames.at(i).toStdString());
    }
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
    if (filecount < 2)
    {
        QMessageBox::warning(this,"警告","请先选择多个测线文件");
        return;
    }
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
    myMerge.outResult(filename);
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
        draw_form_1->autoset_heatMapView(xx1,yy1,zz1);
        draw_form_1->autoset_contourView(xx1,yy1,zz1);
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
    draw_form_3->create_xyz_f(filename,xx3,yy3,zz3);
    draw_form_3->autoset_heatMapView(xx3,yy3,zz3);
    draw_form_3->autoset_contourView(xx3,yy3,zz3);
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

void mergeForm::on_pushButton_clicked()
{
    // 选择背景文件
    QString tmp = QFileDialog::getSaveFileName(this, tr("请选择保存文件"),
                                               QCoreApplication::applicationFilePath(),"*.*");
    if (!tmp.isEmpty())
        ui->lineEdit->setText(tmp);
}

