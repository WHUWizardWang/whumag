#include "importform.h"
#include "ui_importform.h"

#include "dataimportdialog.h"
#include "formkit.h"
#include "uiscale.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <QApplication>

ImportForm::ImportForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImportForm)
{
    ui->setupUi(this);
    ui->comboBox_datatype->addItem("实测数据");
    ui->comboBox_datatype->addItem("处理后数据");
    ui->comboBox_platform->addItem("船磁");
    ui->comboBox_platform->addItem("航磁");
    ui->comboBox_platform->addItem("水下磁测");
    buildLayout();
}

// Replaces the .ui layout with a cleaner one.  The widgets the code below still reads (the two
// mode radios, the "加入数据库" check box and the dx / dy spin boxes) stay alive but hidden.
void ImportForm::buildLayout()
{
    for (QWidget *w : {static_cast<QWidget *>(ui->radioButton), static_cast<QWidget *>(ui->radioButton_2),
                       static_cast<QWidget *>(ui->shujuku), static_cast<QWidget *>(ui->widget_dxdy)})
    {
        w->setParent(this);
        w->hide();
    }

    setWindowTitle(tr("导入数据"));
    resize(UiScale::windowSize(620, 480));
    setMinimumWidth(UiScale::dp(520));

    auto *root = new QVBoxLayout;
    root->setContentsMargins(26, 22, 26, 18);
    root->setSpacing(16);
    root->addWidget(FormKit::header(QStringLiteral("import"), tr("导入数据"), tr("把测线数据文件加入当前工程")));

    // file
    ui->lineEdit_path->setPlaceholderText(tr("选择 txt / csv / dat / cdf / nc 数据文件"));
    ui->pushButton_choose->setText(tr("选择文件…"));
    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(ui->lineEdit_path, 1);
    pathRow->addWidget(ui->pushButton_choose);
    root->addLayout(FormKit::field(tr("数据文件"), pathRow,
                                   tr("文本文件可用任意分隔符，也支持 NASA CDF（如卫星磁测数据）和 netCDF classic；"
                                      "选择文件后设置分隔方式和各列的含义。")));
    formatSummary_ = FormKit::hint(tr("尚未设置数据格式"));
    formatButton_ = new QPushButton(tr("格式与列设置…"));
    formatButton_->setEnabled(false);
    connect(formatButton_, &QPushButton::clicked, this, [this]() { chooseFormat(); });
    auto *formatRow = new QHBoxLayout;
    formatRow->setSpacing(8);
    formatRow->addWidget(formatSummary_, 1);
    formatRow->addWidget(formatButton_, 0, Qt::AlignTop);
    root->addLayout(formatRow);
    connect(ui->lineEdit_path, &QLineEdit::textEdited, this, [this]() { resetFormat(); });

    // data type / platform side by side
    auto *pair = new QHBoxLayout;
    pair->setSpacing(14);
    pair->addLayout(FormKit::field(tr("数据类型"), ui->comboBox_datatype), 1);
    pair->addLayout(FormKit::field(tr("测量平台"), ui->comboBox_platform), 1);
    root->addLayout(pair);

    // name in the project
    auto *prefix = new QLabel(QStringLiteral("real_"));
    FormKit::setRole(prefix, "mono");
    ui->lineEdit_tablename->setPlaceholderText(tr("例如 line01"));
    auto *nameRow = new QHBoxLayout;
    nameRow->setSpacing(6);
    nameRow->addWidget(prefix);
    nameRow->addWidget(ui->lineEdit_tablename, 1);
    root->addLayout(FormKit::field(tr("数据名称"), nameRow, tr("在工程中显示的名称，文件后缀会自动加上。")));

    ui->lineEdit_3->setPlaceholderText(tr("可选"));
    root->addLayout(FormKit::field(tr("备注"), ui->lineEdit_3));
    root->addStretch(1);

    // footer
    auto *cancel = new QPushButton(tr("取消"));
    connect(cancel, &QPushButton::clicked, this, &QWidget::close);
    ui->pushButton_confirm->setText(tr("导入"));
    ui->pushButton_confirm->setDefault(true);
    FormKit::setRole(ui->pushButton_confirm, "primary");
    ui->pushButton_confirm->setMinimumWidth(96);
    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addStretch(1);
    footer->addWidget(cancel);
    footer->addWidget(ui->pushButton_confirm);
    root->addLayout(footer);

    // swap: the old layout first, then adopt the new one (which re-parents every widget used above),
    // and only then remove the now empty group boxes
    delete layout();
    setLayout(root);
    delete ui->groupBox;
    delete ui->groupBox_2;
}

ImportForm::~ImportForm()
{
    delete ui;
}

bool ImportForm::appendTextToFile(const QString &textToAdd)
{
    txtPath = projectPath +"/database.txt";
    QFile file(txtPath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
    {
        // 如果文件打开失败（比如因为文件不存在且没有写权限），则尝试创建文件
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "无法打开文件以写入内容：" << file.errorString();
            return false;
        }
    }
    QTextStream out(&file);
    out << textToAdd << "\n";
    file.close();
    return true;
}

void ImportForm::resetFormat()
{
    formatPath_.clear();
    formatSummary_->setText(tr("尚未设置数据格式"));
    formatButton_->setEnabled(!ui->lineEdit_path->text().trimmed().isEmpty());
}

bool ImportForm::chooseFormat()
{
    const QString path = ui->lineEdit_path->text().trimmed();
    if (path.isEmpty() || !QFileInfo::exists(path))
    {
        QMessageBox::warning(this, "警告", "请选择一个存在的数据文件");
        return false;
    }
    DataImportDialog dialog(path, formatPath_ == path ? importSettings_ : DataIO::ImportSettings(), this);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    importSettings_ = dialog.settings();
    formatPath_ = path;
    formatSummary_->setText(dialog.summary());
    return true;
}

void ImportForm::on_pushButton_choose_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "选择文件", QDir::homePath(),
        "数据文件 (*.txt *.csv *.dat *.xyz *.cdf *.nc *.grd);;文本文件 (*.txt *.csv *.dat *.xyz);;"
        "CDF / netCDF (*.cdf *.nc *.grd);;所有文件 (*)");
    if (filePath.isEmpty())
        return;
    ui->lineEdit_path->setText(filePath);
    if (ui->lineEdit_tablename->text().trimmed().isEmpty())
        ui->lineEdit_tablename->setText(QFileInfo(filePath).completeBaseName());   // sensible default name
    resetFormat();
    chooseFormat();
}


void ImportForm::on_pushButton_confirm_clicked()
{
    QString TableName = ui->lineEdit_tablename->text().trimmed();
    if (TableName.isEmpty())
    {
        QMessageBox::warning(this,"警告","请为导入数据命名");
        return;
    }
    TableName = "real_"+TableName;

    QString filePath = ui->lineEdit_path->text().trimmed();
    if(filePath.isEmpty() || !QFileInfo::exists(filePath))
    {
        QMessageBox::warning(this,"警告","请选择一个存在的数据文件");
        return;
    }
    if (QFileInfo::exists(projectPath + "/Measured/" + TableName + ".txt"))
    {
        QMessageBox::warning(this, "警告", QString("工程中已有名为 %1 的数据，请换一个名称").arg(TableName));
        ui->lineEdit_tablename->setFocus();
        return;
    }
    if (formatPath_ != filePath && !chooseFormat())
        return;

    // the file is converted to the program's format ("x y value" per line) and then added to the project
    QTemporaryDir tmp;
    const QString converted = tmp.filePath("import.txt");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const DataIO::ImportResult r = DataIO::importToXyz(filePath, importSettings_, converted);
    QApplication::restoreOverrideCursor();
    if (!r.ok())
    {
        QMessageBox::warning(this, "导入失败", r.error);
        return;
    }
    TableName += ".txt";
    double dx = ui->doubleSpinBox_x->value();
    double dy = ui->doubleSpinBox_y->value();
    const QString dateTimeString = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    appendTextToFile(QString::number(ui->comboBox_datatype->currentIndex()) + " "
                     + TableName + " "
                     + filePath + " "
                     + dateTimeString + " "
                     + QString::number(r.xMin) + " "
                     + QString::number(r.xMax) + " "
                     + QString::number(r.yMin) + " "
                     + QString::number(r.yMax) + " "
                     + QString::number(ui->comboBox_platform->currentIndex()));
    // 传输参数（主窗口把转换后的文件复制到工程的 Measured 目录）
    inputReceived(TableName, converted, dx, dy);
    QString msg = QString("数据 %1 已导入：%2 个点").arg(TableName).arg(r.written);
    if (r.skipped > 0)
        msg += QString("，跳过 %1 行（%2）").arg(r.skipped).arg(r.skippedExamples.join("；"));
    emit textUpdated(msg);
    for (const QString &note : r.notes)
        emit textUpdated("  " + note);
    if (r.skipped > r.written)
        QMessageBox::warning(this, "跳过的行较多", QString("导入 %1 个点，跳过 %2 行。如果不是文件本身缺少数据（填充值），请检查列设置。\n%3")
                                                       .arg(r.written).arg(r.skipped).arg(r.skippedExamples.join("\n")));
    QCoreApplication::processEvents();
    // 加入数据库
    if (!ui->checkBox->isChecked())
    {
        ui->lineEdit_path->clear();          // the next import starts from an empty form
        ui->lineEdit_tablename->clear();
        resetFormat();
        close();
        return;
    }
}

void ImportForm::dataInput(const QString &tablename,const QString &input,double &dx,double &dy)
{
    emit inputReceived(tablename,input,dx,dy);
}

