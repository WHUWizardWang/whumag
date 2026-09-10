#include "anoqueryform.h"
#include "ui_anoqueryform.h"

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
    table_model_->setHorizontalHeaderItem(0, new QStandardItem("X"));
    table_model_->setHorizontalHeaderItem(1, new QStandardItem("Y"));
    table_model_->setHorizontalHeaderItem(2, new QStandardItem("Z"));;
}

void AnoQueryForm::fill_item_model(QVector<AnoPoint> &ano_pnts)
{
    init_tableView();
    for (int i = 0; i < ano_pnts.size(); ++i)
    {
        QStandardItem *item;
        item = new QStandardItem(QString::number(ano_pnts[i].x, 'f', 6));
        table_model_->setItem(i, 0, item);
        item = new QStandardItem(QString::number(ano_pnts[i].y, 'f', 6));
        table_model_->setItem(i, 1, item);
        item = new QStandardItem(QString::number(ano_pnts[i].z, 'f', 6));
        table_model_->setItem(i, 2, item);
    }
}

void AnoQueryForm::on_radioButton_point_toggled(bool checked)
{
    point_form_->setVisible(checked);
}

void AnoQueryForm::on_radioButton_file_toggled(bool checked)
{
    //    file_form_->setVisible(checked);
}

void AnoQueryForm::on_radioButton_grid_toggled(bool checked)
{
    grid_form_->setVisible(checked);
}

void AnoQueryForm::on_pushButton_3_clicked()
{
    QVector<AnoPoint> ano_pnts;
    int length = 0;
    double step_lon,step_lat;
    if (ui->radioButton_point->isChecked())
    {
        length = 1;
        point_form_->toCoordGeodeticArray(ano_pnts);
    }

    else if (ui->radioButton_grid->isChecked())
    {

        if (ui->comboBox_error->currentText() == "EMAG2")
        {
            step_lon = 360.0/10800.0;
            step_lat = 180.0/5400.0;
        }
        else if (ui->comboBox_error->currentText() == "MAMEA")
        {
            step_lon = (160.0-93.0)/2011.0;
            step_lat = (46.0+12.0)/1741.0;
        }
        length = grid_form_->getQueryNum(step_lon,step_lat);
        if (length < 1)
        {
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
    }

    if (length != 0)
    {


        ui->textBrowser->append(QTime::currentTime().toString("hh:mm:ss") + "\t计算中，请等待......");

        int ret = -1;
        if (ui->comboBox_error->currentText() == "EMAG2")
        {
            ret = MagAnoQuery::omg_emag2(ano_pnts);
            if (ano_pnts.size()>4){
            draw_Form *draw_form_ = new draw_Form;
            draw_form_->setAttribute(Qt::WA_DeleteOnClose);
            QVector<double> xx,yy,zz;
            draw_form_->create_xyz_p(ano_pnts,xx,yy,zz);
            draw_form_->autoset_heatMapView(xx,yy,zz);
//          draw_form_->create_contour_txt(ano_pnts);
            draw_form_->autoset_contourView(xx,yy,zz);
            draw_form_->show();}
        }
        else if (ui->comboBox_error->currentText() == "MAMEA")
        {
            ret = MagAnoQuery::omg_mamea(ano_pnts);
            if (ano_pnts.size()>4){
            draw_Form *draw_form_ = new draw_Form;
            draw_form_->setAttribute(Qt::WA_DeleteOnClose);
            QVector<double> xx,yy,zz;
            draw_form_->create_xyz_p(ano_pnts,xx,yy,zz);
            draw_form_->autoset_heatMapView(xx,yy,zz);
//            draw_form_->create_contour_txt(ano_pnts);
            draw_form_->autoset_contourView(xx,yy,zz);
            draw_form_->show();
            }
        }

        if (ret == 0)
        {
            fill_item_model(ano_pnts);
            ui->textBrowser->append(QTime::currentTime().toString("hh:mm:ss") + "\t完成计算。有效查询数量共计：" + QString::number(length));
        }
        else
        {
            ui->textBrowser->append("COF文件不存在！");
        }
    }
}

void AnoQueryForm::on_pushButton_export_clicked()
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
