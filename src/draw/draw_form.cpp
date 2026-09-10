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
void draw_Form::setMapStep(double dx,double dy)
{
    x_step = dx;
    y_step = dy;
}

draw_Form::~draw_Form()
{
    delete ui;
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
            if (values.size() < 3)
            {
                qWarning() << "Skipping malformed line in" << filename << ":" << line;
                continue;
            }
            bool ok;
            xx.append(values[0].toDouble(&ok));
            yy.append(values[1].toDouble(&ok));
            result.append(values[2].toDouble(&ok));
        }
    }
    file.close();

}

QCustomPlot* draw_Form::buildHeatmapPlot(int cols, int rows,
                                          double minX, double maxX, double minY, double maxY,
                                          double minZ, double maxZ,
                                          const std::function<double(int, int)> &valueAt,
                                          QCPColorScale **outColorScale)
{
    QCustomPlot *customPlot = new QCustomPlot();
    QCPColorMap *heatmap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    heatmap->data()->setSize(cols, rows);
    heatmap->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double z = valueAt(i, j);
            if (std::isnan(z))
                heatmap->data()->setAlpha(j, i, 0);
            else if (z != 0)
                heatmap->data()->setCell(j, i, z);
            else
                heatmap->data()->setAlpha(j, i, 0);
        }
    }
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    heatmap->setColorScale(colorScale);
    colorScale->setDataRange(QCPRange(minZ, maxZ));
    colorScale->setGradient(QCPColorGradient::gpJet);
    if (outColorScale)
        *outColorScale = colorScale;
    return customPlot;
}

void draw_Form::set_heatMapView(QVector<double> xx,QVector<double> yy,QVector<double> result)
{
    // 清除之前的图形
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
    // 检查输入数据有效性
    if (xx.isEmpty() || yy.isEmpty() || result.isEmpty() ||
        xx.size() != yy.size() || xx.size() != result.size()) {
        QMessageBox::warning(this, tr("TableView"), tr("图形绘制错误:输入数据为空或维度不匹配"));
        magWarn = false;
        return;
    }
    // 确定数据范围
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    double minZ = *std::min_element(result.begin(), result.end());
    double maxZ = *std::max_element(result.begin(), result.end());

    // 确定网格尺寸
    // 使用类内定义的格网分辨率计算nx和ny
    if (x_step <= 0.0 || y_step <= 0.0) {
        qDebug() << "Error: grid step (x_step/y_step) not set before set_heatMapView";
        return;
    }
    int nx = qRound((maxX - minX) / x_step) + 1;
    int ny = qRound((maxY - minY) / y_step) + 1;

    // 验证计算结果
    if (nx <= 0 || ny <= 0) {
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
    // 创建QCustomPlot对象
    QCustomPlot *customPlot = buildHeatmapPlot(cols, rows, minX, maxX, minY, maxY, minZ, maxZ,
        [&result, cols](int i, int j) { return result[i * cols + j]; });
    // 设置轴标签
    customPlot->xAxis->setLabel("经度");
    customPlot->yAxis->setLabel("纬度");
    // 绘图窗口设置
    customPlot->rescaleAxes();
    customPlot->replot();
    QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_heatmap);
    layout1->addWidget(customPlot);
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
        plotter->setStep(x_step,y_step);
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
    double rangeX = (maxX > minX) ? (maxX - minX) : 1.0;
    double rangeY = (maxY > minY) ? (maxY - minY) : 1.0;
    for (int i = 0; i < xx.size(); i++) {
        // 将坐标映射到网格索引
        int xIndex = qBound(0, static_cast<int>((xx[i] - minX) / rangeX * (cols - 1)), cols - 1);
        int yIndex = qBound(0, static_cast<int>((yy[i] - minY) / rangeY * (rows - 1)), rows - 1);

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
    QCPColorScale *colorScale = nullptr;
    QCustomPlot *customPlot = buildHeatmapPlot(cols, rows, minX, maxX, minY, maxY, minZ, maxZ,
        [&gridValues](int row, int col) { return gridValues[row][col]; }, &colorScale);
    colorScale->axis()->setLabel("数值范围");

    // 设置轴标签
    customPlot->xAxis->setLabel("经度 °");
    customPlot->yAxis->setLabel("纬度 °");

    // 添加标题
    customPlot->plotLayout()->insertRow(0);
    QCPTextElement *title = new QCPTextElement(customPlot);
    // title->setText(QString("热力图 (原始数据: %1 点，降采样: %2 x %3)")
    //                    .arg(result.size()).arg(rows).arg(cols));
    title->setText("热力图");
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

void draw_Form::autoset_contourView(QVector<double> xx,QVector<double> yy,QVector<double> result)
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
    ContourPlotter *contour = new ContourPlotter;
    contour->setData(std::vector<double>(xx.begin(), xx.end()),
                     std::vector<double>(yy.begin(), yy.end()),
                     std::vector<double>(result.begin(), result.end()));
    contour->autoDetectStep();
    if (std::abs(x_step) < 1e-10 || std::abs(y_step) < 1e-10) {
        double xmin = *std::min_element(xx.begin(), xx.end());
        double xmax = *std::max_element(xx.begin(), xx.end());
        double ymin = *std::min_element(yy.begin(), yy.end());
        double ymax = *std::max_element(yy.begin(), yy.end());
        x_step = (xmax - xmin) / 100.0;
        y_step = (ymax - ymin) / 100.0;
    }
    // 绘制等值线图
    contour->autoplotContour();
    QVBoxLayout *layout1 = new QVBoxLayout(ui->tab_counter);
    layout1->addWidget(contour);


}
