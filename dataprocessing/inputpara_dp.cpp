#include "dataprocessing/inputpara_dp.h"
#include "mainwindow.h"

int inputPara_up(QDialog &dialog,double &h,
                  int &data_num,QStringList &data_name_list,int &data_index,QString &dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("向上延拓-输入参数: ");
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // Value1
    QLabel *label1 = new QLabel("经度/X") ;
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    QLabel *label2 = new QLabel("纬度/Y") ;
    QDoubleSpinBox *spinbox2 = new QDoubleSpinBox(&dialog);
    spinbox1->setDecimals(2);
    spinbox2->setDecimals(2);
    spinbox1->setValue(0.5);
    spinbox2->setValue(0.5);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(label1);
    horizontalLayout->addWidget(spinbox1);
    horizontalLayout->addWidget(label2);
    horizontalLayout->addWidget(spinbox2);
    form.addRow("格网分辨率:    ", horizontalLayout);
    // Value2
    QDoubleSpinBox *spinbox3 = new QDoubleSpinBox(&dialog);
    spinbox3->setRange(0.0,9999.0);
    spinbox3->setDecimals(2);
    spinbox3->setValue(0.5);
    QLabel *lineEdit = new QLabel("KM");
    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addWidget(spinbox3);
    horizontalLayout2->addWidget(lineEdit);
    form.addRow("延拓高度: ",horizontalLayout2);
    // #4
    QLineEdit *linEdit1= new QLineEdit;
    linEdit1->setText("up.txt");
    form.addRow("保存文件名: ",linEdit1);
    // Add Cancel and OK button
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
//        step_lon = spinbox1->value();
//        step_lat = spinbox2->value();
        if (comboBox->count()==0)
            return -1;
        h = spinbox3->value();
        data_index = comboBox->currentIndex();
        dir = linEdit1->text();
        return 0;
    }
    return -1;
}
int inputPara_down(QDialog &dialog,double &h,int &type,
                    int &data_num,QStringList &data_name_list,int &data_index,QString &dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("向下延拓-输入参数: ");
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // Value1
    QLabel *label1 = new QLabel("经度/X") ;
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    QLabel *label2 = new QLabel("纬度/Y") ;
    QDoubleSpinBox *spinbox2 = new QDoubleSpinBox(&dialog);
    spinbox1->setDecimals(2);
    spinbox2->setDecimals(2);
    spinbox1->setValue(0.5);
    spinbox2->setValue(0.5);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(label1);
    horizontalLayout->addWidget(spinbox1);
    horizontalLayout->addWidget(label2);
    horizontalLayout->addWidget(spinbox2);
    form.addRow("格网分辨率:    ", horizontalLayout);
    // Value2
    QDoubleSpinBox *spinbox3 = new QDoubleSpinBox(&dialog);
    spinbox3->setRange(0.0,9999.0);
    spinbox3->setDecimals(2);
    spinbox3->setValue(0.5);
    QLabel *lineEdit = new QLabel("KM");
    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addWidget(spinbox3);
    horizontalLayout2->addWidget(lineEdit);
    form.addRow("延拓高度: ",horizontalLayout2);
    // Value3
    QComboBox *comboBox1 = new QComboBox();
    comboBox1->addItem("Tikhonov正则化法");
    comboBox1->addItem("积分迭代法");
    comboBox1->addItem("Landweber迭代法");
    comboBox1->addItem("迭代Tikhonov正则化法");
    form.addRow("延拓因子:    ", comboBox1);
    // #4
    QLineEdit *linEdit1 = new QLineEdit;
    linEdit1->setText("down.txt");
    form.addRow("保存文件名: ",linEdit1);
    // Add Cancel and OK button
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
//        step_lon = spinbox1->value();
//        step_lat = spinbox2->value();
        if (comboBox->count()==0)
            return -1;
        h = spinbox3->value();
        type = comboBox1->currentIndex();
        data_index = comboBox->currentIndex();
        dir = linEdit1->text();
        return 0;
    }
    return -1;
}

int inputPara_correct(QDialog &dialog,QDate &date0,QDate &date1,int &useGeoid,double &height,
                      int &data_num,QStringList &data_name_list,int &data_index,QString &dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("通化改正-输入参数: ");
    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // #2
    QDateEdit *dateEdit0 = new QDateEdit();
    QDateEdit *dateEdit1 = new QDateEdit();
    dateEdit0->setDate(QDate::currentDate().addDays(-360));
    dateEdit1->setDate(QDate::currentDate());
    form.addRow("通化前时间: ", dateEdit0);
    form.addRow("通化后时间: ", dateEdit1);
    // #3
    QComboBox *comboBox1 = new QComboBox;
    comboBox1->addItem("IGRF");
    form.addRow("选择磁场模型: ",comboBox1);
    // #4
    QLineEdit *linEdit = new QLineEdit;
    linEdit->setText("tonghua.txt");
    form.addRow("保存文件名: ",linEdit);
    // #5
    QComboBox *comboBox2 = new QComboBox;
    comboBox2->addItem("E");
    comboBox2->addItem("M");
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    spinbox1->setRange(0.0,9999.0);
    spinbox1->setValue(0.5);
    QLabel *lineEdit = new QLabel("KM");
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(comboBox2);
    horizontalLayout->addWidget(spinbox1);
    horizontalLayout->addWidget(lineEdit);
    form.addRow("高度: ",horizontalLayout);
    // Add Cancel and OK button
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        date0 = dateEdit0->date();
        date1 = dateEdit1->date();
        if (comboBox->count()==0)
            return -1;
        useGeoid = comboBox1->currentIndex();
        height = spinbox1->value()*1000;
        data_index = comboBox->currentIndex();
        dir = linEdit->text();
        return 0;
    }
    return -1;
}
