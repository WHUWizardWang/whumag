#include "inputpara_rm.h"

int checkAndSaveFile(const QString &folderPath,const QString &fileName)
{
    // 构造完整的文件路径
    QString fullFilePath = QDir(folderPath).filePath(fileName);
    // 使用QFile或QDir检查文件是否存在
    QFile file(fullFilePath);
    if (file.exists())
    {
        // 如果文件已存在，则弹出错误对话框
        QMessageBox::critical(nullptr, "Error",
                              QString("文件已存在 '%1' 已存在。 \n 请重新设置参数。").arg(fileName));
        return -1;
    }
    else
    {
        return 0;
    }
}

int inputPara_taylor(QDialog &dialog,int &order,QString &filename,
                     int &data_num,QStringList &data_name_list,int &data_index,QString dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("泰勒多项式-输入参数: ");
    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // #2
    QSpinBox *spinbox1 = new QSpinBox(&dialog);
    form.addRow("阶数: ", spinbox1);
    // #3
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    QLabel *label1 = new QLabel("taylor_");
    QLabel *label2 = new QLabel(".txt");
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(label1);
    horizontalLayout->addWidget(lineEdit);
    horizontalLayout->addWidget(label2);
    form.addRow("另存为: ", horizontalLayout);
    // #4
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        order = spinbox1->value();
        data_index = comboBox->currentIndex();
        filename = "taylor_"+lineEdit->text()+".txt";
        int ret = checkAndSaveFile(dir+"/Processed",filename);
        if (ret == -1)
            return -1;
        return 0;
    }
    else
        return -1;
}

int inputPara_polyhedral(QDialog &dialog,short &type,double &para,QString &filename,
                         int &data_num,QStringList &data_name_list,int &data_index,QString dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("多面函数-输入参数: ");
    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // #2
    QRadioButton *radioButton1 = new QRadioButton("正双曲函数");
    QRadioButton *radioButton2 = new QRadioButton("反双曲函数");
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(radioButton1);
    horizontalLayout->addWidget(radioButton2);
    form.addRow("多面函数选择: ", horizontalLayout);
    // #3
    QString value2 = QString("平滑因子: ");
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    form.addRow(value2, spinbox1);
    // #4
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    form.addRow("另存为: ", lineEdit);
    // #5
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        if (radioButton1->isChecked())
            type = 0;
        else if(radioButton2->isChecked())
            type = 1;
        else
            return -2;
        para = spinbox1->value();
        data_index = comboBox->currentIndex();
        filename = lineEdit->text();
        int ret = checkAndSaveFile(dir+"/Processed",filename);
        if (ret == -1)
            return -1;
        return 0;
    }
    else
        return -1;
}

int inputPara_spline(QDialog &dialog,double &c,int &s,QString &filename,
                     int &data_num,QStringList &data_name_list,int &data_index,QString dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("样条函数-输入参数: ");
    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // #2
    QString value1 = QString("曲率: ");
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    form.addRow(value1, spinbox1);
    spinbox1->setDecimals(2);
//    spinbox1->setMinimum(0.0);
//    spinbox1->setMaximum(100.0);
    // #3
    QString value2 = QString("弹性稀疏度: ");
    QSpinBox *spinbox2 = new QSpinBox(&dialog);
    form.addRow(value2, spinbox2);
    spinbox2->setMaximum(99999);
    // #4
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    form.addRow("另存为: ", lineEdit);
    // #5
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        c = spinbox1->value();
        s = spinbox2->value();
        data_index = comboBox->currentIndex();
        filename = lineEdit->text();
        int ret = checkAndSaveFile(dir+"/Processed",filename);
        if (ret == -1)
            return -1;
        return 0;
    }
    else
        return -1;
}

int inputPara_compress(QDialog &dialog,double &n_nonzero_coefs,QString &filename,
                       int &data_num,QStringList &data_name_list,int &data_index,QString dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("压缩感知-输入参数: ");
    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // #2
    QString value1 = QString("稀疏度: ");
    QSpinBox *spinbox1 = new QSpinBox(&dialog);
    spinbox1->setMaximum(99999);
    form.addRow(value1, spinbox1);
    // #3
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    form.addRow("另存为: ", lineEdit);
    // #4
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        n_nonzero_coefs = spinbox1->value();
        data_index = comboBox->currentIndex();
        filename = lineEdit->text();
        int ret = checkAndSaveFile(dir+"/Processed",filename);
        if (ret == -1)
            return -1;
        return 0;
    }
    else
        return -1;
}

int inputPara_lssvmpso(QDialog &dialog,QString &filename,
                       int &data_num,QStringList &data_name_list,int &data_index,QString dir)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("LSSVMPSO-输入参数: ");
    // #1
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    // #2
    form.addRow("选择数据: ",comboBox);
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    form.addRow("另存为: ", lineEdit);
    // #3
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        data_index = comboBox->currentIndex();
        filename = lineEdit->text();
        int ret = checkAndSaveFile(dir+"/Processed",filename);
        if (ret == -1)
            return -1;
        return 0 ;
    }
    else
        return -1;
}
