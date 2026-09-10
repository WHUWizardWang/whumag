#include "contourplotter.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <algorithm>
#include <cmath>

ContourPlotter::ContourPlotter(QWidget *parent) :
    QMainWindow(parent),
    m_rms(0.0)
{
    // 创建QCustomPlot实例
    customPlot = new QCustomPlot(this);
    setCentralWidget(customPlot);

    // 设置窗口大小
    resize(800, 600);

    // 设置白色背景
    customPlot->setBackground(Qt::white);
}

ContourPlotter::~ContourPlotter()
{
}

bool ContourPlotter::loadData(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening file:" << filepath;
        return false;
    }

    X.clear();
    Y.clear();
    Z.clear();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        if (values.size() >= 3) {
            X.push_back(values[0].toDouble());
            Y.push_back(values[1].toDouble());
            Z.push_back(values[2].toDouble());
        }
    }

    return true;
}

double ContourPlotter::bilinearInterpolate(double x, double y,
                                           const std::vector<double>& gridX,
                                           const std::vector<double>& gridY,
                                           const std::vector<std::vector<double>>& gridZ)
{
    // 检查输入数据的有效性
    if (gridX.empty() || gridY.empty() || gridZ.empty() || gridZ[0].empty()) {
        return 0.0;
    }

    // 修正的边界处理
    if (x <= gridX.front()) {
        if (y <= gridY.front()) return gridZ[0][0];
        if (y >= gridY.back()) return gridZ[gridY.size()-1][0];
    }
    if (x >= gridX.back()) {
        if (y <= gridY.front()) return gridZ[0][gridX.size()-1];
        if (y >= gridY.back()) return gridZ[gridY.size()-1][gridX.size()-1];
    }

    // 使用二分查找确定x所在的区间
    int i = 0;
    int lower = 0;
    int upper = gridX.size() - 1;

    while (lower <= upper) {
        int mid = (lower + upper) / 2;
        if (x < gridX[mid]) {
            upper = mid - 1;
        } else if (x > gridX[mid] && mid < gridX.size() - 1 && x >= gridX[mid + 1]) {
            lower = mid + 1;
        } else {
            i = mid;
            break;
        }
    }

    // 使用二分查找确定y所在的区间
    int j = 0;
    lower = 0;
    upper = gridY.size() - 1;

    while (lower <= upper) {
        int mid = (lower + upper) / 2;
        if (y < gridY[mid]) {
            upper = mid - 1;
        } else if (y > gridY[mid] && mid < gridY.size() - 1 && y >= gridY[mid + 1]) {
            lower = mid + 1;
        } else {
            j = mid;
            break;
        }
    }

    // 确保索引不会越界
    i = std::min(i, static_cast<int>(gridX.size() - 2));
    j = std::min(j, static_cast<int>(gridY.size() - 2));
    i = std::max(i, 0);
    j = std::max(j, 0);

    // 获取周围四个点的值
    double x1 = gridX[i];
    double x2 = gridX[i + 1];
    double y1 = gridY[j];
    double y2 = gridY[j + 1];

    // 安全地访问网格数据
    if (j >= 0 && j + 1 < gridZ.size() && i >= 0 && i + 1 < gridZ[0].size()) {
        double q11 = gridZ[j][i];
        double q12 = gridZ[j + 1][i];
        double q21 = gridZ[j][i + 1];
        double q22 = gridZ[j + 1][i + 1];

        // 防止除零错误
        double dx = x2 - x1;
        double dy = y2 - y1;
        if (std::abs(dx) < 1e-10 || std::abs(dy) < 1e-10) {
            return q11; // 如果网格极小，直接返回最近点的值
        }

        // 计算插值权重
        double fx = (x - x1) / dx;
        double fy = (y - y1) / dy;

        // 执行双线性插值
        return q11 * (1 - fx) * (1 - fy) +
               q21 * fx * (1 - fy) +
               q12 * (1 - fx) * fy +
               q22 * fx * fy;
    }

    // 如果出现任何问题，返回默认值
    return 0.0;
}

void ContourPlotter::plotContour()
{
    // 检查数据有效性
    if (X.empty() || Y.empty() || Z.empty()) {
        qDebug() << "数据为空";
        return;
    }
    if (X.size() != Y.size() || X.size() != Z.size()) {
        qDebug() << "数据大小不一致";
        return;
    }
    // 检查dx和dy是否为有效值
    if (std::abs(dx) < 1e-10 || std::abs(dy) < 1e-10) {
        qDebug() << "步长值过小或为零: dx=" << dx << ", dy=" << dy;
        return;
    }

    double xmin = *std::min_element(X.begin(), X.end());
    double xmax = *std::max_element(X.begin(), X.end());
    double ymin = *std::min_element(Y.begin(), Y.end());
    double ymax = *std::max_element(Y.begin(), Y.end());

    // 使用类内定义的格网分辨率计算nx和ny
    int nx = qRound((xmax - xmin) / dx) + 1;
    int ny = qRound((ymax - ymin) / dy) + 1;

    // 验证数据尺寸
    if (Z.size() != nx * ny) {
        qDebug() << "数据尺寸不匹配: Z.size()=" << Z.size() << ", nx*ny=" << nx*ny;
        return;
    }

    // 验证计算结果
    if (nx <= 0 || ny <= 0) {
        qDebug() << "Error: Could not determine grid dimensions";
        return;
    }

    // 创建2D网格数据
    std::vector<std::vector<double>> Z_grid(ny, std::vector<double>(nx));
    for (int i = 0; i < ny; ++i) {
        for (int j = 0; j < nx; ++j) {
            int index = i * nx + j;
            if (index < Z.size()) {
                Z_grid[i][j] = Z[index];
            } else {
                Z_grid[i][j] = 0.0; // 防止索引越界
            }
        }
    }

    std::vector<double> xi(nx);
    std::vector<double> yi(ny);

    for (int i = 0; i < nx; ++i) {
        xi[i] = xmin + (xmax - xmin) * i / (nx - 1);
    }
    for (int i = 0; i < ny; ++i) {
        yi[i] = ymin + (ymax - ymin) * i / (ny - 1);
    }
    std::vector<std::vector<double>> zi(ny, std::vector<double>(nx));
    for (int i = 0; i < ny; ++i) {
        for (int j = 0; j < nx; ++j) {
            // 使用原始数据点进行插值
            // double x = xmin + (xmax - xmin) * j / (nx - 1);
            // double y = ymin + (ymax - ymin) * i / (ny - 1);
            zi[i][j] = Z_grid[i][j];  // 直接使用网格数据，不进行插值
        }
    }

    // 清除之前的图形
    customPlot->clearPlottables();

    // 创建颜色图
    QCPColorMap *colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    colorMap->data()->setSize(nx, ny);
    colorMap->data()->setRange(QCPRange(xmin, xmax), QCPRange(ymin, ymax));

    // 设置数据
    for (int i = 0; i < ny; ++i) {
        for (int j = 0; j < nx; ++j) {
            colorMap->data()->setCell(j, i, zi[i][j]);
        }
    }

    // 设置颜色比例尺
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorMap->setColorScale(colorScale);
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    // 生成等值线级别
    int numContours = 10; // 设置等值线数量
    std::vector<double> levels = generateContourLevels(zi, numContours);

    // 创建颜色映射
    QVector<QColor> contourColors;
    for (int i = 0; i < numContours; ++i) {
        // 使用HSV色彩空间创建渐变色
        QColor color = QColor::fromHsv(i * 359 / numContours, 255, 255);
        contourColors.append(color);
    }

    // 创建图例
    customPlot->legend->setVisible(true);
    customPlot->legend->setFont(QFont("Arial", 9));
    customPlot->legend->setRowSpacing(-3);


    // 为每个等值线级别计算并绘制等值线
    for (int levelIndex = 0; levelIndex < levels.size(); ++levelIndex) {
        double level = levels[levelIndex];
        std::vector<QVector<double>> contourX, contourY;
        findContours(zi, level, contourX, contourY);

        // 找出最长的等值线
        size_t longestContourIndex = 0;
        size_t maxLength = 0;
        for (size_t i = 0; i < contourX.size(); ++i) {
            if (contourX[i].size() > maxLength) {
                maxLength = contourX[i].size();
                longestContourIndex = i;
            }
        }

        // 为当前等值线设置颜色
        QPen contourPen;
        contourPen.setColor(contourColors[levelIndex]);
        contourPen.setWidth(2);

        // 绘制每条等值线
        for (size_t i = 0; i < contourX.size(); ++i) {
            QCPCurve *contourLine = new QCPCurve(customPlot->xAxis, customPlot->yAxis);

            // 设置图例项
            if (i == 0) {
                contourLine->setName(QString::number(level, 'f', 2));
            } else {
                contourLine->removeFromLegend();
            }

            // 将网格索引转换为实际坐标
            QVector<double> realX, realY;
            for (int j = 0; j < contourX[i].size(); ++j) {
                double x = xmin + (xmax - xmin) * contourX[i][j] / (nx - 1);
                double y = ymin + (ymax - ymin) * contourY[i][j] / (ny - 1);
                realX.append(x);
                realY.append(y);
            }

            contourLine->setData(realX, realY);
            contourLine->setPen(contourPen);

            // 只在最长的等值线上添加标签
            if (i == longestContourIndex && realX.size() > 0) {
                int midIndex = realX.size() / 2;

                QCPItemText *textLabel = new QCPItemText(customPlot);
                textLabel->position->setType(QCPItemPosition::ptPlotCoords);

                // 添加标签偏移
                double offsetX = (xmax - xmin) * 0.01;
                double offsetY = (ymax - ymin) * 0.01;
                textLabel->position->setCoords(realX[midIndex] + offsetX, realY[midIndex] + offsetY);

                textLabel->setText(QString::number(level, 'f', 2));
                textLabel->setFont(QFont("Arial", 8));
                textLabel->setPen(QPen(contourColors[levelIndex]));
                textLabel->setBrush(QBrush(Qt::white));
                textLabel->setPadding(QMargins(2, 2, 2, 2));
            }
        }
    }

    // 调整图例位置和样式
    customPlot->legend->setFillOrder(QCPLayoutGrid::foRowsFirst);
    customPlot->legend->setWrap(4); // 每行显示4个项目
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);

    // 设置轴标签
    customPlot->xAxis->setLabel("lon");
    customPlot->yAxis->setLabel("lat");

    // // 如果有RMS值，添加标题
    // if (m_rms != 0.0) {
    //     customPlot->plotLayout()->insertRow(0);
    //     customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot,
    //                                                                   QString("RMSE: %1").arg(m_rms, 0, 'f', 4)));
    // }

    // 调整轴范围并重绘
    customPlot->rescaleAxes();
    customPlot->replot();
}

void ContourPlotter::autoplotContour()
{
    // 检查数据有效性
    if (X.empty() || Y.empty() || Z.empty()) {
        qDebug() << "数据为空";
        return;
    }
    if (X.size() != Y.size() || X.size() != Z.size()) {
        qDebug() << "数据大小不一致: X=" << X.size() << " Y=" << Y.size() << " Z=" << Z.size();
        return;
    }

    qDebug() << "原始数据点数: " << Z.size();

    // 1. 确定数据范围
    double xmin = *std::min_element(X.begin(), X.end());
    double xmax = *std::max_element(X.begin(), X.end());
    double ymin = *std::min_element(Y.begin(), Y.end());
    double ymax = *std::max_element(Y.begin(), Y.end());
    double zmin = *std::min_element(Z.begin(), Z.end());
    double zmax = *std::max_element(Z.begin(), Z.end());

    qDebug() << "X范围: " << xmin << " 到 " << xmax;
    qDebug() << "Y范围: " << ymin << " 到 " << ymax;
    qDebug() << "Z范围: " << zmin << " 到 " << zmax;

    // 检查步长是否设置合理
    if (std::abs(dx) < 1e-10 || std::abs(dy) < 1e-10) {
        // 自动设置合理的步长
        dx = (xmax - xmin) / 100.0;
        dy = (ymax - ymin) / 100.0;
        qDebug() << "步长自动设置为: dx=" << dx << ", dy=" << dy;
    }

    // 检查数据分布，确定是否为规则网格
    bool isRegularGrid = checkRegularGrid(X, Y, dx, dy);
    qDebug() << "数据检测为:" << (isRegularGrid ? "规则网格" : "不规则点");

    // 计算网格大小 - 基于实际数据分布
    int nx, ny;
    // if (isRegularGrid) {
    //     // 如果是规则网格，使用数据中暗示的网格尺寸
    //     nx = static_cast<int>((xmax - xmin) / dx) + 1;
    //     ny = static_cast<int>((ymax - ymin) / dy) + 1;
    // } else {
    //     // 不规则数据，使用合适的网格尺寸
    //     const int MAX_GRID_SIZE = 500;
    //     nx = qMin(MAX_GRID_SIZE, static_cast<int>((xmax - xmin) / dx) + 1);
    //     ny = qMin(MAX_GRID_SIZE, static_cast<int>((ymax - ymin) / dy) + 1);
    // }
    const int MAX_GRID_SIZE = 500;
    nx = qMin(MAX_GRID_SIZE, static_cast<int>((xmax - xmin) / dx) + 1);
    ny = qMin(MAX_GRID_SIZE, static_cast<int>((ymax - ymin) / dy) + 1);

    qDebug() << "网格尺寸: " << nx << "x" << ny;

    // 2. 创建网格
    std::vector<std::vector<double>> gridValues(ny, std::vector<double>(nx, 0.0));
    std::vector<std::vector<int>> gridCounts(ny, std::vector<int>(nx, 0));

    // 3. 将原始数据映射到网格 - 改进版
    for (size_t i = 0; i < X.size(); i++) {
        if (std::isnan(Z[i])) continue;

        // 计算精确的网格索引
        double xRel = (X[i] - xmin) / (xmax - xmin);
        double yRel = (Y[i] - ymin) / (ymax - ymin);
        int xIndex = qBound(0, static_cast<int>(xRel * (nx - 1) + 0.5), nx - 1);
        int yIndex = qBound(0, static_cast<int>(yRel * (ny - 1) + 0.5), ny - 1);

        gridValues[yIndex][xIndex] += Z[i];
        gridCounts[yIndex][xIndex]++;
    }

    // 4. 计算每个网格单元的平均值
    int emptyCount = 0;
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            if (gridCounts[y][x] > 0) {
                gridValues[y][x] /= gridCounts[y][x];
            } else {
                gridValues[y][x] = std::numeric_limits<double>::quiet_NaN();
                emptyCount++;
            }
        }
    }
    qDebug() << "空网格单元数: " << emptyCount << " ("
             << (100.0 * emptyCount / (nx * ny)) << "%)";

    // 5. 填充空网格单元 - 使用插值
    if (emptyCount > 0 && emptyCount < nx * ny * 0.5) {
        fillEmptyGridCells(gridValues);
        qDebug() << "已填充空网格单元";
    }

    // 创建坐标向量
    std::vector<double> xi(nx);
    std::vector<double> yi(ny);

    for (int i = 0; i < nx; i++) {
        xi[i] = xmin + (xmax - xmin) * i / (nx - 1);
    }
    for (int i = 0; i < ny; i++) {
        yi[i] = ymin + (ymax - ymin) * i / (ny - 1);
    }

    // 清除之前的图形
    customPlot->clearPlottables();

    // 创建颜色图
    QCPColorMap *colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    colorMap->data()->setSize(nx, ny);
    colorMap->data()->setRange(QCPRange(xmin, xmax), QCPRange(ymin, ymax));
    colorMap->setInterpolate(true);
    colorMap->setTightBoundary(false);
    // 设置颜色图数据
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            double z = gridValues[y][x];
            if (std::isnan(z)) {
                colorMap->data()->setAlpha(x, y, 0);
            } else {
                colorMap->data()->setCell(x, y, z);
            }
        }
    }

    // 设置颜色比例尺
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorMap->setColorScale(colorScale);
    colorScale->setDataRange(QCPRange(zmin, zmax));
    colorScale->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();
    colorScale->axis()->setLabel("数值范围");

    // 自动确定适合的等值线级别数量
    int numContours = qMin(20, qMax(5, static_cast<int>(sqrt(Z.size() / 100))));
    qDebug() << "设置等值线数量: " << numContours;

    // 生成等值线级别
    std::vector<double> levels = generateContourLevels(gridValues, numContours);

    // 创建颜色映射
    QVector<QColor> contourColors;
    for (int i = 0; i < numContours; i++) {
        // 使用HSV色彩空间创建渐变色
        QColor color = QColor::fromHsv(i * 359 / numContours, 255, 255);
        contourColors.append(color);
    }

    // 创建图例
    customPlot->legend->setVisible(true);
    customPlot->legend->setFont(QFont("Arial", 9));
    customPlot->legend->setRowSpacing(-3);

    // 为每个等值线级别计算并绘制等值线
    for (int levelIndex = 0; levelIndex < levels.size(); ++levelIndex) {
        double level = levels[levelIndex];
        std::vector<QVector<double>> contourX, contourY;
        findContours(gridValues, level, contourX, contourY);

        // 找出最长的等值线
        size_t longestContourIndex = 0;
        size_t maxLength = 0;
        for (size_t i = 0; i < contourX.size(); ++i) {
            if (contourX[i].size() > maxLength) {
                maxLength = contourX[i].size();
                longestContourIndex = i;
            }
        }

        // 为当前等值线设置颜色
        QPen contourPen;
        contourPen.setColor(contourColors[levelIndex]);
        contourPen.setWidth(2);

        // 绘制每条等值线
        for (size_t i = 0; i < contourX.size(); ++i) {
            QCPCurve *contourLine = new QCPCurve(customPlot->xAxis, customPlot->yAxis);

            // 设置图例项
            if (i == 0) {
                contourLine->setName(QString::number(level, 'f', 2));
            } else {
                contourLine->removeFromLegend();
            }

            // 将网格索引转换为实际坐标
            QVector<double> realX, realY;
            for (int j = 0; j < contourX[i].size(); j++) {
                double x = xmin + (xmax - xmin) * contourX[i][j] / (nx - 1);
                double y = ymin + (ymax - ymin) * contourY[i][j] / (ny - 1);
                realX.append(x);
                realY.append(y);
            }

            contourLine->setData(realX, realY);
            contourLine->setPen(contourPen);

            // 只在最长的等值线上添加标签
            if (i == longestContourIndex && realX.size() > 0) {
                int midIndex = realX.size() / 2;

                QCPItemText *textLabel = new QCPItemText(customPlot);
                textLabel->position->setType(QCPItemPosition::ptPlotCoords);

                // 添加标签偏移
                double offsetX = (xmax - xmin) * 0.01;
                double offsetY = (ymax - ymin) * 0.01;
                textLabel->position->setCoords(realX[midIndex] + offsetX, realY[midIndex] + offsetY);

                textLabel->setText(QString::number(level, 'f', 2));
                textLabel->setFont(QFont("Arial", 8));
                textLabel->setPen(QPen(contourColors[levelIndex]));
                textLabel->setBrush(QBrush(Qt::white));
                textLabel->setPadding(QMargins(2, 2, 2, 2));
            }
        }
    }

    // 调整图例位置和样式
    customPlot->legend->setFillOrder(QCPLayoutGrid::foRowsFirst);
    customPlot->legend->setWrap(4); // 每行显示4个项目
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);

    // 设置轴标签
    customPlot->xAxis->setLabel("X");
    customPlot->yAxis->setLabel("Y");

    // 添加标题
    customPlot->plotLayout()->insertRow(0);
    QCPTextElement *title = new QCPTextElement(customPlot);
    title->setText(QString("等值线图 (原始数据: %1 点，网格: %2 x %3)")
                       .arg(Z.size()).arg(ny).arg(nx));
    title->setText("等值线图");
    title->setFont(QFont("sans", 10, QFont::Bold));
    customPlot->plotLayout()->addElement(0, 0, title);

    // 设置交互
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // 调整轴范围并重绘
    customPlot->rescaleAxes();
    customPlot->replot();

    qDebug() << "等值线图绘制完成";
}

// 检查数据是否为规则网格
bool ContourPlotter::checkRegularGrid(const std::vector<double>& x,
                                      const std::vector<double>& y,
                                      double expectedDx, double expectedDy)
{
    // 如果数据点数量很少，不做详细检查
    if (x.size() < 20) return false;

    // 获取唯一X和Y坐标
    std::set<double> uniqueX, uniqueY;
    for (size_t i = 0; i < x.size(); i++) {
        uniqueX.insert(std::round(x[i] / (expectedDx/10)) * (expectedDx/10));
        uniqueY.insert(std::round(y[i] / (expectedDy/10)) * (expectedDy/10));
    }

    // 估计行列数
    double estimatedRows = uniqueY.size();
    double estimatedCols = uniqueX.size();

    // 如果唯一坐标数与总点数的比例接近预期，可能是规则网格
    bool likelyGrid = (estimatedRows * estimatedCols) >= 0.9 * x.size() &&
                      (estimatedRows * estimatedCols) <= 1.1 * x.size();

    return likelyGrid;
}

// 填充空网格单元
void ContourPlotter::fillEmptyGridCells(std::vector<std::vector<double>>& grid)
{
    int ny = grid.size();
    if (ny == 0) return;
    int nx = grid[0].size();

    // 临时存储插值结果
    std::vector<std::vector<double>> newGrid = grid;

    // 对每个空单元尝试插值
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            if (std::isnan(grid[y][x])) {
                // 收集周围非NaN值
                std::vector<double> neighbors;
                int searchRadius = 1;

                // 扩大搜索半径直到找到足够的邻居
                while (neighbors.size() < 4 && searchRadius <= 5) {
                    for (int dy = -searchRadius; dy <= searchRadius; dy++) {
                        for (int dx = -searchRadius; dx <= searchRadius; dx++) {
                            int ny2 = y + dy;
                            int nx2 = x + dx;
                            if (ny2 >= 0 && ny2 < ny && nx2 >= 0 && nx2 < nx) {
                                if (!std::isnan(grid[ny2][nx2])) {
                                    neighbors.push_back(grid[ny2][nx2]);
                                }
                            }
                        }
                    }
                    searchRadius++;
                }

                // 如果找到至少一个邻居，用平均值填充
                if (!neighbors.empty()) {
                    double sum = std::accumulate(neighbors.begin(), neighbors.end(), 0.0);
                    newGrid[y][x] = sum / neighbors.size();
                }
            }
        }
    }

    // 更新原始网格
    grid = newGrid;
}

    // 生成等值线级别
std::vector<double> ContourPlotter::generateContourLevels(
    const std::vector<std::vector<double>>& data, int numLevels)
{
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    // 找出数据的最大值和最小值
    for (const auto& row : data) {
        for (double val : row) {
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
    }

    // 生成等间距的等值线级别
    std::vector<double> levels;
    double step = (maxVal - minVal) / (numLevels + 1);
    for (int i = 1; i <= numLevels; ++i) {
        levels.push_back(minVal + i * step);
    }

    return levels;
}

// 寻找等值线
void ContourPlotter::findContours(
    const std::vector<std::vector<double>>& data, double level,
    std::vector<QVector<double>>& contourX,
    std::vector<QVector<double>>& contourY)
{
    int rows = data.size();
    int cols = data[0].size();

    // 遍历网格单元
    for (int i = 0; i < rows - 1; ++i) {
        for (int j = 0; j < cols - 1; ++j) {
            double z1 = data[i][j];
            double z2 = data[i][j + 1];
            double z3 = data[i + 1][j + 1];
            double z4 = data[i + 1][j];

            // 检查是否有等值线穿过当前单元
            if ((z1 <= level && level < z2) || (z2 <= level && level < z1) ||
                (z2 <= level && level < z3) || (z3 <= level && level < z2) ||
                (z3 <= level && level < z4) || (z4 <= level && level < z3) ||
                (z4 <= level && level < z1) || (z1 <= level && level < z4)) {

                QVector<double> cellX, cellY;
                double x, y;

                // 检查每条边
                if ((z1 <= level && level < z2) || (z2 <= level && level < z1)) {
                    interpolateEdge(j, i, z1, j + 1, i, z2, level, x, y);
                    cellX.append(x);
                    cellY.append(y);
                }
                if ((z2 <= level && level < z3) || (z3 <= level && level < z2)) {
                    interpolateEdge(j + 1, i, z2, j + 1, i + 1, z3, level, x, y);
                    cellX.append(x);
                    cellY.append(y);
                }
                if ((z3 <= level && level < z4) || (z4 <= level && level < z3)) {
                    interpolateEdge(j + 1, i + 1, z3, j, i + 1, z4, level, x, y);
                    cellX.append(x);
                    cellY.append(y);
                }
                if ((z4 <= level && level < z1) || (z1 <= level && level < z4)) {
                    interpolateEdge(j, i + 1, z4, j, i, z1, level, x, y);
                    cellX.append(x);
                    cellY.append(y);
                }

                if (!cellX.isEmpty()) {
                    contourX.push_back(cellX);
                    contourY.push_back(cellY);
                }
            }
        }
    }
}

// 在边上进行插值
void ContourPlotter::interpolateEdge(
    double x1, double y1, double z1,
    double x2, double y2, double z2,
    double level, double& x, double& y)
{
    double t = (level - z1) / (z2 - z1);
    x = x1 + t * (x2 - x1);
    y = y1 + t * (y2 - y1);
}


void ContourPlotter::savePlot(const QString& pngpath)
{
    customPlot->savePng(pngpath + "/con.png", 0, 0, 1.0, -1);
}

double ContourPlotter::estimateStep(const std::vector<double>& v)
{
    const double EPS = 1e-9;
    if (v.size() < 2) return 0.0;

    // 1. 去重并排序
    std::vector<double> uniq = v;
    std::sort(uniq.begin(), uniq.end());
    uniq.erase(std::unique(uniq.begin(), uniq.end(),
                           [&](double a, double b){ return std::fabs(a-b) < EPS; }),
               uniq.end());

    // 2. 相邻差分
    std::vector<double> diffs;
    diffs.reserve(uniq.size()-1);
    for (size_t i = 1; i < uniq.size(); ++i) {
        double d = uniq[i] - uniq[i-1];
        if (d > EPS) diffs.push_back(d);
    }
    if (diffs.empty()) return 0.0;

    // 3. 用中位数或众数做代表（这里用中位数）
    std::nth_element(diffs.begin(),
                     diffs.begin() + diffs.size()/2,
                     diffs.end());
    return diffs[diffs.size()/2];
}

void ContourPlotter::autoDetectStep()
{
    double sx = estimateStep(X);
    double sy = estimateStep(Y);

    // 设一个合理下限，避免过小导致巨量网格
    if (sx < 1e-6 || sy < 1e-6) {
        // 回退策略：十分之一百跨度
        double xmin = *std::min_element(X.begin(), X.end());
        double xmax = *std::max_element(X.begin(), X.end());
        double ymin = *std::min_element(Y.begin(), Y.end());
        double ymax = *std::max_element(Y.begin(), Y.end());
        sx = (xmax - xmin) / 100.0;
        sy = (ymax - ymin) / 100.0;
    }
    setStep(sx, sy);
}
