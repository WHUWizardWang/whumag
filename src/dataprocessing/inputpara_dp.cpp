#include "dataprocessing/inputpara_dp.h"
#include "mainwindow.h"

int inputPara_up(QDialog &dialog,double &h,bool &useBL,
                 int &data_num,QStringList &data_name_list,int &data_index,QString &dir,
                 double &step_x,double &step_y)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("向上延拓-输入参数");

    // 1. 数据选择
    QComboBox *comboBox = new QComboBox;
    for (int i = 0; i < data_num; ++i) {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据：", comboBox);

    // 2. 格网分辨率 (经度/X, 纬度/Y)
    QDoubleSpinBox *spinboxX = new QDoubleSpinBox(&dialog);
    QDoubleSpinBox *spinboxY = new QDoubleSpinBox(&dialog);
    spinboxX->setDecimals(6);
    spinboxY->setDecimals(6);
    spinboxX->setRange(0.000001, 1e6);
    spinboxY->setRange(0.000001, 1e6);
    spinboxX->setValue(0.0045);
    spinboxY->setValue(0.0045);
    QHBoxLayout *gridLayout = new QHBoxLayout;
    gridLayout->addWidget(new QLabel("经度/X"));
    gridLayout->addWidget(spinboxX);
    gridLayout->addSpacing(20);
    gridLayout->addWidget(new QLabel("纬度/Y"));
    gridLayout->addWidget(spinboxY);
    form.addRow("格网分辨率：", gridLayout);

    // 3. 延拓高度
    QDoubleSpinBox *spinboxH = new QDoubleSpinBox(&dialog);
    spinboxH->setRange(0.0001, 9999.0);
    spinboxH->setDecimals(4);
    spinboxH->setValue(0.5);
    QLabel *kmLabel = new QLabel("KM");
    QHBoxLayout *heightLayout = new QHBoxLayout;
    heightLayout->addWidget(spinboxH);
    heightLayout->addWidget(kmLabel);
    form.addRow("延拓高度：", heightLayout);

    // 4. 是否使用 BL 坐标
    QCheckBox *cbUseBL = new QCheckBox("使用 BL 坐标（经纬度，度）", &dialog);
    cbUseBL->setChecked(useBL);  // 如果有默认值
    form.addRow(cbUseBL);
    auto updateUnit = [=]() { kmLabel->setText(cbUseBL->isChecked() ? "KM" : "与坐标同单位"); };
    QObject::connect(cbUseBL, &QCheckBox::toggled, &dialog, updateUnit);
    updateUnit();

    // 5. 输出文件名
    QLineEdit *fileNameEdit = new QLineEdit("up.txt", &dialog);
    form.addRow("保存文件名：", fileNameEdit);

    // 按钮
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted,
                     &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected,
                     &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // 读取值
        step_x     = spinboxX->value();
        step_y     = spinboxY->value();
        h          = spinboxH->value();
        if (comboBox->count() == 0)
            return -1;
        data_index = comboBox->currentIndex();
        dir        = fileNameEdit->text();
        useBL      = cbUseBL->isChecked();  // 将复选框状态写回

        return 0;
    }
    return -1;
}
int inputPara_down(QDialog &dialog,double &h,int &type,bool &useBL,
                    int &data_num,QStringList &data_name_list,int &data_index,QString &dir
                    ,double &step_lon,double &step_lat,bool &autoParameter,double &parameter)
{
    QFormLayout form(&dialog);
    dialog.setWindowTitle("向下延拓-输入参数: ");
    QComboBox *comboBox = new QComboBox;
    for (int i = 0;i<data_num;i++)
    {
        comboBox->addItem(data_name_list[i]);
    }
    form.addRow("选择数据: ",comboBox);
    // grid spacing (degrees with BL coordinates, otherwise the unit of x / y)
    QLabel *label1 = new QLabel("经度/X") ;
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    QLabel *label2 = new QLabel("纬度/Y") ;
    QDoubleSpinBox *spinbox2 = new QDoubleSpinBox(&dialog);
    for (QDoubleSpinBox *s : {spinbox1, spinbox2})
    {
        s->setDecimals(6);
        s->setRange(0.000001, 1e6);
        s->setValue(0.0045);
    }
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(label1);
    horizontalLayout->addWidget(spinbox1);
    horizontalLayout->addWidget(label2);
    horizontalLayout->addWidget(spinbox2);
    form.addRow("格网分辨率:    ", horizontalLayout);
    // height
    QDoubleSpinBox *spinbox3 = new QDoubleSpinBox(&dialog);
    spinbox3->setRange(0.0001,9999.0);
    spinbox3->setDecimals(4);
    spinbox3->setValue(0.5);
    QLabel *unitLabel = new QLabel("KM");
    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addWidget(spinbox3);
    horizontalLayout2->addWidget(unitLabel);
    form.addRow("延拓高度: ",horizontalLayout2);
    // operator
    QComboBox *comboBox1 = new QComboBox();
    comboBox1->addItem("Tikhonov正则化法");
    comboBox1->addItem("积分迭代法");
    comboBox1->addItem("Landweber迭代法");
    comboBox1->addItem("迭代Tikhonov正则化法");
    form.addRow("延拓因子:    ", comboBox1);
    // regularisation parameter: L-curve or manual
    QComboBox *paramMode = new QComboBox();
    paramMode->addItem("自动（L 曲线拐点）");
    paramMode->addItem("手动指定");
    QDoubleSpinBox *paramSpin = new QDoubleSpinBox(&dialog);
    paramSpin->setDecimals(6);
    QLabel *paramLabel = new QLabel;
    QHBoxLayout *paramLayout = new QHBoxLayout();
    paramLayout->addWidget(paramMode, 1);
    paramLayout->addWidget(paramLabel);
    paramLayout->addWidget(paramSpin, 1);
    form.addRow("正则化参数: ", paramLayout);
    QLabel *paramHint = new QLabel;
    paramHint->setWordWrap(true);
    paramHint->setStyleSheet("color: gray;");
    form.addRow(QString(), paramHint);
    auto updateParam = [=]() {
        const bool iterations = comboBox1->currentIndex() == 1 || comboBox1->currentIndex() == 2;
        paramLabel->setText(iterations ? "迭代次数 n" : "α");
        paramSpin->setDecimals(iterations ? 0 : 6);
        paramSpin->setRange(iterations ? 1 : 0.000001, iterations ? 100000 : 1000);
        paramSpin->setValue(iterations ? 13 : 0.95);
        const bool manual = paramMode->currentIndex() == 1;
        paramSpin->setEnabled(manual);
        paramLabel->setEnabled(manual);
        paramHint->setText(manual ? "原程序固定使用 α = 0.95 / n = 13，通常过度平滑。"
                                  : "在一组候选参数上计算 L 曲线，取曲率最大处（拐点）；结果窗口会显示 L 曲线。");
    };
    QObject::connect(comboBox1, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, updateParam);
    QObject::connect(paramMode, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, updateParam);
    updateParam();
    // coordinates
    QCheckBox *cbUseBL = new QCheckBox("使用 BL 坐标（经纬度，度）", &dialog);
    cbUseBL->setChecked(useBL);
    form.addRow(cbUseBL);
    auto updateUnit = [=]() { unitLabel->setText(cbUseBL->isChecked() ? "KM" : "与坐标同单位"); };
    QObject::connect(cbUseBL, &QCheckBox::toggled, &dialog, updateUnit);
    updateUnit();
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
        step_lon = spinbox1->value();
        step_lat = spinbox2->value();
        if (comboBox->count()==0)
            return -1;
        h = spinbox3->value();
        type = comboBox1->currentIndex();
        useBL = cbUseBL->isChecked();
        data_index = comboBox->currentIndex();
        dir = linEdit1->text();
        autoParameter = paramMode->currentIndex() == 0;
        parameter = paramSpin->value();
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
    for (QDateEdit *d : {dateEdit0, dateEdit1})
    {
        d->setDateRange(QDate(1900, 1, 1), QDate(2030, 1, 1));   // IGRF-14
        d->setDisplayFormat("yyyy-MM-dd");
        d->setCalendarPopup(true);
    }
    dateEdit0->setDate(QDate::currentDate().addDays(-360));
    dateEdit1->setDate(QDate::currentDate());
    form.addRow("测量日期: ", dateEdit0);
    form.addRow("通化到日期: ", dateEdit1);
    QLabel *dateHint = new QLabel("通化后数值 = 实测值 + F_IGRF(通化到日期) − F_IGRF(测量日期)；IGRF-14 适用于 1900—2030 年。");
    dateHint->setWordWrap(true);
    dateHint->setStyleSheet("color: gray;");
    form.addRow(QString(), dateHint);
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
    comboBox2->addItem("E（椭球高）");
    comboBox2->addItem("M（海拔高）");
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    spinbox1->setRange(-10.0,9999.0);
    spinbox1->setDecimals(3);
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
        useGeoid = comboBox2->currentIndex();   // E = above the ellipsoid, M = above mean sea level
        height = spinbox1->value();             // km (the IGRF routine takes km)
        data_index = comboBox->currentIndex();
        dir = linEdit->text();
        return 0;
    }
    return -1;
}
