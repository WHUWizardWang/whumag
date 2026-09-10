#include "MagneticComplexityAnalyzer.h" // 包含头文件

using namespace Geomagnetic;
MagneticComplexityAnalyzer::MagneticComplexityAnalyzer()
    : SUBAREA_SIZE(15), FEATURE_COUNT(6)
{
}

MagneticComplexityAnalyzer::~MagneticComplexityAnalyzer()
{
}
ComplexityResult MagneticComplexityAnalyzer::analyzeComplexity(const Datapoint& data, double gridSize,int jumpSize,int stepSize)
{
    ComplexityResult result;

    // 创建并预处理网格数据
    result.gridData = createGrid(data, gridSize);
    interpolateEmptyGrids(result.gridData);
    calculateGridGradients(result.gridData);

    // 收集子区特征
    std::vector<SubArea> subAreas;

    // 收集有效网格点的行列索引
    for (int i = jumpSize; i < result.gridData.rows-jumpSize; i += stepSize) {
        for (int j = jumpSize; j < result.gridData.cols-jumpSize; j += stepSize) {
            SubArea subArea = calculateSubAreaFeaturesFromGrid(result.gridData, i, j);
            if (subArea.magneticValues.size() > 0) {
                subAreas.push_back(subArea);
                // 保存对应的网格坐标
                result.rowIndices.push_back(i);
                result.colIndices.push_back(j);
            }
        }
    }

    // 使用PCA计算所有子区域的复杂度
    result.complexityValues = calculateComplexity(subAreas, result.gridData, result.rowIndices, result.colIndices);

    // 计算并存储对应的测线间距
    result.spacingValues.resize(result.complexityValues.size());
    for (size_t i = 0; i < result.complexityValues.size(); i++) {
        double complexity = result.complexityValues[i];
        double spacing = mapComplexityToSpacing(complexity);
        result.spacingValues[i] = spacing;

        // 同时更新网格数据中的复杂度和测线间距
        int row = result.rowIndices[i];
        int col = result.colIndices[i];
        result.gridData.grid[row][col].complexity = complexity;
        result.gridData.grid[row][col].surveySpacing = spacing;
    }

    return result;
}

std::vector<ComplexPoint> MagneticComplexityAnalyzer::convertIndicesToGeo(
    const ComplexityResult &result)
{
    std::vector<ComplexPoint> outVec;
    outVec.reserve(result.complexityValues.size());

    // 提取网格信息
    const auto &grid = result.gridData;
    double cellSize = grid.cellSize;
    double lonMin = grid.lonMin;
    double latMin = grid.latMin;
    // rows, cols, lonMax, latMax 也可以视需要读取

    // 遍历所有有效点
    for (size_t i = 0; i < result.complexityValues.size(); i++)
    {
        int row = result.rowIndices[i];
        int col = result.colIndices[i];

        // 计算真实经纬度
        double lon = lonMin + col * cellSize;
        double lat = latMin + row * cellSize;
        double baseMag = grid.grid[row][col].meanMagnetic;

        // 获取对应的 复杂度/测线间距
        double cplx = result.complexityValues[i];
        double sp   = result.spacingValues[i];

        // 组装进 outVec
        ComplexPoint cp { lon, lat, cplx, sp, baseMag };
        outVec.push_back(cp);
    }

    return outVec;
}

ComplexityResult MagneticComplexityAnalyzer::airanalyzeComplexity(const Datapoint& data, double gridSize,int jumpSize,int stepSize)
{
    ComplexityResult result;

    // 创建并预处理网格数据
    result.gridData = createGrid(data, gridSize);
    interpolateEmptyGrids(result.gridData);
    calculateGridGradients(result.gridData);

    // 收集子区特征
    std::vector<SubArea> subAreas;

    // 收集有效网格点的行列索引
    for (int i = jumpSize; i < result.gridData.rows-jumpSize; i += stepSize) {
        for (int j = jumpSize; j < result.gridData.cols-jumpSize; j += stepSize) {
            SubArea subArea = calculateSubAreaFeaturesFromGrid(result.gridData, i, j);
            if (subArea.magneticValues.size() > 0) {
                subAreas.push_back(subArea);
                // 保存对应的网格坐标
                result.rowIndices.push_back(i);
                result.colIndices.push_back(j);
            }
        }
    }

    // 使用PCA计算所有子区域的复杂度
    result.complexityValues = calculateComplexity(subAreas, result.gridData, result.rowIndices, result.colIndices);

    // 计算并存储对应的测线间距
    result.spacingValues.resize(result.complexityValues.size());
    for (size_t i = 0; i < result.complexityValues.size(); i++) {
        double complexity = result.complexityValues[i];
        double spacing = airComplexityToSpacing(complexity);
        result.spacingValues[i] = spacing;

        // 同时更新网格数据中的复杂度和测线间距
        int row = result.rowIndices[i];
        int col = result.colIndices[i];
        result.gridData.grid[row][col].complexity = complexity;
        result.gridData.grid[row][col].surveySpacing = spacing;
    }

    return result;
}

ComplexityResult MagneticComplexityAnalyzer::analyzeComplexityChunked(
    const Datapoint& data, double gridSize, int jumpSize, int chunkSize,bool air) {

    // 初始化结果和准备网格数据
    ComplexityResult finalResult;
    finalResult.gridData = createGrid(data, gridSize);
    //interpolateEmptyGrids(finalResult.gridData);
    calculateGridGradients(finalResult.gridData);

    const GridData& grid = finalResult.gridData;
    int rows = grid.rows, cols = grid.cols;
    const int M = FEATURE_COUNT;

    // 使用Qt的全局线程池
    QThreadPool* threadPool = QThreadPool::globalInstance();
    int maxThreadCount = threadPool->maxThreadCount();
    threadPool->setMaxThreadCount(maxThreadCount);  // 可以根据需要调整线程数

    // —— 第一遍：累加 sum 和 scatter（并行版本）——
    QMutex resultMutex;
    QAtomicInt totalCount(0);

    // 为结果创建容器
    Eigen::VectorXd globalSum = Eigen::VectorXd::Zero(M);
    Eigen::MatrixXd globalScatter = Eigen::MatrixXd::Zero(M, M);

    // 定义第一阶段的任务类
    class FirstPassTask : public QRunnable {
    public:
        FirstPassTask(const GridData& gridData, int startRow, int endRow,
                      int startCol, int endCol, int jumpSize, int M,
                      Eigen::VectorXd& globalSum, Eigen::MatrixXd& globalScatter,
                      QAtomicInt& totalCount, QMutex& mutex,
                      MagneticComplexityAnalyzer* analyzer)
            : gridData(gridData), startRow(startRow), endRow(endRow),
            startCol(startCol), endCol(endCol), jumpSize(jumpSize), M(M),
            globalSum(globalSum), globalScatter(globalScatter),
            totalCount(totalCount), mutex(mutex), analyzer(analyzer) {
            setAutoDelete(true);
        }

        void run() override {
            Eigen::VectorXd localSum = Eigen::VectorXd::Zero(M);
            Eigen::MatrixXd localScatter = Eigen::MatrixXd::Zero(M, M);
            int localCount = 0;

            for (int i = startRow; i < endRow; ++i) {
                for (int j = startCol; j < endCol; ++j) {
                    SubArea tmp;
                    analyzer->collectSubAreaFeatures(gridData, i, j, tmp);
                    if (tmp.magneticValues.empty()) continue;

                    Eigen::VectorXd x(M);
                    x << tmp.stdDev, tmp.meanValue, tmp.gradientMean, -tmp.kurtosis,
                        tmp.slopeStdDev, tmp.roughness;

                    localSum += x;
                    localScatter += x * x.transpose();
                    ++localCount;
                }
            }

            // 更新全局结果
            QMutexLocker locker(&mutex);
            globalSum += localSum;
            globalScatter += localScatter;
            locker.unlock();

            totalCount.fetchAndAddRelaxed(localCount);
        }

    private:
        const GridData& gridData;
        int startRow, endRow, startCol, endCol, jumpSize, M;
        Eigen::VectorXd& globalSum;
        Eigen::MatrixXd& globalScatter;
        QAtomicInt& totalCount;
        QMutex& mutex;
        MagneticComplexityAnalyzer* analyzer;
    };

    // 提交第一阶段的任务
    QVector<QFuture<void>> firstPassFutures;

    for (int startRow = jumpSize; startRow < rows - jumpSize; startRow += chunkSize) {
        for (int startCol = jumpSize; startCol < cols - jumpSize; startCol += chunkSize) {
            int endRow = qMin(startRow + chunkSize, rows - jumpSize);
            int endCol = qMin(startCol + chunkSize, cols - jumpSize);

            FirstPassTask* task = new FirstPassTask(
                finalResult.gridData, startRow, endRow, startCol, endCol,
                jumpSize, M, globalSum, globalScatter, totalCount, resultMutex, this
                );

            threadPool->start(task);
            // 创建QFuture对象来等待所有任务完成
            firstPassFutures.append(QtConcurrent::run(threadPool, []{/* 空任务，仅用于等待 */}));
        }
    }

    // 等待第一阶段所有任务完成
    for (QFuture<void>& future : firstPassFutures) {
        future.waitForFinished();
    }

    // 确保所有第一阶段任务已完成
    threadPool->waitForDone();

    // 检查是否有足够的数据点
    int count = totalCount.load();
    if (count < 2) return finalResult;

    // 计算均值和协方差
    Eigen::VectorXd mean = globalSum / double(count);
    Eigen::MatrixXd cov = (globalScatter / double(count - 1)) - (mean * mean.transpose());

    // 特征值分解
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);
    Eigen::VectorXd eigVals = solver.eigenvalues().reverse();
    Eigen::MatrixXd eigVecs = solver.eigenvectors().rowwise().reverse();
    Eigen::VectorXd contrib = eigVals / eigVals.sum();
    // 确保特征向量的符号一致
    for (int i = 0; i < eigVecs.cols(); i++) {
        // 找到绝对值最大的元素的索引
        int maxIdx;
        eigVecs.col(i).cwiseAbs().maxCoeff(&maxIdx);

        // 如果最大元素为负，则翻转特征向量
        if (eigVecs.col(i)(maxIdx) < 0) {
            eigVecs.col(i) = -eigVecs.col(i);
        }
    }

    // 预分配输出向量
    QVector<int> rowIndices;
    QVector<int> colIndices;
    QVector<double> complexityValues;
    QVector<double> spacingValues;

    // —— 第二遍：逐点计算复杂度 + 输出（并行版本）——
    // 定义第二阶段的任务类
    class SecondPassTask : public QRunnable {
    public:
        SecondPassTask(GridData& gridData, int startRow, int endRow,
                       int startCol, int endCol, int jumpSize, int M,
                       const Eigen::VectorXd& mean, const Eigen::MatrixXd& cov,
                       const Eigen::VectorXd& contrib, const Eigen::MatrixXd& eigVecs,
                       QVector<int>& rowIndices, QVector<int>& colIndices,
                       QVector<double>& complexityValues, QVector<double>& spacingValues,
                       QMutex& mutex, MagneticComplexityAnalyzer* analyzer,bool air)
            : gridData(gridData), startRow(startRow), endRow(endRow),
            startCol(startCol), endCol(endCol), jumpSize(jumpSize), M(M),
            mean(mean), cov(cov), contrib(contrib), eigVecs(eigVecs),
            rowIndices(rowIndices), colIndices(colIndices),
            complexityValues(complexityValues), spacingValues(spacingValues),
            mutex(mutex), analyzer(analyzer),air(air)  {
            setAutoDelete(true);
        }

        void run() override {
            QVector<int> localRowIndices;
            QVector<int> localColIndices;
            QVector<double> localComplexityValues;
            QVector<double> localSpacingValues;

            for (int i = startRow; i < endRow; ++i) {
                for (int j = startCol; j < endCol; ++j) {
                    SubArea tmp;
                    analyzer->collectSubAreaFeatures(gridData, i, j, tmp);
                    if (tmp.magneticValues.empty()) continue;

                    Eigen::VectorXd z(M);
                    z << tmp.stdDev, tmp.meanValue, tmp.gradientMean, -tmp.kurtosis,
                        tmp.slopeStdDev, tmp.roughness;

                    // z‑score 标准化
                    z = (z - mean).array() / cov.diagonal().array().sqrt();

                    double Ti = 0;
                    for (int g = 0; g < M; ++g) {
                        Ti += contrib(g) * eigVecs.col(g).dot(z);
                    }
                    double spacing = 0;
                    if(air)
                        spacing = analyzer->airComplexityToSpacing(Ti);
                    else
                        spacing = analyzer->mapComplexityToSpacing(Ti);
                    localRowIndices.append(i);
                    localColIndices.append(j);
                    localComplexityValues.append(Ti);
                    localSpacingValues.append(spacing);

                    // 将结果直接存储到网格中
                    QMutexLocker locker(&mutex);
                    gridData.grid[i][j].complexity = Ti;
                    gridData.grid[i][j].surveySpacing = spacing;
                    locker.unlock();
                }
            }

            // 更新全局结果
            QMutexLocker locker(&mutex);
            rowIndices.append(localRowIndices);
            colIndices.append(localColIndices);
            complexityValues.append(localComplexityValues);
            spacingValues.append(localSpacingValues);
        }

    private:
        GridData& gridData;
        int startRow, endRow, startCol, endCol, jumpSize, M;
        const Eigen::VectorXd& mean;
        const Eigen::MatrixXd& cov;
        const Eigen::VectorXd& contrib;
        const Eigen::MatrixXd& eigVecs;
        QVector<int>& rowIndices;
        QVector<int>& colIndices;
        QVector<double>& complexityValues;
        QVector<double>& spacingValues;
        QMutex& mutex;
        MagneticComplexityAnalyzer* analyzer;
        bool air;
    };

    // 提交第二阶段的任务
    QVector<QFuture<void>> secondPassFutures;

    for (int startRow = jumpSize; startRow < rows - jumpSize; startRow += chunkSize) {
        for (int startCol = jumpSize; startCol < cols - jumpSize; startCol += chunkSize) {
            int endRow = qMin(startRow + chunkSize, rows - jumpSize);
            int endCol = qMin(startCol + chunkSize, cols - jumpSize);

            SecondPassTask* task = new SecondPassTask(
                finalResult.gridData, startRow, endRow, startCol, endCol,
                jumpSize, M, mean, cov, contrib, eigVecs,
                rowIndices, colIndices, complexityValues, spacingValues,
                resultMutex, this,air
                );

            threadPool->start(task);
            secondPassFutures.append(QtConcurrent::run(threadPool, []{/* 空任务，仅用于等待 */}));
        }
    }

    // 等待第二阶段所有任务完成
    for (QFuture<void>& future : secondPassFutures) {
        future.waitForFinished();
    }

    // 确保所有第二阶段任务已完成
    threadPool->waitForDone();

    // 将QVector结果转换为std::vector（如果需要）
    finalResult.rowIndices.reserve(rowIndices.size());
    finalResult.colIndices.reserve(colIndices.size());
    finalResult.complexityValues.reserve(complexityValues.size());
    finalResult.spacingValues.reserve(spacingValues.size());

    for (int i = 0; i < rowIndices.size(); ++i) {
        finalResult.rowIndices.push_back(rowIndices[i]);
        finalResult.colIndices.push_back(colIndices[i]);
        finalResult.complexityValues.push_back(complexityValues[i]);
        finalResult.spacingValues.push_back(spacingValues[i]);
    }

    return finalResult;
}
// 新的辅助函数 - 直接在传入的对象上计算子区特征，避免创建和复制临时对象
void MagneticComplexityAnalyzer::collectSubAreaFeatures(
    const GridData& gridData, int centerRow, int centerCol, SubArea& outSubArea) {

    // 清空并准备对象重用
    outSubArea.magneticValues.clear();
    outSubArea.meanValue = 0;
    outSubArea.stdDev = 0;
    outSubArea.gradientMean = 0;
    outSubArea.kurtosis = 0;
    outSubArea.slopeStdDev = 0;
    outSubArea.roughness = 0;

    // 计算子区范围
    int halfSize = SUBAREA_SIZE / 2;
    int rowMin = std::max(0, centerRow - halfSize);
    int rowMax = std::min(gridData.rows - 1, centerRow + halfSize);
    int colMin = std::max(0, centerCol - halfSize);
    int colMax = std::min(gridData.cols - 1, centerCol + halfSize);

    // 预估容量并预分配
    int estimatedCells = (rowMax - rowMin + 1) * (colMax - colMin + 1);
    outSubArea.magneticValues.reserve(estimatedCells);

    // 收集子区数据
    for (int i = rowMin; i <= rowMax; i++) {
        for (int j = colMin; j <= colMax; j++) {
            if (gridData.grid[i][j].hasData) {
                outSubArea.magneticValues.push_back(gridData.grid[i][j].meanMagnetic);
            }
        }
    }

    // 如果没有足够的数据，则返回空子区
    if (outSubArea.magneticValues.size() < 3) {
        outSubArea.magneticValues.clear();
        return;
    }

    // 计算统计特征
    outSubArea = calculateSubAreaFeaturesFromGrid(gridData, centerRow, centerCol);
}

// 优化的合并预处理函数
void MagneticComplexityAnalyzer::interpolateAndCalculateGradients(GridData& gridData) {
    int rows = gridData.rows;
    int cols = gridData.cols;

    // 1. 首先进行插值 - 使用标志数组加速
    std::vector<std::vector<bool>> needsInterpolation(rows, std::vector<bool>(cols, false));
    int interpolationCount = 0;

    // 标记需要插值的单元格
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (!gridData.grid[i][j].hasData) {
                needsInterpolation[i][j] = true;
                interpolationCount++;
            }
        }
    }

    // 执行插值 - 仅处理标记的单元格
    if (interpolationCount > 0) {
        printf("需要插值的网格单元数: %d\n", interpolationCount);

        bool madeProgress = true;
        while (madeProgress && interpolationCount > 0) {
            madeProgress = false;

            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    if (needsInterpolation[i][j]) {
                        // 周围有数据的网格数量和磁场总和
                        int count = 0;
                        double sum = 0.0;

                        // 检查周围8个网格单元
                        for (int ni = std::max(0, i - 1); ni <= std::min(rows - 1, i + 1); ni++) {
                            for (int nj = std::max(0, j - 1); nj <= std::min(cols - 1, j + 1); nj++) {
                                if (gridData.grid[ni][nj].hasData) {
                                    count++;
                                    sum += gridData.grid[ni][nj].meanMagnetic;
                                }
                            }
                        }

                        // 如果周围有数据，进行插值
                        if (count > 0) {
                            gridData.grid[i][j].meanMagnetic = sum / count;
                            gridData.grid[i][j].hasData = true;
                            needsInterpolation[i][j] = false;
                            interpolationCount--;
                            madeProgress = true;
                        }
                    }
                }
            }
        }

        // 如果还有单元格无法插值，则使用全局平均值
        if (interpolationCount > 0) {
            double globalMean = 0.0;
            int validCount = 0;

            // 计算全局平均值
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    if (gridData.grid[i][j].hasData) {
                        globalMean += gridData.grid[i][j].meanMagnetic;
                        validCount++;
                    }
                }
            }

            if (validCount > 0) {
                globalMean /= validCount;

                // 填充剩余单元格
                for (int i = 0; i < rows; i++) {
                    for (int j = 0; j < cols; j++) {
                        if (needsInterpolation[i][j]) {
                            gridData.grid[i][j].meanMagnetic = globalMean;
                            gridData.grid[i][j].hasData = true;
                        }
                    }
                }
            }
        }
    }

    // 2. 计算梯度 - 使用缓存友好的遍历方式
    printf("计算梯度...\n");

    // 单次遍历计算所有梯度 - 避免多次遍历网格
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            auto& cell = gridData.grid[i][j];

            // 计算经度方向梯度(东西)
            if (j > 0 && j < cols - 1) {
                cell.gradLon = (gridData.grid[i][j+1].meanMagnetic - gridData.grid[i][j-1].meanMagnetic)
                / (2 * gridData.cellSize);
            } else if (j > 0) {
                cell.gradLon = (cell.meanMagnetic - gridData.grid[i][j-1].meanMagnetic)
                / gridData.cellSize;
            } else if (j < cols - 1) {
                cell.gradLon = (gridData.grid[i][j+1].meanMagnetic - cell.meanMagnetic)
                / gridData.cellSize;
            }

            // 计算纬度方向梯度(南北)
            if (i > 0 && i < rows - 1) {
                cell.gradLat = (gridData.grid[i+1][j].meanMagnetic - gridData.grid[i-1][j].meanMagnetic)
                / (2 * gridData.cellSize);
            } else if (i > 0) {
                cell.gradLat = (cell.meanMagnetic - gridData.grid[i-1][j].meanMagnetic)
                / gridData.cellSize;
            } else if (i < rows - 1) {
                cell.gradLat = (gridData.grid[i+1][j].meanMagnetic - cell.meanMagnetic)
                / gridData.cellSize;
            }

            // 东北方向梯度
            if (i > 0 && j < cols - 1) {
                cell.gradNE = (gridData.grid[i-1][j+1].meanMagnetic - cell.meanMagnetic)
                / (sqrt(2.0) * gridData.cellSize);
            }

            // 东南方向梯度
            if (i < rows - 1 && j < cols - 1) {
                cell.gradSE = (gridData.grid[i+1][j+1].meanMagnetic - cell.meanMagnetic)
                / (sqrt(2.0) * gridData.cellSize);
            }

            // 同时计算梯度标准差 - 内联计算减少内存访问
            std::array<double, 4> gradients = {
                fabs(cell.gradLon), fabs(cell.gradLat),
                fabs(cell.gradNE), fabs(cell.gradSE)
            };

            // 计算平均值
            double mean = (gradients[0] + gradients[1] + gradients[2] + gradients[3]) / 4.0;

            // 计算方差
            double variance = 0.0;
            for (int k = 0; k < 4; k++) {
                double diff = gradients[k] - mean;
                variance += diff * diff;
            }

            // 计算标准差
            cell.gradientStdDev = sqrt(variance / 4.0);
        }
    }
}

// 显示复杂度热图
void MagneticComplexityAnalyzer::showComplexityMap(const ComplexityResult& result,int jumpSize)
{
    // 创建显示复杂度热力图的对话框窗口
    QDialog* plotDialog = new QDialog();
    plotDialog->setWindowTitle("磁场复杂度热图");
    plotDialog->resize(800, 600);

    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(plotDialog);

    // 创建QCustomPlot对象
    QCustomPlot* customPlot = new QCustomPlot(plotDialog);
    layout->addWidget(customPlot);

    // 绘制复杂度热图
    plotComplexityMap(customPlot, result, jumpSize,false);

    // 添加关闭按钮
    QPushButton* closeButton = new QPushButton("关闭", plotDialog);
    layout->addWidget(closeButton);
    QObject::connect(closeButton, &QPushButton::clicked, plotDialog, &QDialog::accept);

    // 非阻塞方式显示对话框(对话框关闭后会自动释放内存)
    plotDialog->setAttribute(Qt::WA_DeleteOnClose);
    plotDialog->show();
}

// 显示测线间距热图
void MagneticComplexityAnalyzer::showSpacingMap(const ComplexityResult& result, int jumpSize)
{
    // 创建显示测线间距热力图的对话框窗口
    QDialog* plotDialog = new QDialog();
    plotDialog->setWindowTitle("测线间距建议热图");
    plotDialog->resize(800, 600);

    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(plotDialog);

    // 创建QCustomPlot对象
    QCustomPlot* customPlot = new QCustomPlot(plotDialog);
    layout->addWidget(customPlot);

    // 绘制测线间距热图
    plotComplexityMap(customPlot, result, jumpSize, true);

    // 添加关闭按钮
    QPushButton* closeButton = new QPushButton("关闭", plotDialog);
    layout->addWidget(closeButton);
    QObject::connect(closeButton, &QPushButton::clicked, plotDialog, &QDialog::accept);

    // 非阻塞方式显示对话框(对话框关闭后会自动释放内存)
    plotDialog->setAttribute(Qt::WA_DeleteOnClose);
    plotDialog->show();
}


void MagneticComplexityAnalyzer::plotComplexityMap(QCustomPlot* customPlot,
                                                   const ComplexityResult& result,
                                                   int jumpSize,
                                                   bool showSpacing)
{
    // 清除原有图形
    customPlot->clearPlottables();
    customPlot->clearItems();

    const GridData& gridData = result.gridData;
    const double GAMMA = 0.3;        // 0.3–0.6 常用，<1 抬低值 >1 压高值


    // 创建颜色图对象
    QCPColorMap* colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);

    // 设置颜色图大小
    int effectiveRows = gridData.rows - jumpSize * 2 ;  // 减去边缘的15*2行
    int effectiveCols = gridData.cols - jumpSize * 2 ;  // 减去边缘的15*2列

    // 安全检查 - 确保有效尺寸合理
    if (effectiveRows <= 0 || effectiveCols <= 0) {
        qWarning() << "有效网格尺寸无效，无法绘制热图";
        return;
    }

    // 计算有效区域的实际地理坐标范围（去除边缘区域）
    double lonRange = gridData.lonMax - gridData.lonMin;
    double latRange = gridData.latMax - gridData.latMin;

    // 计算每个网格单元对应的地理坐标步长
    double lonStep = lonRange / gridData.cols;
    double latStep = latRange / gridData.rows;

    // 计算有效显示区域的地理坐标范围（排除jumpSize边缘）
    double effectiveLonMin = gridData.lonMin + jumpSize * lonStep;
    double effectiveLonMax = gridData.lonMax - jumpSize * lonStep;
    double effectiveLatMin = gridData.latMin + jumpSize * latStep;
    double effectiveLatMax = gridData.latMax - jumpSize * latStep;


    // 限制最大尺寸以防止性能问题
    const int maxSize = 3000; // 最大尺寸限制
    int sampleStep = 1;

    if (effectiveRows > maxSize || effectiveCols > maxSize) {
        sampleStep = std::max(effectiveRows / maxSize, effectiveCols / maxSize);
        sampleStep = std::max(1, sampleStep);

        effectiveRows = effectiveRows / sampleStep;
        effectiveCols = effectiveCols / sampleStep;

        qDebug() << "热图尺寸过大，采用降采样步长:" << sampleStep;
    }

    // 设置实际颜色图尺寸
    colorMap->data()->setSize(effectiveCols, effectiveRows);
    colorMap->data()->setRange(QCPRange(effectiveLonMin, effectiveLonMax), QCPRange(effectiveLatMin, effectiveLatMax));

    // 设置颜色渐变
    QCPColorGradient gradient;
    if (!showSpacing) {
//        gradient = QCPColorGradient(QCPColorGradient::gpJet);
        gradient.clearColorStops();                 // 清空默认 stop
        gradient.setColorInterpolation(QCPColorGradient::ciRGB); // 线性 RGB
        gradient.setColorStopAt(0.13, QColor(242,161,167));   // #B7B5A0
        gradient.setColorStopAt(0.25, QColor(125,198,155));   // #44757A
        gradient.setColorStopAt(0.38, QColor(155,215,243));   // #452A3D
        gradient.setColorStopAt(0.50, QColor(251,221,221));   // #D44C3C
        gradient.setColorStopAt(0.63, QColor(252,230,207));   // #DD6C4C
        gradient.setColorStopAt(0.75, QColor(213,234,217));   // #452A3D
        gradient.setColorStopAt(0.88, QColor(216,238,251));   // #D44C3C
        gradient.setColorStopAt(1, QColor(220,215,235));   // #DD6C4C
        gradient.setLevelCount(256); // 设置渐变级别
    } else {
        //gradient = QCPColorGradient(QCPColorGradient::gpJet);
        gradient.clearColorStops();                 // 清空默认 stop
        gradient.setColorInterpolation(QCPColorGradient::ciRGB); // 线性 RGB
        gradient.setColorStopAt(0.00, QColor(219,49,36));   // #B7B5A0
        gradient.setColorStopAt(0.17, QColor(252,140,90));   // #44757A
        gradient.setColorStopAt(0.33, QColor(255,223,146));   // #452A3D
        gradient.setColorStopAt(0.50, QColor(230,241,243));   // #D44C3C
        gradient.setColorStopAt(0.67, QColor(144,190,244));   // #DD6C4C
        gradient.setColorStopAt(0.83, QColor(075,116,178));   // #E5855D
        gradient.setLevelCount(9); // 设置渐变级别
    }

    colorMap->setGradient(gradient);
    // 收集所有有效值进行统计分析
    std::vector<double> allValues;
    for (size_t i = 0; i < result.complexityValues.size(); i++) {
        int row = result.rowIndices[i];
        int col = result.colIndices[i];

        if (row >= jumpSize && row < gridData.rows - jumpSize &&
            col >= jumpSize && col < gridData.cols - jumpSize) {
            double value = showSpacing ? result.spacingValues[i] : result.complexityValues[i];
            if (std::isfinite(value)) { // 确保值是有限的
                allValues.push_back(value);
            }
        }
    }

    if (allValues.empty()) {
        qWarning() << "没有有效数据用于绘制";
        return;
    }

    // 计算统计信息以优化颜色映射
    std::sort(allValues.begin(), allValues.end());
    double minVal = allValues.front();
    double maxVal = allValues.back();

    // 计算百分位数来处理异常值并增强对比度
    size_t p1_idx = static_cast<size_t>(allValues.size() * 0.01);   // 1%分位数
    size_t p99_idx = static_cast<size_t>(allValues.size() * 0.99);  // 99%分位数
    size_t p5_idx = static_cast<size_t>(allValues.size() * 0.05);   // 5%分位数
    size_t p95_idx = static_cast<size_t>(allValues.size() * 0.95);  // 95%分位数

    double p1_val = allValues[p1_idx];
    double p99_val = allValues[p99_idx];
    double p5_val = allValues[p5_idx];
    double p95_val = allValues[p95_idx];
    double median = allValues[allValues.size() / 2];

    qDebug() << "数据统计: min=" << minVal << ", max=" << maxVal
             << ", median=" << median << ", P1=" << p1_val << ", P99=" << p99_val;

    // 智能选择显示范围以增强细节
    double displayMin, displayMax;

    // 策略：根据数据分布特征选择合适的显示范围
    double iqr = p95_val - p5_val;  // 四分位距
    double range = maxVal - minVal;

    if (iqr < range * 0.2) {
        // 如果大部分数据集中在很小的范围内，使用更紧的范围
        displayMin = p5_val;
        displayMax = p95_val;

        // 如果范围仍然太小，使用标准差方法
        if ((displayMax - displayMin) < range * 0.05) {
            double mean = std::accumulate(allValues.begin(), allValues.end(), 0.0) / allValues.size();
            double variance = 0.0;
            for (double val : allValues) {
                variance += (val - mean) * (val - mean);
            }
            double stddev = std::sqrt(variance / allValues.size());

            displayMin = mean - 1.5 * stddev;
            displayMax = mean + 1.5 * stddev;
            displayMin = std::max(displayMin, p1_val);
            displayMax = std::min(displayMax, p99_val);
        }
    } else {
        // 数据分布较均匀，使用1%-99%范围
        displayMin = p1_val;
        displayMax = p99_val;
    }

    qDebug() << "显示范围: [" << displayMin << ", " << displayMax << "]";

    // 创建值映射表
    std::map<std::pair<int, int>, double> valueMap;
    auto gammaMap = [=](double v)->double {
        if (displayMax - displayMin < 1e-12)   // 避免除零
            return v;
        double norm = (v - displayMin) / (displayMax - displayMin);   // 0-1
        norm        = std::pow(norm, GAMMA);                          // γ 拉伸
        return displayMin + norm * (displayMax - displayMin);         // 还原到数值域
    };

    // 填充颜色图数据
    for (size_t i = 0; i < result.complexityValues.size(); i++) {
        int row = result.rowIndices[i];
        int col = result.colIndices[i];

        if (row >= jumpSize && row < gridData.rows - jumpSize &&
            col >= jumpSize && col < gridData.cols - jumpSize) {

            double value = showSpacing ? result.spacingValues[i] : result.complexityValues[i];

            if (!std::isfinite(value)) continue;

            // 应用智能范围限制以增强对比度
            double clampedValue = std::max(displayMin, std::min(displayMax, value));
            double mappedValue    = gammaMap(clampedValue);
            valueMap[{row, col}] = mappedValue;

            // 映射到颜色图坐标
            int mapRow = (row - jumpSize) / sampleStep;
            int mapCol = (col - jumpSize) / sampleStep;

            if (mapRow >= 0 && mapRow < effectiveRows &&
                mapCol >= 0 && mapCol < effectiveCols) {
                colorMap->data()->setCell(mapCol, mapRow, clampedValue);
            }
        }
    }

    // 改进的插值算法（可选，提高视觉质量）
    bool enableInterpolation = (sampleStep > 1); // 只在降采样时进行插值

    if (enableInterpolation) {
        int interpolationStep = sampleStep;
        int maxInterpolationPoints = 500000;
        int interpolatedCount = 0;
        const int searchRadius = std::max(2, sampleStep);

        for (int row = jumpSize; row < gridData.rows - jumpSize; row += interpolationStep) {
            for (int col = jumpSize; col < gridData.cols - jumpSize; col += interpolationStep) {
                if (interpolatedCount >= maxInterpolationPoints) {
                    goto interpolationDone;
                }

                if (valueMap.find({row, col}) != valueMap.end()) {
                    continue;
                }

                // 反距离权重插值
                double weightedSum = 0.0;
                double weightSum = 0.0;

                for (int dr = -searchRadius; dr <= searchRadius; dr += 2) {
                    for (int dc = -searchRadius; dc <= searchRadius; dc += 2) {
                        int nr = row + dr;
                        int nc = col + dc;

                        auto it = valueMap.find({nr, nc});
                        if (it != valueMap.end()) {
                            double distSquared = dr*dr + dc*dc;
                            if (distSquared < 0.0001) distSquared = 0.0001;

                            double weight = 1.0 / std::sqrt(distSquared);
                            weightedSum += weight * it->second;
                            weightSum += weight;
                        }
                    }
                }

                if (weightSum > 0) {
                    double interpolatedValue = weightedSum / weightSum;
                    interpolatedValue = std::max(displayMin, std::min(displayMax, interpolatedValue));
                    interpolatedValue = gammaMap(interpolatedValue);

                    int mapRow = (row - jumpSize) / sampleStep;
                    int mapCol = (col - jumpSize) / sampleStep;

                    if (mapRow >= 0 && mapRow < effectiveRows &&
                        mapCol >= 0 && mapCol < effectiveCols) {
                        colorMap->data()->setCell(mapCol, mapRow, interpolatedValue);
                        interpolatedCount++;
                    }
                }
            }

            if (row % (interpolationStep * 20) == 0) {
                QCoreApplication::processEvents();
            }
        }

    interpolationDone:
        qDebug() << "插值完成，添加了" << interpolatedCount << "个插值点";
    }

    // 添加颜色图例
    QCPColorScale* colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorScale->setGradient(colorMap->gradient());
    colorMap->setColorScale(colorScale);

    // 设置颜色范围和标签
    colorMap->setDataRange(QCPRange(displayMin, displayMax));

    if (showSpacing) {
        colorScale->axis()->setLabel("建议测线间距 (km)");
    } else {
        colorScale->axis()->setLabel("磁场复杂度值");
    }

    // 添加标题
    customPlot->plotLayout()->insertRow(0);
    QCPTextElement* title = new QCPTextElement(customPlot);

    if (showSpacing) {
        title->setText("测线间距分布图");
    } else {
        title->setText("磁场复杂度分布图");
    }

    title->setFont(QFont("sans", 12, QFont::Bold));
    customPlot->plotLayout()->addElement(0, 0, title);

    // 设置坐标轴标签（根据你的坐标系类型选择）
    customPlot->xAxis->setLabel("经度 (°)");   // 或者 "东向坐标 (m)" 如果是投影坐标
    customPlot->yAxis->setLabel("纬度 (°)");   // 或者 "北向坐标 (m)" 如果是投影坐标

    // 启用交互功能
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // 启用平滑插值以改善视觉效果
    const int maxPixels = customPlot->viewport().width() * customPlot->viewport().height();
    if (effectiveRows * effectiveCols > maxPixels * 1.2) {   // 给一点冗余
        sampleStep = qCeil(qSqrt((effectiveRows * effectiveCols) /
                                 double(maxPixels * 1.2)));
    }

    colorMap->setInterpolate(sampleStep > 1);
    customPlot->setAntialiasedElement(QCP::aePlottables, false);
    customPlot->setBufferDevicePixelRatio(customPlot->devicePixelRatioF());

    // 更新显示
    customPlot->rescaleAxes();
    customPlot->replot();
}

// 将分析结果导出到文件
bool MagneticComplexityAnalyzer::exportToFile(const ComplexityResult& result, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QChar(0xFEFF); // 添加BOM头
    out << "lon,lat,baseMag,index"<<Qt::endl;

    const GridData& gridData = result.gridData;

    // 输出每个分析点的信息
    for (size_t i = 0; i < result.complexityValues.size(); i++) {
        int row = result.rowIndices[i];
        int col = result.colIndices[i];
        int index = 0;
        if(result.spacingValues[i]>=8)
            index = 1;
        // 计算实际经纬度
        double lon = gridData.lonMin + col * gridData.cellSize;
        double lat = gridData.latMin + row * gridData.cellSize;
        double baseMag = gridData.grid[row][col].meanMagnetic;

        out << lon << ","
            << lat << ","
            << baseMag << ","
            << index << "\n";
    }

    file.close();
    return true;
}

// 计算复杂度并将结果存储到网格中
std::vector<double> MagneticComplexityAnalyzer::calculateComplexity(
    const std::vector<SubArea>& subAreas,
    GridData& gridData,
    const std::vector<int>& rowIndices,
    const std::vector<int>& colIndices)
{
    const int M = FEATURE_COUNT;
    size_t N = subAreas.size();

    //第一遍：累加统计量
    Eigen::VectorXd sum = Eigen::VectorXd::Zero(M);
    Eigen::MatrixXd scatter = Eigen::MatrixXd::Zero(M, M);
    for (const auto& s : subAreas) {
        Eigen::VectorXd x(M);
        x << s.stdDev, s.meanValue, s.gradientMean, -s.kurtosis, s.slopeStdDev, s.roughness;
        sum += x;
        scatter += x * x.transpose();
    }
    Eigen::VectorXd mean = sum / double(N);
    Eigen::MatrixXd cov = (scatter / double(N - 1)) - (mean * mean.transpose());

    //特征分解
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);
    Eigen::VectorXd eigVals = solver.eigenvalues().reverse();
    Eigen::MatrixXd eigVecs = solver.eigenvectors().rowwise().reverse();
    Eigen::VectorXd contrib = eigVals / eigVals.sum();

    //第二遍：逐个子区计算复杂度（不保留完整矩阵）
    std::vector<double> complexities; complexities.reserve(N);
    for (const auto& s : subAreas) {
        Eigen::VectorXd z(M);
        z << s.stdDev, s.meanValue, s.gradientMean, -s.kurtosis, s.slopeStdDev, s.roughness;
        z = (z - mean).array() / cov.diagonal().array().sqrt(); // z‑score
        double Ti = 0;
        for (int g = 0; g < M; ++g) {
            Ti += contrib(g) * eigVecs.col(g).dot(z);
        }
        complexities.push_back(Ti);
    }
    return complexities;
}
SubArea MagneticComplexityAnalyzer::calculateSubAreaFeatures(const std::vector<SinglePoint>& points)
{
    SubArea subArea;
    int n = points.size();

    // 1. 计算平均值
    double sum = 0;
    for (const auto& point : points) {
        sum += point.tMagnetic;
        subArea.magneticValues.push_back(point.tMagnetic);
    }
    subArea.meanValue = sum / n;

    // 2. 计算标准差
    double variance = 0;
    for (const auto& value : subArea.magneticValues) {
        variance += pow(value - subArea.meanValue, 2);
    }
    subArea.stdDev = sqrt(variance / n);

    // 3. 计算累加梯度均值
    double totalGradient = 0;
    for (int i = 1; i < n; i++) {
        double dx = points[i].X - points[i - 1].X;
        double dy = points[i].Y - points[i - 1].Y;
        double dz = points[i].tMagnetic - points[i - 1].tMagnetic;
        if (dx != 0) totalGradient += abs(dz / dx);
        if (dy != 0) totalGradient += abs(dz / dy);
    }
    subArea.gradientMean = totalGradient / (2 * n);

    // 4. 计算峰态系数
    double m4 = 0;
    for (const auto& value : subArea.magneticValues) {
        m4 += pow(value - subArea.meanValue, 4);
    }
    subArea.kurtosis = (m4 / n) / pow(subArea.stdDev, 4);

    // 5. 计算坡度标准差
    std::vector<double> slopes;
    for (int i = 1; i < n; i++) {
        double dx = points[i].X - points[i - 1].X;
        double dy = points[i].Y - points[i - 1].Y;
        double dz = points[i].tMagnetic - points[i - 1].tMagnetic;
        if (dx != 0) slopes.push_back(atan(dz / dx));
        if (dy != 0) slopes.push_back(atan(dz / dy));
    }

    double slopeMean = 0;
    for (double slope : slopes) {
        slopeMean += slope;
    }
    slopeMean /= slopes.size();

    double slopeVariance = 0;
    for (double slope : slopes) {
        slopeVariance += pow(slope - slopeMean, 2);
    }
    subArea.slopeStdDev = sqrt(slopeVariance / slopes.size());
    //6.计算粗糙度
    double roughness = 0;


    return subArea;
}


std::vector<double> MagneticComplexityAnalyzer::calculateComplexity(const std::vector<SubArea>& subAreas)
{
    MatrixXd X(subAreas.size(), FEATURE_COUNT);

    // 1. 构建原始评价矩阵 X
    for (size_t i = 0; i < subAreas.size(); i++) {
        X(i, 0) = subAreas[i].stdDev;
        X(i, 1) = subAreas[i].meanValue;
        X(i, 2) = subAreas[i].gradientMean;
        X(i, 3) = -subAreas[i].kurtosis; // 同趋势化处理
        X(i, 4) = subAreas[i].slopeStdDev;
        X(i, 5) = subAreas[i].roughness;
    }

    // zscore 标准化：每一列减去均值并除以标准差
    MatrixXd Z = X;
    for (int j = 0; j < FEATURE_COUNT; j++) {
        double colMean = Z.col(j).mean();
        double colStd = std::sqrt((Z.col(j).array() - colMean).square().sum() / (Z.rows() - 1));
        if (colStd == 0) colStd = 1; // 防止除零
        Z.col(j) = (Z.col(j).array() - colMean) / colStd;
    }

    // 3. 计算相关矩阵R
    // MatrixXd R = (Z.transpose() * Z) / (Z.rows()-1);

    // // 4. 求解特征值和特征向量
    // SelfAdjointEigenSolver<MatrixXd> solver(R);
    // VectorXd eigenValues = solver.eigenvalues().reverse(); // 降序排列
    // MatrixXd eigenVectors = solver.eigenvectors();
    // // 特征向量需要按照特征值降序重新排列
    // eigenVectors = eigenVectors.rowwise().reverse();

    // // 5. 计算方差贡献率
    // double sumEigenValues = eigenValues.sum();
    // VectorXd contributionRates = eigenValues / sumEigenValues;

    // // 6. 计算每个子区的复杂度
    // std::vector<double> complexities;
    // complexities.reserve(subAreas.size());

    // for (int i = 0; i < subAreas.size(); i++) {
    //     double Ti = 0;
    //     for (int g = 0; g < FEATURE_COUNT; g++) {
    //         double Tig = 0;
    //         for (int j = 0; j < FEATURE_COUNT; j++) {
    //             Tig += eigenVectors(j, g) * Z(i, j);
    //         }
    //         Ti += contributionRates(g) * Tig;
    //     }
    //     complexities.push_back(Ti);
    // }

    // return complexities;
    // 计算样本协方差矩阵（除以 n-1）
    MatrixXd covMatrix = (Z.transpose() * Z) / (Z.rows() - 1);

    // 求解协方差矩阵的特征值和特征向量
    SelfAdjointEigenSolver<MatrixXd> solver(covMatrix);
    if (solver.info() != Success) {
        throw std::runtime_error("Eigen decomposition failed.");
    }
    VectorXd eigenValues = solver.eigenvalues();
    MatrixXd eigenVectors = solver.eigenvectors();

    // 将特征值和特征向量按降序排列
    VectorXd eigenValuesDesc(FEATURE_COUNT);
    MatrixXd eigenVectorsDesc(FEATURE_COUNT, FEATURE_COUNT);
    for (int i = 0; i < FEATURE_COUNT; i++) {
        eigenValuesDesc(i) = eigenValues(FEATURE_COUNT - 1 - i);
        eigenVectorsDesc.col(i) = eigenVectors.col(FEATURE_COUNT - 1 - i);
    }

    // 计算各主成分的方差贡献率
    double sumEigenValues = eigenValuesDesc.sum();
    VectorXd contributionRates = eigenValuesDesc / sumEigenValues;

    // 根据主成分和贡献率计算每个子区的复杂度得分
    std::vector<double> complexities;
    complexities.reserve(subAreas.size());
    for (int i = 0; i < Z.rows(); i++) {
        double Ti = 0;
        for (int g = 0; g < FEATURE_COUNT; g++) {
            double Tig = 0;
            for (int j = 0; j < FEATURE_COUNT; j++) {
                Tig += eigenVectorsDesc(j, g) * Z(i, j);
            }
            Ti += contributionRates(g) * Tig;
        }
        // complexities.push_back(Ti);
        // int row = indices[i].first;
        // int col = indices[i].second;
        // gridData.grid[row][col].complexity = Ti;
        // gridData.grid[row][col].surveySpacing = mapComplexityToSpacing(Ti);
    }
    return complexities;
}

// std::vector<double> MagneticComplexityAnalyzer::processData(const Datapoint& data, double gridSize,GridData& gridData) {
//     // 创建并预处理网格数据
//     gridData = createGrid(data, gridSize);
//     //int refineFactor = 2;
//     //GridInterpolator interpolator;
//     //gridData = interpolator.interpolateGrid(gridData, refineFactor);
//     interpolateEmptyGrids(gridData);
//     calculateGridGradients(gridData);

//     // 收集子区特征
//     std::vector<SubArea> subAreas;
//     int stepSize = 1; // 设置滑动步长

//     for (int i = 15; i < gridData.rows-15; i += stepSize) {
//         for (int j = 15; j < gridData.cols-15; j += stepSize) {
//             SubArea subArea = calculateSubAreaFeaturesFromGrid(gridData, i, j);
//             if (subArea.magneticValues.size() > 0) {
//                 subAreas.push_back(subArea);

//             }
//         }
//     }

//     // 计算复杂度
//     std::vector<double> complexityValues = calculateComplexity(subAreas);
//     plotComplexityMap(qcustomplot, gridData, complexityValues);
//     return complexityValues;
// }
std::vector<double> MagneticComplexityAnalyzer::processData(const Datapoint& data, double gridSize, GridData& gridData) {
    // 创建并预处理网格数据
    gridData = createGrid(data, gridSize);
    interpolateEmptyGrids(gridData);
    calculateGridGradients(gridData);

    // 收集子区特征
    std::vector<SubArea> subAreas;
    std::vector<int> rowIndices;
    std::vector<int> colIndices;
    int stepSize = 1; // 设置滑动步长

    for (int i = 15; i < gridData.rows-15; i += stepSize) {
        for (int j = 15; j < gridData.cols-15; j += stepSize) {
            SubArea subArea = calculateSubAreaFeaturesFromGrid(gridData, i, j);
            if (subArea.magneticValues.size() > 0) {
                subAreas.push_back(subArea);
                // 保存对应的网格坐标
                rowIndices.push_back(i);
                colIndices.push_back(j);
            }
        }
    }

    // 使用PCA计算所有子区域的复杂度
    std::vector<double> complexityValues = calculateComplexity(subAreas);

    // 创建显示复杂度热力图的对话框窗口
    QDialog* plotDialog = new QDialog();
    plotDialog->setWindowTitle("磁场复杂度热力图");
    plotDialog->resize(800, 600);

    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(plotDialog);

    // 创建QCustomPlot对象
    QCustomPlot* customPlot = new QCustomPlot(plotDialog);
    layout->addWidget(customPlot);

    // 创建颜色图对象
    QCPColorMap* colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);

    // 设置颜色图大小
    int effectiveRows = gridData.rows - 30;  // 减去边缘的15*2行
    int effectiveCols = gridData.cols - 30;  // 减去边缘的15*2列
    colorMap->data()->setSize(effectiveCols, effectiveRows);
    colorMap->data()->setRange(QCPRange(15, gridData.cols-15), QCPRange(15, gridData.rows-15));


    // 设置为Jet配色方案
    // double minComplexity = *std::min_element(complexityValues.begin(), complexityValues.end());
    // double maxComplexity = *std::max_element(complexityValues.begin(), complexityValues.end());
    // colorMap->setDataRange(QCPRange(-10, 10));
    QCPColorGradient jetGradient = QCPColorGradient(QCPColorGradient::gpJet);
    colorMap->setGradient(jetGradient);


    // 将复杂度值填入颜色图
    for (size_t i = 0; i < complexityValues.size(); i++) {
        int row = rowIndices[i];
        int col = colIndices[i];
        colorMap->data()->setCell(col-15, row-15, complexityValues[i]);
    }

    // 添加一个颜色图例
    QCPColorScale* colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("磁场复杂度值");

    // 重新缩放颜色图以覆盖数据范围
    colorMap->rescaleDataRange();

    // 设置轴标签
    customPlot->xAxis->setLabel("列索引 (格网坐标)");
    customPlot->yAxis->setLabel("行索引 (格网坐标)");

    // 启用交互功能
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // 更新画布
    colorMap->rescaleDataRange();
    customPlot->rescaleAxes();
    customPlot->replot();

    // 添加关闭按钮
    QPushButton* closeButton = new QPushButton("关闭", plotDialog);
    layout->addWidget(closeButton);
    QObject::connect(closeButton, &QPushButton::clicked, plotDialog, &QDialog::accept);

    // 非阻塞方式显示对话框(对话框关闭后会自动释放内存)
    plotDialog->setAttribute(Qt::WA_DeleteOnClose);
    plotDialog->show();

    // 返回计算的复杂度值
    return complexityValues;
}

bool MagneticComplexityAnalyzer::isInSubArea(const SinglePoint& center, const SinglePoint& point, double size)
{
    double dx = point.X - center.X;
    double dy = point.Y - center.Y;
    return (abs(dx) <= size / 2 && abs(dy) <= size / 2);
}

Geomagnetic::GridData MagneticComplexityAnalyzer::createGrid(const Datapoint& data, double gridSize)
{
    GridData gridData;
    gridData.cellSize = gridSize;

    // 计算数据范围
    gridData.lonMin = gridData.latMin = std::numeric_limits<double>::max();
    gridData.lonMax = gridData.latMax = std::numeric_limits<double>::lowest();

    for (const auto& point : data) {
        gridData.lonMin = std::min(gridData.lonMin, point.second.X);
        gridData.lonMax = std::max(gridData.lonMax, point.second.X);
        gridData.latMin = std::min(gridData.latMin, point.second.Y);
        gridData.latMax = std::max(gridData.latMax, point.second.Y);
    }

    // 计算网格维度
    gridData.cols = ceil((gridData.lonMax - gridData.lonMin) / gridSize) + 1;
    gridData.rows = ceil((gridData.latMax - gridData.latMin) / gridSize) + 1;

    // 初始化网格
    gridData.grid.resize(gridData.rows, std::vector<GridCell>(gridData.cols));

    // 将数据点分配到网格
    for (const auto& point_pair : data) {
        const auto& point = point_pair.second;
        int row = (point.lat - gridData.latMin) / gridSize;
        int col = (point.lon - gridData.lonMin) / gridSize;

        if (row >= 0 && row < gridData.rows && col >= 0 && col < gridData.cols) {
            gridData.grid[row][col].points.push_back(point);
            gridData.grid[row][col].hasData = true;
        }
    }

    // 计算每个网格的平均磁场值
    for (int i = 0; i < gridData.rows; i++) {
        for (int j = 0; j < gridData.cols; j++) {
            auto& cell = gridData.grid[i][j];
            if (cell.hasData) {
                double sum = 0;
                for (const auto& point : cell.points) {
                    sum += point.tMagnetic;
                }
                cell.meanMagnetic = sum / cell.points.size();
                cell.points.clear(); // 释放内存
                cell.points.shrink_to_fit();//释放内存
            }
        }
    }

    return gridData;
}

// 插值填充空网格
void MagneticComplexityAnalyzer::interpolateEmptyGrids(GridData& gridData) {
    // 收集有数据的网格点
    std::vector<double> x, y, z;
    for (int i = 0; i < gridData.rows; i++) {
        for (int j = 0; j < gridData.cols; j++) {
            if (gridData.grid[i][j].hasData) {
                // 计算网格中心点的经纬度
                double lat = gridData.latMin + i * gridData.cellSize;
                double lon = gridData.lonMin + j * gridData.cellSize;
                x.push_back(lon);
                y.push_back(lat);
                z.push_back(gridData.grid[i][j].meanMagnetic);
            }
        }
    }

    // 没有足够的数据点进行插值
    if (x.size() < 4) {
        // 如果数据点太少，使用简单的插值或返回
        return;
    }

    // 准备需要插值的网格点
    std::vector<double> xi, yi;
    std::vector<std::pair<int, int>> emptyIndices;

    for (int i = 0; i < gridData.rows; i++) {
        for (int j = 0; j < gridData.cols; j++) {
            if (!gridData.grid[i][j].hasData) {
                double lat = gridData.latMin + i * gridData.cellSize;
                double lon = gridData.lonMin + j * gridData.cellSize;
                xi.push_back(lon);
                yi.push_back(lat);
                emptyIndices.push_back({i, j});
            }
        }
    }

    // 如果没有需要插值的点，直接返回
    if (xi.empty()) {
        return;
    }

    // 执行三次样条插值
    std::vector<double> interpolatedValues = OptimizedCubicInterpolator::interpolate(x, y, z, xi, yi);

    // 填充插值结果到网格
    for (size_t k = 0; k < interpolatedValues.size(); k++) {
        int i = emptyIndices[k].first;
        int j = emptyIndices[k].second;
        gridData.grid[i][j].meanMagnetic = interpolatedValues[k];
        gridData.grid[i][j].hasData = true;
    }
}
// 计算网格梯度
void MagneticComplexityAnalyzer::calculateGridGradients(GridData& gridData) {
    // 先为内部点计算四个方向的梯度
    for (int i = 1; i < gridData.rows - 1; i++) {
        for (int j = 1; j < gridData.cols - 1; j++) {
            GridCell& cell = gridData.grid[i][j];

            // 跳过没有数据的单元格
            if (!cell.hasData) continue;

            // 检查周围8个邻居是否都有数据
            bool allNeighborsHaveData = true;
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    if (di == 0 && dj == 0) continue;
                    if (!gridData.grid[i+di][j+dj].hasData) {
                        allNeighborsHaveData = false;
                        break;
                    }
                }
                if (!allNeighborsHaveData) break;
            }

            if (!allNeighborsHaveData) continue;

            // 计算东向梯度（E方向）- 使用MATLAB中的权重计算
            cell.gradLon = ((gridData.grid[i+1][j+1].meanMagnetic + 2*gridData.grid[i][j+1].meanMagnetic + gridData.grid[i-1][j+1].meanMagnetic) -
                            (gridData.grid[i+1][j-1].meanMagnetic + 2*gridData.grid[i][j-1].meanMagnetic + gridData.grid[i-1][j-1].meanMagnetic)) / 8.0;

            // 计算北向梯度（N方向）
            cell.gradLat = ((gridData.grid[i+1][j+1].meanMagnetic + 2*gridData.grid[i+1][j].meanMagnetic + gridData.grid[i+1][j-1].meanMagnetic) -
                            (gridData.grid[i-1][j+1].meanMagnetic + 2*gridData.grid[i-1][j].meanMagnetic + gridData.grid[i-1][j-1].meanMagnetic)) / 8.0;

            // 计算东北向梯度（NE方向）
            cell.gradNE = (cell.gradLat + cell.gradLon) / sqrt(2.0);

            // 计算东南向梯度（SE方向）
            cell.gradSE = (cell.gradLat - cell.gradLon) / sqrt(2.0);
        }
    }

    // 为每个内部点计算梯度标准差
    for (int i = 1; i < gridData.rows - 1; i++) {
        for (int j = 1; j < gridData.cols - 1; j++) {
            GridCell& cell = gridData.grid[i][j];

            // 如果没有计算梯度，则跳过
            if (!cell.hasData || (cell.gradLon == 0 && cell.gradLat == 0)) continue;

            // 计算周围邻居的梯度平均值（包括当前单元格）
            double sumE = 0, sumN = 0, sumNE = 0, sumSE = 0;
            int count = 0;

            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;

                    // 确保索引在有效范围内
                    if (ni >= 1 && ni < gridData.rows - 1 &&
                        nj >= 1 && nj < gridData.cols - 1 &&
                        gridData.grid[ni][nj].hasData) {

                        sumE += gridData.grid[ni][nj].gradLon;
                        sumN += gridData.grid[ni][nj].gradLat;
                        sumNE += gridData.grid[ni][nj].gradNE;
                        sumSE += gridData.grid[ni][nj].gradSE;
                        count++;
                    }
                }
            }

            // 避免除以零
            if (count == 0) continue;

            double meanE = sumE / count;
            double meanN = sumN / count;
            double meanNE = sumNE / count;
            double meanSE = sumSE / count;

            // 计算标准差
            double varE = 0, varN = 0, varNE = 0, varSE = 0;

            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;

                    if (ni >= 1 && ni < gridData.rows - 1 &&
                        nj >= 1 && nj < gridData.cols - 1 &&
                        gridData.grid[ni][nj].hasData) {

                        varE += pow(gridData.grid[ni][nj].gradLon - meanE, 2);
                        varN += pow(gridData.grid[ni][nj].gradLat - meanN, 2);
                        varNE += pow(gridData.grid[ni][nj].gradNE - meanNE, 2);
                        varSE += pow(gridData.grid[ni][nj].gradSE - meanSE, 2);
                    }
                }
            }

            double stdE = sqrt(varE / count);
            double stdN = sqrt(varN / count);
            double stdNE = sqrt(varNE / count);
            double stdSE = sqrt(varSE / count);

            // 计算该点的总梯度标准差
            cell.gradientStdDev = stdE + stdN + stdNE + stdSE;
        }
    }
}

SubArea MagneticComplexityAnalyzer::calculateSubAreaFeaturesFromGrid(
    const GridData& gridData, int centerRow, int centerCol) {

    SubArea subArea;

    // 将SUBAREA_SIZE(km)转换为网格单元数量
    int radius =SUBAREA_SIZE;
    if (radius < 1) radius = 1;  // 确保至少有1个单元格

    // 收集子区域内的所有数据
    std::vector<double> magneticValues;
    std::vector<double> gradientsE;    // 东向梯度
    std::vector<double> gradientsN;    // 北向梯度
    std::vector<double> gradientsNE;   // 东北向梯度
    std::vector<double> gradientsSE;   // 东南向梯度
    std::vector<double> gradients;     // 总梯度幅值
    std::vector<double> slopes;        // 坡度

    for (int i = std::max(0, centerRow - radius);
         i <= std::min(gridData.rows - 1, centerRow + radius); i++) {
        for (int j = std::max(0, centerCol - radius);
             j <= std::min(gridData.cols - 1, centerCol + radius); j++) {

            const GridCell& cell = gridData.grid[i][j];
            if (!cell.hasData) continue;

            // 存储磁场值
            magneticValues.push_back(cell.meanMagnetic);

            // 存储各方向梯度
            gradientsE.push_back(cell.gradLon);
            gradientsN.push_back(cell.gradLat);
            gradientsNE.push_back(cell.gradNE);
            gradientsSE.push_back(cell.gradSE);

            // 计算总梯度幅值
            double gradMagnitude = sqrt(cell.gradLon * cell.gradLon + cell.gradLat * cell.gradLat +
                                        cell.gradNE * cell.gradNE + cell.gradSE * cell.gradSE);
            gradients.push_back(gradMagnitude);

            // 计算坡度 - 梯度与水平面的夹角
            double slope = atan(gradMagnitude);
            slopes.push_back(slope);
        }
    }

    // 如果子区域内没有有效数据点，返回空的subArea
    if (magneticValues.empty()) {
        return subArea;
    }

    // 存储收集到的磁场值
    subArea.magneticValues = magneticValues;

    // 1. 计算平均值
    double sum = std::accumulate(magneticValues.begin(), magneticValues.end(), 0.0);
    subArea.meanValue = sum / magneticValues.size();

    // 2. 计算标准差
    double variance = 0.0;
    for (double value : magneticValues) {
        double diff = value - subArea.meanValue;
        variance += diff * diff;
    }
    variance /= magneticValues.size();
    subArea.stdDev = sqrt(variance);

    // 3. 计算梯度均值 (基于所有方向梯度的幅值平均)
    subArea.gradientMean = std::accumulate(gradients.begin(), gradients.end(), 0.0) / gradients.size();

    // 4. 计算峰态系数
    double sumFourthMoment = 0.0;
    for (double value : magneticValues) {
        double diff = value - subArea.meanValue;
        sumFourthMoment += pow(diff, 4);
    }
    // 避免除以零
    if (subArea.stdDev > 0) {
        double fourthMoment = sumFourthMoment / magneticValues.size();
        subArea.kurtosis = fourthMoment / pow(subArea.stdDev, 4);
    } else {
        subArea.kurtosis = 0.0;
    }

    // 5. 计算坡度标准差 (完全按照MATLAB代码实现)
    // 计算四个方向梯度的平均值
    double meanE = 0.0, meanN = 0.0, meanNE = 0.0, meanSE = 0.0;
    int validCount = gradientsE.size();  // 所有梯度向量应该有相同的大小

    if (validCount > 0) {
        // 计算四个方向的平均值
        meanE = std::accumulate(gradientsE.begin(), gradientsE.end(), 0.0) / validCount;
        meanN = std::accumulate(gradientsN.begin(), gradientsN.end(), 0.0) / validCount;
        meanNE = std::accumulate(gradientsNE.begin(), gradientsNE.end(), 0.0) / validCount;
        meanSE = std::accumulate(gradientsSE.begin(), gradientsSE.end(), 0.0) / validCount;

        // 计算四个方向的平方差之和
        double sumSqE = 0.0, sumSqN = 0.0, sumSqNE = 0.0, sumSqSE = 0.0;

        for (int i = 0; i < validCount; i++) {
            sumSqE += pow(gradientsE[i] - meanE, 2);
            sumSqN += pow(gradientsN[i] - meanN, 2);
            sumSqNE += pow(gradientsNE[i] - meanNE, 2);
            sumSqSE += pow(gradientsSE[i] - meanSE, 2);
        }

        // 计算标准差 (使用相同的分母 validCount)
        double stdE = sqrt(sumSqE / validCount);
        double stdN = sqrt(sumSqN / validCount);
        double stdNE = sqrt(sumSqNE / validCount);
        double stdSE = sqrt(sumSqSE / validCount);

        // 坡度标准差是四个方向标准差之和
        subArea.slopeStdDev = stdE + stdN + stdNE + stdSE;
    } else {
        subArea.slopeStdDev = 0.0;
    }

    // 6. 计算粗糙度 (按MATLAB方法)
    double sum_data_x = 0.0, sum_data_y = 0.0;
    int count_x = 0, count_y = 0;

    // 在子区域范围内计算
    for (int i = std::max(0, centerRow - radius);
         i <= std::min(gridData.rows - 1, centerRow + radius); i++) {
        for (int j = std::max(0, centerCol - radius);
             j <= std::min(gridData.cols - 1, centerCol + radius) - 1; j++) {
            // 计算x方向(横向)的粗糙度 - 需要当前单元格和右侧相邻单元格都有数据
            if (gridData.grid[i][j].hasData && gridData.grid[i][j+1].hasData) {
                double diff_x = gridData.grid[i][j].meanMagnetic - gridData.grid[i][j+1].meanMagnetic;
                sum_data_x += diff_x * diff_x;
                count_x++;
            }
        }
    }

    // 计算y方向(纵向)的粗糙度
    for (int i = std::max(0, centerRow - radius);
         i <= std::min(gridData.rows - 1, centerRow + radius) - 1; i++) {
        for (int j = std::max(0, centerCol - radius);
             j <= std::min(gridData.cols - 1, centerCol + radius); j++) {
            // 需要当前单元格和下方相邻单元格都有数据
            if (gridData.grid[i][j].hasData && gridData.grid[i+1][j].hasData) {
                double diff_y = gridData.grid[i][j].meanMagnetic - gridData.grid[i+1][j].meanMagnetic;
                sum_data_y += diff_y * diff_y;
                count_y++;
            }
        }
    }

    // 计算最终粗糙度
    double r_x = 0.0, r_y = 0.0;
    if (count_x > 0) {
        r_x = pow(sum_data_x / count_x, 2);
    }
    if (count_y > 0) {
        r_y = pow(sum_data_y / count_y, 2);
    }

    // 计算平均粗糙度
    if (count_x > 0 && count_y > 0) {
        subArea.roughness = (r_x + r_y) / 2.0;
    } else if (count_x > 0) {
        subArea.roughness = r_x;
    } else if (count_y > 0) {
        subArea.roughness = r_y;
    } else {
        subArea.roughness = 0.0;  // 无足够数据计算粗糙度
    }

    return subArea;
}
void MagneticComplexityAnalyzer::plotComplexityMap(QCustomPlot* customPlot,
                                                   const GridData& gridData,
                                                   const std::vector<double>& complexityValues) {
    // 清除原有图形
    customPlot->clearPlottables();

    // 创建颜色图对象
    QCPColorMap* colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);

    // 设置颜色图大小为网格大小（减去边缘30行的部分）
    int effectiveRows = gridData.rows - 30;  // 减去边缘的15*2行
    int effectiveCols = gridData.cols - 30;  // 减去边缘的15*2列
    colorMap->data()->setSize(effectiveCols, effectiveRows);
    colorMap->data()->setRange(QCPRange(15, gridData.cols-15), QCPRange(15, gridData.rows-15));

    // 设置色彩梯度
    QCPColorGradient gradient;
    gradient.setColorStopAt(0, QColor(0, 0, 255));   // 蓝色 - 低复杂度
    gradient.setColorStopAt(0.5, QColor(0, 255, 0)); // 绿色 - 中等复杂度
    gradient.setColorStopAt(1, QColor(255, 0, 0));   // 红色 - 高复杂度
    colorMap->setGradient(gradient);

    // 复杂度值索引
    int complexityIndex = 0;

    // 将复杂度值填入颜色图
    for (int i = 15; i < gridData.rows-15; i++) {
        for (int j = 15; j < gridData.cols-15; j++) {
            if (complexityIndex < complexityValues.size()) {
                colorMap->data()->setCell(j-15, i-15, complexityValues[complexityIndex]);
                complexityIndex++;
            }
        }
    }

    // 添加一个颜色图例
    QCPColorScale* colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("磁场复杂度值");

    // 重新缩放颜色图以覆盖数据范围
    colorMap->rescaleDataRange();

    // 设置轴标签
    customPlot->xAxis->setLabel("列索引 (格网坐标)");
    customPlot->yAxis->setLabel("行索引 (格网坐标)");

    // 启用交互功能
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // 更新画布
    customPlot->rescaleAxes();
    customPlot->replot();
}
// 复杂度映射到测线间距
double MagneticComplexityAnalyzer::mapComplexityToSpacing(double complexity) {
    if (complexity > 5.53) return 0.5;
    else if (complexity > 3.52) return 2;
    else if (complexity > 2) return 2.5;
    else if (complexity > 1.19) return 3;
    else if (complexity > 0.75) return 3.5;
    else if (complexity > 0.2) return 4;
    else if (complexity > 0) return 5;
    else if (complexity > -0.2) return 7;
    else if (complexity > -0.34) return 8;
    else if (complexity > -0.72) return 8.5;
    else if (complexity > -1) return 9;
    else return 10;
}
void MagneticComplexityAnalyzer::setSUBAREA_SIZE(int &subarea_size)
{
    SUBAREA_SIZE = subarea_size;
}

double MagneticComplexityAnalyzer::airComplexityToSpacing(double complexity)
{
    if (complexity > 6) return 1;
    else if (complexity > 5.6) return 2;
    else if (complexity > 2.82) return 2.5;
    else if (complexity > 0.6) return 3;
    else if (complexity > -0.11) return 3.5;
    else if (complexity > -0.51) return 4;
    else if (complexity > -1.8) return 4.5;
    else return 5;
}

double MagneticComplexityAnalyzer::computeCheckLineAccuracy(
    const Geomagnetic::Datapoint & backgroundData,
    const Geomagnetic::Datapoint & checkLineData,
    double gridSize,
    int jumpSize,
    int stepSize)
{
    // 1) 计算背景场的复杂度
    ComplexityResult result = analyzeComplexityChunked(backgroundData, gridSize, jumpSize);
    auto geoVec = convertIndicesToGeo(result);

    if (geoVec.empty()) {
        qDebug() << "geoVec is empty, no valid subareas found.";
        return 0.0;
    }

    qDebug() << "[computeCheckLineAccuracyIDW] geoVec size=" << geoVec.size();

    // 2) 存储 spacing=10km 的误差数据
    std::vector<double> line_data_mag, extract_data_mag;
    line_data_mag.reserve(checkLineData.size());
    extract_data_mag.reserve(checkLineData.size());

    // 3) lambda 计算欧式距离
    auto dist2 = [](double x1, double y1, double x2, double y2){
        double dx = x1 - x2;
        double dy = y1 - y2;
        return dx*dx + dy*dy;
    };

    // 4) 遍历检核线点，用 IDW 计算复杂度 & 磁场
    for (const auto & kv : checkLineData)
    {
        const auto & pt = kv.second;
        double px = pt.X; // 检核线点的 (lon)
        double py = pt.Y; // 检核线点的 (lat)

        // ========== (A) IDW 计算 complexityIDW & baseMagIDW ===========
        double sumWeight = 0.0, sumCplx = 0.0, sumBaseMag = 0.0;

        for (size_t i = 0; i < geoVec.size(); i++)
        {
            double dx2 = dist2(px, py, geoVec[i].lon, geoVec[i].lat);
            if (dx2 < 1e-12)
            {
                sumWeight  = 1e9;
                sumCplx    = 1e9 * geoVec[i].complexity;
                sumBaseMag = 1e9 * geoVec[i].baseMag;
                break;
            }
            double w = 1.0 / std::pow(dx2, 1);
            sumWeight  += w;
            sumCplx    += w * geoVec[i].complexity;
            sumBaseMag += w * geoVec[i].baseMag;
        }

        if (sumWeight < 1e-15) continue; // IDW失败，跳过

        double complexityIDW = sumCplx / sumWeight;
        double baseMagIDW    = sumBaseMag / sumWeight;

        // ========== (B) 复杂度 => 测线间距 => 只筛选 spacing=10 ==========
        double sp = mapComplexityToSpacing(complexityIDW);
        if (sp + 7.9 > 1e-5)
        {
            line_data_mag.push_back(pt.tMagnetic);
            extract_data_mag.push_back(baseMagIDW);
        }
    }

    // 5) 确保数据量匹配
    if (line_data_mag.empty() || extract_data_mag.empty() ||
        line_data_mag.size() != extract_data_mag.size())
    {
        qDebug() << "[computeCheckLineAccuracyIDW] 没有 spacing=10 的检核线点.";
        return 0.0;
    }

    // 6) 计算 **原始 RMSE**
    double sum_squared_diff = 0.0;
    std::vector<double> error_value(line_data_mag.size());
    for (size_t i = 0; i < line_data_mag.size(); ++i)
    {
        double diff = line_data_mag[i] - extract_data_mag[i];
        error_value[i] = diff;
        sum_squared_diff += diff * diff;
    }
    double rmse_o = std::sqrt(sum_squared_diff / line_data_mag.size());
    qDebug() << "原始 RMSE: " << rmse_o << " nT";

    // 7) **使用移动平均滤波**
    int windowSize = 140 * 8;  // 保持一致
    std::vector<double> trend_extract_data = acc.movingAverage(extract_data_mag, windowSize);
    std::vector<double> trend_line_data = acc.movingAverage(line_data_mag, windowSize);

    // 8) **趋势线对齐 RMSE**
    double rmse_trend_o, removeLength;
    std::vector<double> modified_trend;
    std::tie(rmse_trend_o, modified_trend, removeLength) = acc.removeTrendLine(trend_extract_data, trend_line_data);

    qDebug() << "趋势对齐后的 RMSE: " << rmse_trend_o << " nT";
    qDebug() << "最佳垂直偏移量: " << removeLength << " nT";

    // 9) **调整基准磁场偏移**
    for (auto& mag : extract_data_mag)
        mag += removeLength;

    // 10) **最终趋势线 RMSE**
    std::vector<double> shifted_trend_extract(trend_extract_data.size());
    for (size_t i = 0; i < trend_extract_data.size(); ++i) {
        shifted_trend_extract[i] = trend_extract_data[i] + removeLength;
    }
    double trend_final_rmse = acc.calculateRMSE(trend_line_data, shifted_trend_extract);
    qDebug() << "应用偏移后的平滑趋势线 RMSE: " << trend_final_rmse << " nT";

    return trend_final_rmse; // 返回最终 RMSE
}
