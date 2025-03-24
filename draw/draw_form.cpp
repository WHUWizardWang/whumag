#include "draw_form.h"
#include "ui_draw_form.h"

draw_Form::draw_Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::draw_Form)
{
    ui->setupUi(this);
    ui->tabWidget->setTabText(0,"热力图");
    ui->tabWidget->setTabText(1,"等值线图");
}

draw_Form::~draw_Form()
{
    delete ui;
}

// 函数用于查找二维vector中的最大值和最小值
std::pair<double, double> draw_Form::findMinMax(const QVector<QVector<double>>& vec)
{
    if (vec.empty() || vec[0].empty()) {
        throw std::invalid_argument("The vector is empty.");
    }
    double minVal = vec[0][0];
    double maxVal = vec[0][0];
    for (const auto& row : vec)
    {
        for (int val : row)
        {
            if (val < minVal)
            {
                minVal = val;
            }
            if (val > maxVal)
            {
                maxVal = val;
            }
        }
    }
    return {minVal, maxVal};
}

double draw_Form::findDifferenceInArithmeticSequence(const QVector<double>& sequence)
{
    if (sequence.size() < 2) {
        return -1;
    }
    double tolerance = 0.0001;
    // 使用unordered_map来统计每个差值（考虑容差）的出现次数
        std::map<double, int> differenceCounts;

        // 遍历数组，计算相邻元素的差值
        for (size_t i = 1; i < sequence.size(); ++i) {
            double difference = std::fabs(sequence[i] - sequence[i - 1]);

            // 查找最接近的、在容差范围内的已有差值，或添加新的差值
            bool found = false;
            for (auto& pair : differenceCounts) {
                if (std::fabs(pair.first - difference) < tolerance) {
                    pair.second++; // 更新计数
                    found = true;
                    break;
                }
            }

            if (!found) {
                differenceCounts[difference] = 1; // 添加新的差值
            }
        }

        // 找到出现次数最多的差值
        double mostCommonDifference = 0.0;
        int maxCount = 0;
        for (const auto& pair : differenceCounts) {
            if (pair.second > maxCount) {
                maxCount = pair.second;
                mostCommonDifference = pair.first;
            }
        }

        return mostCommonDifference;

}

void draw_Form::create_xyz_p(QVector<AnoPoint> ano_pnts,QVector<double> &xx,QVector<double> &yy,QVector<double> &result)
{
    xx.clear();
    yy.clear();
    result.clear();
    int num = ano_pnts.size();
    for (int i=0;i<num;i++)
    {
        xx.push_back(ano_pnts[i].y);
        yy.push_back(ano_pnts[i].x);
        result.push_back(ano_pnts[i].z);
    }
}

void draw_Form::create_xyz_f(QString filename,QVector<double> &xx,QVector<double> &yy,QVector<double> &result)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Cannot open file for reading:" << filename;
        return;
    }
    QTextStream in(&file);
    QString line;
    while (!in.atEnd())
    {
        line = in.readLine().trimmed(); // 读取一行并去除两端空格
        if (!line.isEmpty())
        {
            QStringList values = line.split(",", QString::SkipEmptyParts); // 使用逗号分割
            if (values.size() < 3)
            {
                values = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            }
            bool ok;
            xx.append(values[0].toDouble(&ok));
            yy.append(values[1].toDouble(&ok));
            result.append(values[2].toDouble(&ok));
        }
    }
    file.close();
}

void draw_Form::set_HeatOrSactterView(QVector<double> xx,QVector<double> yy,QVector<double> result)
{
    QDialog dialog;
    QFormLayout form(&dialog);
    dialog.setWindowTitle("输入参数: ");
    //格网分辨率
    QLabel *label1 = new QLabel("经度/X") ;
    QDoubleSpinBox *spinbox1 = new QDoubleSpinBox(&dialog);
    QLabel *label2 = new QLabel("纬度/Y") ;
    QDoubleSpinBox *spinbox2 = new QDoubleSpinBox(&dialog);
    spinbox1->setDecimals(2);
    spinbox2->setDecimals(2);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(label1);
    horizontalLayout->addWidget(spinbox1);
    horizontalLayout->addWidget(label2);
    horizontalLayout->addWidget(spinbox2);
    form.addRow("格网分辨率:    ", horizontalLayout);
//    //是否进行插值
//    QRadioButton *radioButton1 = new QRadioButton("测线");
//    QRadioButton *radioButton2 = new QRadioButton("格网");
//    QHBoxLayout *horizontalLayout1 = new QHBoxLayout();
//    horizontalLayout1->addWidget(radioButton1);
//    horizontalLayout1->addWidget(radioButton2);
//    form.addRow("是否插值:    ", horizontalLayout1);
    // Add Cancel and OK button
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    // Process when OK button is clicked
    if (dialog.exec() == QDialog::Accepted)
    {
        x_step = spinbox1->value();
        y_step = spinbox2->value();

//        if (radioButton1->isChecked())
//            type = 0;
//        else if(radioButton2->isChecked())
//        {
//            if (x_step <= 0 || y_step <= 0) //输入格网分辨率为0或负
//                return;
//            type = 1;
//        }
//        else type = 1;
    }
    else return;
    //
    QLayout *layout = ui->tab_heatmap->layout();
    if (layout)
    {
        delete layout;
        ui->tab_heatmap->setLayout(nullptr);
    }
    else
    {
        QList<QWidget*> children = ui->tab_heatmap->findChildren<QWidget*>();
        qDeleteAll(children); // 使用qDeleteAll自动删除QList中的每个QWidget指针
    }
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    double minZ = *std::min_element(result.begin(), result.end());
    double maxZ = *std::max_element(result.begin(), result.end());
    if (type == 1)
    {
        int cols = round((maxX-minX)/x_step +1);
        int rows = round((maxY-minY)/y_step +1);
        if (rows*cols != result.size())
        {
            QMessageBox::warning(this,tr("TableView"),tr("图形绘制错误:真实格网分辨率与输入不符"));
            return;
        }
        QCustomPlot *customPlot = new QCustomPlot();
        QCPColorMap *heatmap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
        heatmap->data()->setSize(cols, rows);
        heatmap->data()->setRange(QCPRange(y_step, rows - y_step), QCPRange(x_step, cols- x_step));
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                double z = result[i*cols + j];
                if (isnanf(z))
                    heatmap->data()->setAlpha(j,i, 0);
                else if(z) heatmap->data()->setCell(j,i,z);
                else heatmap->data()->setAlpha(j,i, 0);
            }
        }

        // 添加颜色条
        QCPColorScale *colorScale = new QCPColorScale(customPlot);
        customPlot->plotLayout()->addElement(0, 1, colorScale);
        heatmap->setColorScale(colorScale);
        colorScale->setDataRange(QCPRange(minZ,maxZ));
        colorScale->setGradient(QCPColorGradient::gpJet);
        // 设置坐标轴范围
        QVector<double> x_ticks,y_ticks;
        QVector<QString> x_labels,y_labels;
        int numIntervals =5;
        double x_interval = (maxX - minX) / (numIntervals - 1);
        double nx_interval = cols/(numIntervals - 1);
        double y_interval = (maxY - minY) / (numIntervals - 1);
        double ny_interval = rows/(numIntervals - 1);
        for (int i = 0; i < numIntervals; ++i)
        {
            if (i == numIntervals - 1)
            {
                x_ticks.push_back(rows);
                y_ticks.push_back(cols);
                x_labels.push_back(QString::number(maxX));
                y_labels.push_back(QString::number(maxY));
            }
            else
            {
                x_ticks.push_back(i * ny_interval);
                y_ticks.push_back(i * nx_interval);
                x_labels.push_back(QString::number(minX + i * x_interval));
                y_labels.push_back(QString::number(minY + i * y_interval));
            }
        }
        QSharedPointer<QCPAxisTickerText> x_textTicker(new QCPAxisTickerText);
        x_textTicker->addTicks(x_ticks, x_labels);
        customPlot->xAxis->setTicker(x_textTicker);
        QSharedPointer<QCPAxisTickerText> y_textTicker(new QCPAxisTickerText);
        y_textTicker->addTicks(y_ticks, y_labels);
        customPlot->yAxis->setTicker(y_textTicker);
        // 绘图窗口设置
        customPlot->rescaleAxes();
        customPlot->replot();
        QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_heatmap);
        layout1->addWidget(customPlot);
    }
    else if(type == 0)
    {
         QCustomPlot *customPlot = new QCustomPlot();
         customPlot->addGraph();
         customPlot->graph(0)->setData(xx,yy);
         customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
         customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle,5));
         customPlot->replot();
         QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_heatmap);
         layout1->addWidget(customPlot);
    }

}

void draw_Form::set_heatMapView(QVector<double> xx,QVector<double> yy,QVector<double> result)
{
    //
    QLayout *layout = ui->tab_heatmap->layout();
    if (layout)
    {
        delete layout;
        ui->tab_heatmap->setLayout(nullptr);
    }
    else
    {
        QList<QWidget*> children = ui->tab_heatmap->findChildren<QWidget*>();
        qDeleteAll(children); // 使用qDeleteAll自动删除QList中的每个QWidget指针
    }
    // 确定网格尺寸
    int nx = 0;
    int ny = 0;
    // 找到第一个Y值改变的位置，这就是nx
    for (size_t i = 1; i < yy.size(); ++i) {
        if (std::abs(yy[i] - yy[0]) > 1e-3) {  // 使用小数值比较
            nx = i;
            break;
        }
    }
    // 计算ny
    if (nx > 0) {
        ny = result.size() / nx;
    } else {
        qDebug() << "Error: Could not determine grid dimensions";
        return;
    }
    int cols = nx;
    int rows = ny;
    if (rows*cols != result.size())
    {
        QMessageBox::warning(this,tr("TableView"),tr("图形绘制错误:真实格网分辨率不符"));
        magWarn = false;
        return;
    }
    //
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    double minZ = *std::min_element(result.begin(), result.end());
    double maxZ = *std::max_element(result.begin(), result.end());
    QCustomPlot *customPlot = new QCustomPlot();
    QCPColorMap *heatmap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    heatmap->data()->setSize(cols, rows);
    heatmap->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY));
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            double z = result[i*cols + j];
            if (isnanf(z))
                heatmap->data()->setAlpha(j,i, 0);
            else if(z) heatmap->data()->setCell(j,i,z);
            else heatmap->data()->setAlpha(j,i, 0);
        }
    }

    // 添加颜色条
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    heatmap->setColorScale(colorScale);
    colorScale->setDataRange(QCPRange(minZ,maxZ));
    colorScale->setGradient(QCPColorGradient::gpJet);
    // 设置轴标签
    customPlot->xAxis->setLabel("X /km");
    customPlot->yAxis->setLabel("Y /km");
    // 绘图窗口设置
    customPlot->rescaleAxes();
    customPlot->replot();
    QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_heatmap);
    layout1->addWidget(customPlot);
}

void draw_Form::create_contour_txt(QVector<AnoPoint> &ano_pnts)
{
    QString appDirPath = QCoreApplication::applicationDirPath();
    QString filePath = appDirPath+"/temp_qt2python.txt";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Cannot open file for writing:" << filePath;
        return;
    }
    QTextStream out(&file);
    for (AnoPoint value : ano_pnts)
    {
        out << value.y << " "<<value.x <<" "<<value.z<<endl;
    }
}

void draw_Form::set_ContourView(QString filename)
{
    QLayout *oldlayout = ui->tab_counter->layout();
        if (oldlayout)
        {
            delete oldlayout;
            ui->tab_counter->setLayout(nullptr);
        }
        else
        {
            QList<QWidget*> children = ui->tab_counter->findChildren<QWidget*>();
            qDeleteAll(children); // 使用qDeleteAll自动删除QList中的每个QWidget指针
        }
        ContourPlotter *plotter = new ContourPlotter();
        if (!plotter->loadData(filename)) {
            qDebug() << "Failed to load data file";
        }
        // 绘制等值线图
        plotter->plotContour();
        QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_counter);
        layout1->addWidget(plotter);
}


void draw_Form::on_pushButton_path_clicked()
{
    // 选择文件
    QString path = QFileDialog::getSaveFileName(this,"选择保存路径","","Files(*.png)");
    if(!path.isEmpty())
    {
        ui->lineEdit->setText(path);
    }
}

void draw_Form::on_pushButton_save_clicked()
{
    QString path = ui->lineEdit->text();
    int currentIndex = ui->tabWidget->currentIndex();
    if (currentIndex == 0)
    {
        QPixmap pixmap = QPixmap::grabWidget(ui->tab_heatmap);
        pixmap.save(path,"PNG");
    }
    else if (currentIndex == 1)
    {
        QPixmap pixmap = QPixmap::grabWidget(ui->tab_counter);
        pixmap.save(path,"PNG");
    }
    QMessageBox::information(nullptr,"保存完成","文件已成功保存!");
}
void draw_Form::autoset_heatMapView(QVector<double> xx, QVector<double> yy, QVector<double> result)
{
    // 清理现有布局
    QLayout *layout = ui->tab_heatmap->layout();
    if (layout)
    {
        delete layout;
        ui->tab_heatmap->setLayout(nullptr);
    }
    else
    {
        QList<QWidget*> children = ui->tab_heatmap->findChildren<QWidget*>();
        qDeleteAll(children);
    }

    // 检查输入数据有效性
    if (xx.isEmpty() || yy.isEmpty() || result.isEmpty() || xx.size() != yy.size() || xx.size() != result.size()) {
        QMessageBox::warning(this, tr("数据错误"), tr("输入数据为空或维度不匹配"));
        magWarn = false;
        return;
    }

    const int MAX_GRID_SIZE = 500; // 最大网格尺寸

    qDebug() << "原始数据点数: " << result.size();

    // 1. 确定数据范围
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    double minZ = *std::min_element(result.begin(), result.end());
    double maxZ = *std::max_element(result.begin(), result.end());

    // 2. 创建降采样网格
    int cols = qMin(MAX_GRID_SIZE, static_cast<int>(sqrt(result.size() / 4)));
    int rows = qMin(MAX_GRID_SIZE, static_cast<int>(sqrt(result.size() / 4)));

    qDebug() << "降采样后网格尺寸: " << rows << "x" << cols;

    // 3. 创建降采样后的数据
    QVector<QVector<double>> gridValues(rows, QVector<double>(cols, 0.0));
    QVector<QVector<int>> gridCounts(rows, QVector<int>(cols, 0));

    // 4. 将原始数据映射到降采样网格
    for (int i = 0; i < xx.size(); i++) {
        // 将坐标映射到网格索引
        int xIndex = qBound(0, static_cast<int>((xx[i] - minX) / (maxX - minX) * (cols - 1)), cols - 1);
        int yIndex = qBound(0, static_cast<int>((yy[i] - minY) / (maxY - minY) * (rows - 1)), rows - 1);

        // 累加值，稍后计算平均值
        if (!std::isnan(result[i])) {
            gridValues[yIndex][xIndex] += result[i];
            gridCounts[yIndex][xIndex]++;
        }
    }

    // 5. 计算每个网格单元的平均值
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (gridCounts[y][x] > 0) {
                gridValues[y][x] /= gridCounts[y][x];
            } else {
                gridValues[y][x] = std::numeric_limits<double>::quiet_NaN();
            }
        }
    }

    // ===== 创建热图 =====
    QCustomPlot *customPlot = new QCustomPlot();
    QCPColorMap *heatmap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    heatmap->data()->setSize(cols, rows);
    heatmap->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY));

    // 填充热图数据
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            double z = gridValues[y][x];
            if (std::isnan(z)) {
                heatmap->data()->setAlpha(x, y, 0);
            } else if (z != 0) {
                heatmap->data()->setCell(x, y, z);
            } else {
                heatmap->data()->setAlpha(x, y, 0);
            }
        }
    }

    // 添加颜色条
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    heatmap->setColorScale(colorScale);
    colorScale->setDataRange(QCPRange(minZ, maxZ));
    colorScale->setGradient(QCPColorGradient::gpJet);
    colorScale->axis()->setLabel("数值范围");

    // 设置轴标签
    customPlot->xAxis->setLabel("X /km");
    customPlot->yAxis->setLabel("Y /km");

    // 添加标题
    customPlot->plotLayout()->insertRow(0);
    QCPTextElement *title = new QCPTextElement(customPlot);
    title->setText(QString("热图 (原始数据: %1 点，降采样: %2 x %3)")
                       .arg(result.size()).arg(rows).arg(cols));
    title->setFont(QFont("sans", 10, QFont::Bold));
    customPlot->plotLayout()->addElement(0, 0, title);

    // 设置交互
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // 绘图窗口设置
    customPlot->rescaleAxes();
    customPlot->replot();

    // 添加到布局
    QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_heatmap);
    layout1->addWidget(customPlot);

    magWarn = true;
}

// 辅助函数：查找最接近的索引
int draw_Form::findClosestIndex(const QList<double>& sortedValues, double target)
{
    auto it = std::lower_bound(sortedValues.begin(), sortedValues.end(), target);

    if (it == sortedValues.begin()) {
        return 0;
    }
    else if (it == sortedValues.end()) {
        return sortedValues.size() - 1;
    }
    else {
        int index = std::distance(sortedValues.begin(), it);
        double diff1 = *it - target;
        double diff2 = target - *(it - 1);
        return (diff1 < diff2) ? index : index - 1;
    }
}
