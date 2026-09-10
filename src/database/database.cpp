#include "database.h"
#include "ui_database.h"
#pragma execution_character_set("utf-8")

database::database(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::database)
{
    // 1.<设置表格内容>
    ui->setupUi(this);
//    if (DatabaseManager::instance().initConnection())
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
        // 设置表格模型
        model = new QSqlTableModel(this);
        model->setTable("file_metadata"); // 指定使用表格
        // 把model放在view里面
        ui->tableView->setModel(model);
        // 显示model里面的语句
        model->select();
        ui->tableView->setItemDelegateForColumn(0,new EnumComboBoxDelegate1(model));
        ui->tableView->setItemDelegateForColumn(8,new EnumComboBoxDelegate2(model));
        model->setHeaderData(0,Qt::Horizontal,"数据类型");
        model->setHeaderData(1,Qt::Horizontal,"数据名称");
        model->setHeaderData(2,Qt::Horizontal,"保存路径");
        model->setHeaderData(3,Qt::Horizontal,"导入时间");
        model->setHeaderData(4,Qt::Horizontal,"min_X");
        model->setHeaderData(5,Qt::Horizontal,"max_X");
        model->setHeaderData(6,Qt::Horizontal,"min_Y");
        model->setHeaderData(7,Qt::Horizontal,"max_Y");
        model->setHeaderData(8,Qt::Horizontal,"测量平台");
        // 设置表格的列宽
        ui->tableView->setColumnWidth(0,100);
        ui->tableView->setColumnWidth(1,200);
        ui->tableView->setColumnWidth(2,500);
        ui->tableView->setColumnWidth(3,200);
        ui->tableView->setColumnWidth(4,100);
        ui->tableView->setColumnWidth(5,100);
        ui->tableView->setColumnWidth(6,100);
        ui->tableView->setColumnWidth(7,100);
        ui->tableView->setColumnWidth(8,100);
        // 设置model编辑模式为手动提交修改
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        // 设置view中的数据库不允许修改
        // ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    // 2.<设置comboBox内容>
    ui->comboBox_datatype->addItem("实测数据");
    ui->comboBox_datatype->addItem("处理后数据");
    ui->comboBox_platform->addItem("船磁");
    ui->comboBox_platform->addItem("航磁");
    ui->comboBox_platform->addItem("水下磁测");
}

database::~database()
{
    delete ui;
}

void database::on_pushButton_add_clicked()
{
    // 添加空记录
    QSqlRecord record = model->record();
    // 获取行号
    int row = model->rowCount();
    model->insertRecord(row,record);
    // 打开文件路径
    QString filename = QFileDialog::getOpenFileName(this, tr("请打开文件"),
                                               QCoreApplication::applicationFilePath(),
                                               "*.*");
    if (filename.isEmpty())
    {
        model->select();    // 未选择文件，则刷新界面并返回
        return;
    }
    // 设置表格中对应行号的第三列为获取到的路径
    rows.push_back(row);
    QModelIndex index = model->index(row,2);
    model->setData(index,filename);
    // 该文件的区域大小
    double xmin,xmax,ymin,ymax;
    openData(filename,xmin,xmax,ymin,ymax);
    // 设置表格中对应行号的4567列为区域大小
    QModelIndex index1 = model->index(row,4);
    model->setData(index1,QString::number(xmin));
    QModelIndex index2 = model->index(row,5);
    model->setData(index2,QString::number(xmax));
    QModelIndex index3 = model->index(row,6);
    model->setData(index3,QString::number(ymin));
    QModelIndex index4 = model->index(row,7);
    model->setData(index4,QString::number(ymax));
}

void database::on_pushButton_delete_clicked()
{
    // 取出选中模型
    QItemSelectionModel *sModel =  ui->tableView->selectionModel();
    // 取出模型中的索引
    QModelIndexList list = sModel->selectedRows();
    // 删除所有选中的行
    for(int i=0;i<list.size();i++)
        model->removeRow(list.at(i).row());
}

void database::on_pushButton_update_clicked()
{
    // 更改输入时间，点击确定之后才做修改，点击增加数据尚未导入数据库中
    for (int i = 0;i<rows.size();i++)
    {
        QModelIndex index = model->index(rows.at(i),3);
        model->setData(index,QDateTime::currentDateTime());
    }
    rows.clear();
    if(model->submitAll())          // 开启事务
        model->database().commit(); // 提交事务
    else                            // 回滚事务
    {
        model->database().rollback();
        QMessageBox::warning(this,tr("TableView"),tr("数据库错误： %1").arg(model->lastError().text()));
    }
}

void database::on_pushButton_show_clicked()
{
    // 取出选中模型
    QItemSelectionModel *sModel =  ui->tableView->selectionModel();
    // 取出模型中的索引
    QModelIndexList list = sModel->selectedRows();
    if (list.size() < 1)
        return;
    for(int i=0;i<list.size();i++)
    {
        draw_Form *draw_form_ = new draw_Form;
        QVector<double> xx,yy,zz;
        QString filepath = model->data(model->index(list.at(i).row(),2)).toString();
        draw_form_->create_xyz_f(filepath,xx,yy,zz);
//        draw_form_->set_HeatOrSactterView(xx,yy,zz);
        draw_form_->autoset_heatMapView(xx,yy,zz);
        if (!draw_form_->magWarn)
            return;
        if (proPath.isEmpty())
            draw_form_->autoset_contourView(xx,yy,zz);
        else
            draw_form_->autoset_contourView(xx,yy,zz);
        draw_form_->show();
    }
}

void database::on_pushButton_cancel_clicked()
{
    model->revertAll(); // 取消所有动作
    model->submitAll(); // 提交动作
}

void database::on_pushButton_filter_clicked()
{
    QStringList filterstr_list;  //查询条件
    QString datatype = ui->comboBox_datatype->currentText();
    //数据结构
    if(!datatype.isEmpty())
    {
        QStringList datatype_list = datatype.split(";");
        for(int i=0;i<datatype_list.size();i++)
        {
            datatype_list[i] = "'" + datatype_list[i] +"'";
        }
        QString str = datatype_list.join(",");
        filterstr_list.push_back("datatype IN (" + str +")");
    }
    //测量平台
    QString platform = ui->comboBox_platform->currentText();
    if(!platform.isEmpty())
    {
        QStringList platform_list = platform.split(";");
        for(int i=0;i<platform_list.size();i++)
        {
            platform_list[i] = "'" + platform_list[i] +"'";
        }
        QString str = platform_list.join(",");
        filterstr_list.push_back("platform IN (" + str +")");
    }
    //数据名称
    QString name = ui->lineEdit_name->text();
    if(!name.isEmpty())
    {
        filterstr_list.push_back(QObject::tr("name LIKE '%%1%'").arg(name));
    }
    //数据路径
    QString path = ui->lineEdit_path->text();
    if(!path.isEmpty())
    {
        filterstr_list.push_back(QObject::tr("path LIKE '%%1%'").arg(path));
    }
    //x范围
    if(ui->checkBox_x->isChecked())
    {
        QString xmin = QString::number(ui->doubleSpinBox_xmin->value());
        QString xmax = QString::number(ui->doubleSpinBox_xmax->value());
        if (xmin.isEmpty() && xmax.isEmpty())
        {
            return;
        }
        else if (!xmax.isEmpty())
        {
            //xmax有值，xmin无值
            filterstr_list.push_back(QObject::tr("max_X BETWEEN '%1' AND '%2'").arg(xmin).arg(xmax));
        }
        else if (!xmin.isEmpty())
        {
            //xmin有值，xmax无值
            filterstr_list.push_back(QObject::tr("min_X BETWEEN '%1' AND '%2'").arg(xmin).arg(xmax));
        }
    }
    //y范围
    if(ui->checkBox_y->isChecked())
    {
        QString ymin = QString::number(ui->doubleSpinBox_ymin->value());
        QString ymax = QString::number(ui->doubleSpinBox_ymax->value());
        if (ymin.isEmpty() && ymax.isEmpty())
        {
            return;
        }
        else if (!ymax.isEmpty())
        {
            //xmax有值，xmin无值
            filterstr_list.push_back(QObject::tr("max_Y BETWEEN '%1' AND '%2'").arg(ymin).arg(ymax));
        }
        else if (!ymin.isEmpty())
        {
            //xmin有值，xmax无值
            filterstr_list.push_back(QObject::tr("min_Y BETWEEN '%1' AND '%2'").arg(ymin).arg(ymax));
        }
    }
    //时间范围
    if(ui->checkBox_time->isChecked())
    {
        QString timebegin = ui->dateTimeEdit_begin->text();
        QString timeend = ui->dateTimeEdit_end->text();
        if (timebegin.isEmpty() && timeend.isEmpty())
        {
            return;
        }
        else if (!timeend.isEmpty())
        {
            //xmax有值，xmin无值
            filterstr_list.push_back(QObject::tr("time BETWEEN '%1' AND '%2'").arg(timebegin).arg(timeend));
        }
        else if (!timebegin.isEmpty())
        {
            //xmin有值，xmax无值
            filterstr_list.push_back(QObject::tr("time BETWEEN '%1' AND '%2'").arg(timebegin).arg(timeend));
        }

    }
    QString filterstr = filterstr_list.join(" AND ");
    model->setFilter(filterstr);
    model->select();
}

void database::openData(QString filePath,double &xmin,double &xmax,double &ymin,double &ymax)
{
    QVector<double> xx,yy,vv;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
           return;
    QTextStream in(&file);
    int index=0;
    while (!in.atEnd())
    {
        index++;
        QString line = in.readLine();
        if (line.isEmpty())
            continue;
        QStringList fields = line.split(QRegExp("\\s+|,|;"), QString::SkipEmptyParts);
        if (fields.size() != 3)
        {
            qDebug()<<"错误1(过少或过多): 第"<<QString::number(index)<<"行 "<<line;
            continue;
        }
        if (!isNumeric(fields[0])||!isNumeric(fields[1])||!isNumeric(fields[2]))
        {
            qDebug()<<"错误2(有其他字符): 第"<<QString::number(index)<<"行 "<<line;
            continue;
        }
        double x = fields[0].toDouble();
        double y = fields[1].toDouble();
        double value = fields[2].toDouble();
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

void database::on_pushButton_cancelfilter_clicked()
{
    model->setFilter("");
    model->select();
}
