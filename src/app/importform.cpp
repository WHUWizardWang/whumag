#include "importform.h"
#include "ui_importform.h"

ImportForm::ImportForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImportForm)
{
    ui->setupUi(this);
    ui->radioButton->setVisible(false);
    ui->shuxing->setVisible(true);
    ui->shujuku->setVisible(false);
    ui->widget_dxdy->setVisible(false);
    ui->comboBox_datatype->addItem("实测数据");
    ui->comboBox_datatype->addItem("处理后数据");
    ui->comboBox_platform->addItem("船磁");
    ui->comboBox_platform->addItem("航磁");
    ui->comboBox_platform->addItem("水下磁测");
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
    }
}


void ImportForm::on_pushButton_confirm_clicked()
{
    QString TableName = ui->lineEdit_tablename->text();
    TableName = "real_"+TableName;

    if (TableName.isEmpty())
    {
        QMessageBox::warning(this,"警告","请为导入数据命名");
        return;
    }
    QString filePath = ui->lineEdit_path->text();
    if(filePath.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择一个文件");
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
        close();
        return;
    }
}

void ImportForm::dataInput(const QString &tablename,const QString &input,double &dx,double &dy)
{
    emit inputReceived(tablename,input,dx,dy);
}

