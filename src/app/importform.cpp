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
//    if (file_suffix == ".txt" || file_suffix == ".dat")
//    {
//        xyz_toSQL(fileinfo,TableName);
//        emit textUpdated("数据%"+TableName+"已成功导入至数据库!");
//        QCoreApplication::processEvents();
//        return;
//    }
//    else if (file_suffix == ".grd")
//    {
//        grd_toSQL(fileinfo,TableName);
//        emit textUpdated("数据"+TableName+"已成功导入至数据库!");
//        QCoreApplication::processEvents();
//        return;
//    }

}

void ImportForm::dataInput(const QString &tablename,const QString &input,double &dx,double &dy)
{
    emit inputReceived(tablename,input,dx,dy);
}

void ImportForm::xyz_toSQL(QFileInfo fileinfo,QString TableName)
{
    TableName = TableName.left(TableName.lastIndexOf('.'));
    if (DatabaseManager::instance().initConnection())
    {
        qDebug() << "Database connection successful!";
        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        if (db.open())
        {
            qDebug()<<"seccess!";
        } else
        {
            qDebug()<<"failed!";
            qDebug()<<db.lastError();
        }
        // 判断是否存在 表TableName
        int flag =0;
        QString str = QString("select count(*) from information_schema.tables where table_name='%1';").arg(TableName) ;
        query.prepare(str);
        query.exec();
        if(query.first())
        {
            flag = query.value(0).toInt();
        }
        if(flag)
        {
            qDebug()<<"Table already exists !";
            return;
        }
        else
        {
            str = QString("CREATE TABLE %1 (x double precision,y double precision, z double precision);").arg(TableName);
            query.exec(str);
            QString str = QString("select count(*) from information_schema.tables where table_name='%1';").arg(TableName);
            query.prepare(str);
            query.exec();
            if(query.first())
            {
                qDebug()<<"create tabel success!";
            }
            // 数据写入
            QFile file(fileinfo.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                qDebug() << "Could not open file!";
                return;
            }
            QTextStream in(&file);
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList fields = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                double x = fields.at(0).toDouble();
                double y = fields.at(1).toDouble();
                double z = fields.at(2).toDouble();
                QString str = QString("INSERT INTO %4 VALUES(%1,%2,%3);")
                    .arg(x).arg(y).arg(z).arg(TableName);
//                qDebug()<<str;
                query.prepare(str);
                if(!query.exec())
                {
                    qDebug()<<"query error :"<<query.lastError();
                }
            }
            file.close();
        }
        qDebug() << "finish!";
        close();
    }
}

void ImportForm::grd_toSQL(QFileInfo fileinfo,QString TableName)
{
    QStringList shell_str;
    shell_str <<"-c";
    shell_str <<  QString("raster2pgsql -s 4326 -I -C %1 %2 | PGPASSWORD=123456 psql -h 127.0.0.1 -d whumag -U postgres")
            .arg(fileinfo.absoluteFilePath()).arg(TableName);
    QProcess process;
    process.start("/bin/bash",shell_str);
    process.waitForFinished(-1); // 等待命令执行完成，-1表示无限期等待
    QString output = process.readAllStandardOutput(); // 读取标准输出
    QString errorOutput = process.readAllStandardError(); // 读取错误输出
    qDebug()<<errorOutput;
}
