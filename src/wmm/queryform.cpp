#include "queryform.h"
#include "ui_queryform.h"

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
    for (int i = 0; i < length; ++i)
    {
        QStandardItem *item;
        item = new QStandardItem(QString::number(CoordGeodeticArr[i].phi, 'f', 6));
        table_model_->setItem(i, 0, item);
        item = new QStandardItem(QString::number(CoordGeodeticArr[i].lambda, 'f', 6));
        table_model_->setItem(i, 1, item);

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
        item = new QStandardItem(height_str);
        table_model_->setItem(i, 2, item);

        // Date
        QDate date(UserDateArr[i].Year, UserDateArr[i].Month, UserDateArr[i].Day);
        item = new QStandardItem(date.toString("yyyy/MM/dd"));
        table_model_->setItem(i, 3, item);

        // Magnetic elements
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].F, 'f', 1));
        table_model_->setItem(i, 4, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Fdot, 'f', 1));
        table_model_->setItem(i, 5, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].H, 'f', 1));
        table_model_->setItem(i, 6, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Hdot, 'f', 1));
        table_model_->setItem(i, 7, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].X, 'f', 1));
        table_model_->setItem(i, 8, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Xdot, 'f', 1));
        table_model_->setItem(i, 9, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Y, 'f', 1));
        table_model_->setItem(i, 10, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Ydot, 'f', 1));
        table_model_->setItem(i, 11, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Z, 'f', 1));
        table_model_->setItem(i, 12, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Zdot, 'f', 1));
        table_model_->setItem(i, 13, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Decl, 'f', 1));
        table_model_->setItem(i, 14, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Decldot, 'f', 1));
        table_model_->setItem(i, 15, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Incl, 'f', 1));
        table_model_->setItem(i, 16, item);
        item = new QStandardItem(QString::number(GeoMagneticElementsArr[i].Incldot, 'f', 1));
        table_model_->setItem(i, 17, item);
    }
}

void QueryForm::on_radioButton_point_toggled(bool checked)
{
    point_form_->setVisible(checked);
}

void QueryForm::on_radioButton_file_toggled(bool checked)
{
    file_form_->setVisible(checked);
}

void QueryForm::on_radioButton_grid_toggled(bool checked)
{
    grid_form_->setVisible(checked);
}

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

        ui->textBrowser->append(QTime::currentTime().toString("hh:mm:ss") + "\t计算中，请等待......");

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

        if (ret == 0)
        {
            fill_item_model(CoordGeodeticArr, UserDateArr, GeoMagneticElementsArr, length);
            ui->textBrowser->append(QTime::currentTime().toString("hh:mm:ss") + "\t完成计算。有效查询数量共计：" + QString::number(length));
        }
        else
        {
            ui->textBrowser->append("COF文件不存在！");
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
    QString save_file_name = QFileDialog::getSaveFileName(this, "保存查询结果", "./query_results.txt", "TXT(*.txt)");
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
    file.close();
    ui->textBrowser->append("导出至 " + save_file_name);
    ui->textBrowser->append("查询结果导出完成。");
}
