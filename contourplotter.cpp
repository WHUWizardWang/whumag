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

    // 处理边界情况
    if (x <= gridX.front()) return gridZ[0][0];
    if (x >= gridX.back()) return gridZ[0][gridX.size()-1];
    if (y <= gridY.front()) return gridZ[0][0];
    if (y >= gridY.back()) return gridZ[gridY.size()-1][0];

    // 找到最近的网格点
    int i = 0;
    int j = 0;

    // 查找x所在的区间
    for (size_t k = 0; k < gridX.size() - 1; ++k) {
        if (x >= gridX[k] && x <= gridX[k + 1]) {
            i = k;
            break;
        }
    }

    // 查找y所在的区间
    for (size_t k = 0; k < gridY.size() - 1; ++k) {
        if (y >= gridY[k] && y <= gridY[k + 1]) {
            j = k;
            break;
        }
    }

    // 确保索引不会越界
    i = std::min(i, static_cast<int>(gridX.size() - 2));
    j = std::min(j, static_cast<int>(gridY.size() - 2));

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

        // 计算插值权重
        double fx = (x - x1) / (x2 - x1);
        double fy = (y - y1) / (y2 - y1);

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
    if (X.empty() || Y.empty() || Z.empty()) {
        return;
    }
    // 确定网格尺寸
    int nx = 0;
    int ny = 0;

    // 找到第一个Y值改变的位置，这就是nx
    for (size_t i = 1; i < Y.size(); ++i) {
        if (std::abs(Y[i] - Y[0]) > 1e-3) {  // 使用小数值比较
            nx = i;
            break;
        }
    }

    // 计算ny
    if (nx > 0) {
        ny = Z.size() / nx;
    } else {
        qDebug() << "Error: Could not determine grid dimensions";
        return;
    }

    // 验证数据大小是否合理
    if (nx * ny != static_cast<int>(Z.size())) {
        qDebug() << "Error: Data size mismatch";
        return;
    }

    // 创建2D网格数据
    std::vector<std::vector<double>> Z_grid(ny, std::vector<double>(nx));
    for (int i = 0; i < ny; ++i) {
        for (int j = 0; j < nx; ++j) {
            Z_grid[i][j] = Z[i * nx + j];
        }
    }

    double xmin = *std::min_element(X.begin(), X.end());
    double xmax = *std::max_element(X.begin(), X.end());
    double ymin = *std::min_element(Y.begin(), Y.end());
    double ymax = *std::max_element(Y.begin(), Y.end());

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
            double x = xmin + (xmax - xmin) * j / (nx - 1);
            double y = ymin + (ymax - ymin) * i / (ny - 1);
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
    customPlot->xAxis->setLabel("X /km");
    customPlot->yAxis->setLabel("Y /km");
    // 为每个等值线级别计算并绘制等值线
    // for (double level : levels) {
    //     std::vector<QVector<double>> contourX, contourY;
    //     findContours(zi, level, contourX, contourY);

    //     // 绘制每条等值线
    //     for (size_t i = 0; i < contourX.size(); ++i) {
    //         QCPCurve *contourLine = new QCPCurve(customPlot->xAxis, customPlot->yAxis);

    //         // 将网格索引转换为实际坐标
    //         QVector<double> realX, realY;
    //         for (int j = 0; j < contourX[i].size(); ++j) {
    //             double x = xmin + (xmax - xmin) * contourX[i][j] / (nx - 1);
    //             double y = ymin + (ymax - ymin) * contourY[i][j] / (ny - 1);
    //             realX.append(x);
    //             realY.append(y);
    //         }

    //         contourLine->setData(realX, realY);
    //         contourLine->setPen(QPen(Qt::black, 1, Qt::SolidLine));
    //          // 在等值线中间位置添加标注
    //         if (realX.size() > 0) {
    //             // 选择等值线上的一个合适点进行标注（这里选择中间点）
    //             int midIndex = realX.size() / 2;

    //             // 创建文本标签
    //             QCPItemText *textLabel = new QCPItemText(customPlot);
    //             textLabel->position->setType(QCPItemPosition::ptPlotCoords);
    //             textLabel->position->setCoords(realX[midIndex], realY[midIndex]);

    //             // 设置标签文本（保留两位小数）
    //             textLabel->setText(QString::number(level, 'f', 2));

    //             // 设置标签样式
    //             textLabel->setFont(QFont("Arial", 8));
    //             textLabel->setPen(QPen(Qt::black));
    //             textLabel->setBrush(QBrush(Qt::white));
    //             textLabel->setPadding(QMargins(2, 2, 2, 2));

    //             // 可选：添加一个指向线，连接标签和等值线
    //             QCPItemLine *arrow = new QCPItemLine(customPlot);
    //             arrow->start->setParentAnchor(textLabel->bottom);
    //             arrow->end->setCoords(realX[midIndex], realY[midIndex]);
    //             arrow->setPen(QPen(Qt::black, 1, Qt::DashLine));
    //         }
    //     }
    // }
    // 为每个等值线级别计算并绘制等值线

    // 如果有RMS值，添加标题
    if (m_rms != 0.0) {
        customPlot->plotLayout()->insertRow(0);
        customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot,
                                                                      QString("RMSE: %1").arg(m_rms, 0, 'f', 4)));
    }

    // 调整轴范围并重绘
    customPlot->rescaleAxes();
    customPlot->replot();
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
