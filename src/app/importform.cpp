#include "importform.h"
#include "ui_importform.h"

#include "formkit.h"
#include "uiscale.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

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
    resize(UiScale::windowSize(600, 440));
    setMinimumWidth(UiScale::dp(520));

    auto *root = new QVBoxLayout;
    root->setContentsMargins(26, 22, 26, 18);
    root->setSpacing(16);
    root->addWidget(FormKit::header(QStringLiteral("import"), tr("导入数据"), tr("把测线数据文件加入当前工程")));

    // file
    ui->lineEdit_path->setPlaceholderText(tr("选择 .txt 或 .dat 数据文件"));
    ui->pushButton_choose->setText(tr("选择文件…"));
    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(ui->lineEdit_path, 1);
    pathRow->addWidget(ui->pushButton_choose);
    root->addLayout(FormKit::field(tr("数据文件"), pathRow, tr("每行一个点：经度 纬度 磁场值，以空格分隔。")));

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

void ImportForm::openData(QString filePath,double &xmin,double &xmax,double &ymin,double &ymax)
{
    QVector<double> xx,yy,vv;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
           return;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList fields = line.split(" "); // 假设CSV字段由逗号分隔
        if (fields.size() < 3)
                continue;
        bool okX, okY, okValue;
        double x = fields[0].toDouble(&okX);
        double y = fields[1].toDouble(&okY);
        double value = fields[2].toDouble(&okValue);

        if (!okX || !okY || !okValue)
                continue;
        xx.append(x);
        yy.append(y);
        vv.append(value);
    }
    file.close();
    if (xx.isEmpty() || yy.isEmpty())
    {
        QMessageBox::warning(this, "警告", "所选文件未包含可解析的数据行");
        xmin = xmax = ymin = ymax = 0.0;
        return;
    }
    xmin = *std::min_element(std::begin(xx),std::end(xx));
    xmax = *std::max_element(std::begin(xx),std::end(xx));
    ymin = *std::min_element(std::begin(yy),std::end(yy));
    ymax = *std::max_element(std::begin(yy),std::end(yy));
}

void ImportForm::on_pushButton_choose_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", QDir::homePath(), "文本文件 (*.txt);;数据文件 (*.dat)");
    if (!filePath.isEmpty())
    {
        ui->lineEdit_path->setText(filePath);
        if (ui->lineEdit_tablename->text().trimmed().isEmpty())
            ui->lineEdit_tablename->setText(QFileInfo(filePath).completeBaseName());   // sensible default name
    }
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

    QString filePath = ui->lineEdit_path->text();
    if(filePath.isEmpty() || !QFileInfo::exists(filePath))
    {
        QMessageBox::warning(this,"警告","请选择一个存在的数据文件");
        return;
    }
    QFileInfo fileinfo = QFileInfo(filePath);
    // 文件后缀
    QString file_suffix = fileinfo.suffix();
    file_suffix="."+file_suffix;
    // 文件名
    QString file_name = fileinfo.fileName();
    file_name.replace(file_name.size()-file_suffix.size(),file_suffix.size(),"");
    // 绝对路径
    QString file_path = fileinfo.absolutePath();
    //
    TableName = TableName+file_suffix;
    double dx = ui->doubleSpinBox_x->value();
    double dy = ui->doubleSpinBox_y->value();
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString dateTimeString = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");  // 转换为字符串形式
    double xmin,xmax,ymin,ymax;
    openData(filePath,xmin,xmax,ymin,ymax);
    appendTextToFile(QString::number(ui->comboBox_datatype->currentIndex()) + " "
                     + TableName + " "
                     + filePath + " "
                     + dateTimeString + " "
                     + QString::number(xmin) + " "
                     + QString::number(xmax) + " "
                     + QString::number(ymin) + " "
                     + QString::number(ymax) + " "
                     + QString::number(ui->comboBox_platform->currentIndex()));
    // 传输参数
    inputReceived(TableName,fileinfo.absoluteFilePath(),dx,dy);
    emit textUpdated("数据 "+TableName+" 已成功导入至工程!");
    QCoreApplication::processEvents();
    // 加入数据库
    if (!ui->checkBox->isChecked())
    {
        ui->lineEdit_path->clear();          // the next import starts from an empty form
        ui->lineEdit_tablename->clear();
        close();
        return;
    }
}

void ImportForm::dataInput(const QString &tablename,const QString &input,double &dx,double &dy)
{
    emit inputReceived(tablename,input,dx,dy);
}

