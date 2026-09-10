#include "accuracy.h"
#include "MagneticComplexityAnalyzer.h"

Accuracy::Accuracy(QObject *parent)
    : QObject(parent)
{
}

//读文件
void Accuracy::readData(const QString& filename,Geomagnetic::Datapoint& dataPoints) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Failed to open file: " << filename.toStdString() << std::endl;
        return;
    }

    QTextStream in(&file);

    // 定义可能的分隔符
    QVector<QChar> possibleDelimiters = {',', '\t', ';', ' '};
    QMap<QChar, int> delimiterScores;

    // 读取前10行来判断分隔符（或者文件的所有行，如果少于10行）
    QStringList sampleLines;
    for (int i = 0; i < 10 && !in.atEnd(); ++i) {
        sampleLines.append(in.readLine());
    }

    // 分析每个可能的分隔符
    for (const QChar& delimiter : possibleDelimiters) {
        QMap<int, int> fieldCountFrequency; // 映射字段数量到它的出现频率

        for (const QString& line : sampleLines) {
            if (line.trimmed().isEmpty() || line.startsWith("#")) continue; // 跳过空行和注释行

            QStringList fields;
            if (delimiter == ' ') {
                // 对于空格分隔符特殊处理，避免多个连续空格被视为多个分隔符
                fields = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            } else {
                fields = line.split(delimiter, Qt::SkipEmptyParts);
            }

            // 只考虑字段数大于等于3的情况（因为我们需要经度、纬度和磁力值）
            if (fields.size() >= 3) {
                fieldCountFrequency[fields.size()]++;
            }
        }

        // 计算这个分隔符的得分（最常见的字段数的频率）
        int maxFrequency = 0;
        for (int frequency : fieldCountFrequency.values()) {
            if (frequency > maxFrequency) {
                maxFrequency = frequency;
            }
        }

        delimiterScores[delimiter] = maxFrequency;
    }

    // 选择得分最高的分隔符
    QChar bestDelimiter = ','; // 默认使用逗号
    int bestScore = 0;

    for (auto it = delimiterScores.begin(); it != delimiterScores.end(); ++it) {
        if (it.value() > bestScore) {
            bestScore = it.value();
            bestDelimiter = it.key();
        }
    }

    // 如果找不到合适的分隔符，保持使用逗号
    if (bestScore == 0) {
        std::cerr << "Warning: Could not determine delimiter, defaulting to comma." << std::endl;
    } else {
        // std::cout << "Using delimiter: " << (bestDelimiter == '\t' ? "TAB" :
        //                                          bestDelimiter == ' ' ? "SPACE" :
        //                                          QString(bestDelimiter).toStdString())
        //           << std::endl;
    }

    // 重置文件指针到开始位置
    file.seek(0);
    QTextStream newIn(&file);

    // 开始实际读取数据
    int index = 0;
    while (!newIn.atEnd()) {
        QString line = newIn.readLine();
        if (line.trimmed().isEmpty() || line.startsWith("#")) continue; // 跳过空行和注释行

        QStringList fields;
        if (bestDelimiter == ' ') {
            fields = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        } else {
            fields = line.split(bestDelimiter, Qt::SkipEmptyParts);
        }

        // 如果字段数为 3 或 4，才继续，否则报错并跳过
        if (fields.size() < 3 || fields.size() > 4) {
            std::cerr << "Error: Wrong number of fields in line: " << line.toStdString() << std::endl;
            continue;
        }

        // 公共的三列解析：lon, lat, mag
        bool ok1 = false, ok2 = false, ok3 = false, ok4 = false;
        double lon = fields[0].toDouble(&ok1);
        double lat = fields[1].toDouble(&ok2);
        double mag = fields[2].toDouble(&ok3);
        if (!ok1 || !ok2 || !ok3) {
            std::cerr << "Error: Invalid numeric data in line: " << line.toStdString() << std::endl;
            continue;
        }

        int keyIndex = 0; // 默认用自增索引（三列格式无第四列时保持默认值）
        if (fields.size() == 4) {
            // 如果是四列，就把第四列当成插入的索引
            keyIndex = fields[3].toInt(&ok4);
            if (!ok4) {
                std::cerr << "Error: Invalid index in fourth field: " << line.toStdString() << std::endl;
                continue;
            }
        }

        Geomagnetic::SinglePoint datapoint;
        datapoint.X = lon;
        datapoint.Y = lat;
        datapoint.lon = lon;
        datapoint.lat = lat;
        datapoint.tMagnetic = mag;
        datapoint.index = keyIndex; // 使用第四列作为索引（如果存在）

        // 将datapoint数据插入到dataPoints中
        dataPoints.insert(std::make_pair(index++, datapoint));
    }

    std::cout << "成功加载 " << index << " 个数据点。" << std::endl;
    file.close();
}

bool Accuracy::readDatawithComplexity(const QString& filename, QVector<Geomagnetic::ComplexPoint>& result) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Failed to open file: " << filename.toStdString() << std::endl;
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8"); // 设置UTF-8编码

    // 跳过BOM头（如果存在）
    QString firstChar = in.read(1);
    if (firstChar != QChar(0xFEFF)) {
        in.seek(0); // 如果不是BOM头，则回到文件开始
    }

    // 清空结果向量
    result.clear();

    // 判断文件分隔符
    QString headerLine = in.readLine();
    QChar delimiter = ','; // 默认使用逗号分隔符

    // 检查可能的分隔符
    QVector<QChar> possibleDelimiters = {',', '\t', ';', ' '};
    for (const QChar& d : possibleDelimiters) {
        if (headerLine.count(d) >= 5) { // 至少需要5个分隔符才能有6个字段
            delimiter = d;
            break;
        }
    }

    // 特殊处理空格分隔符
    bool isSpaceDelimiter = (delimiter == ' ');

    // 跳过剩余的标题行（如果有）
    bool hasHeader = false;
    if (headerLine.contains("行索引", Qt::CaseInsensitive) ||
        headerLine.contains("经度", Qt::CaseInsensitive) ||
        headerLine.contains("longitude", Qt::CaseInsensitive) ||
        headerLine.contains("lat", Qt::CaseInsensitive) ||
        headerLine.contains("格网", Qt::CaseInsensitive) ||
        headerLine.contains("索引", Qt::CaseInsensitive)) {
        // 这是标题行，已经读取了，继续处理
        hasHeader = true;
    } else {
        // 不是标题行，回到文件开始重新读取
        in.seek(0);

        // 再次检查BOM头
        firstChar = in.read(1);
        if (firstChar != QChar(0xFEFF)) {
            in.seek(0);
        }
    }

    int lineNumber = 1; // 从第一行开始计数

    // 定义默认字段索引（适应新的数据格式）
    int lonGridIndex = 0;    // 经度格网索引
    int latGridIndex = 1;    // 纬度格网索引
    int lonIndex = 2;        // 经度
    int latIndex = 3;        // 纬度
    int magValueIndex = 4;   // 磁异常值
    int spacingIndex = 5;    // 测线间距

    // 如果有标题行，尝试从标题识别字段位置
    if (hasHeader) {
        QStringList headers;
        if (isSpaceDelimiter) {
            headers = headerLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        } else {
            headers = headerLine.split(delimiter, Qt::SkipEmptyParts);
        }

        for (int i = 0; i < headers.size(); i++) {
            QString h = headers[i].toLower();
            // 根据标题内容确定各字段的位置
            if (h.contains("经度格网") || h.contains("行索引") || h.contains("列索引")) {
                lonGridIndex = i;
            } else if (h.contains("纬度格网") || h.contains("行索引")) {
                latGridIndex = i;
            } else if (h.contains("经度") || h.contains("lon")) {
                lonIndex = i;
            } else if (h.contains("纬度") || h.contains("lat")) {
                latIndex = i;
            } else if (h.contains("磁异常") || h.contains("磁场") || h.contains("mag")) {
                magValueIndex = i;
            } else if (h.contains("测线间距") || h.contains("spacing")) {
                spacingIndex = i;
            }
        }
    }

    // 读取数据行
    while (!in.atEnd()) {
        lineNumber++;
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue; // 跳过空行

        QStringList fields;
        if (isSpaceDelimiter) {
            fields = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        } else {
            fields = line.split(delimiter, Qt::SkipEmptyParts);
        }

        // 检查字段数量是否足够
        if (fields.size() < 6) {
            std::cerr << "Warning: Line " << lineNumber << " has insufficient fields ("
                      << fields.size() << " < 6), skipping: " << line.toStdString() << std::endl;
            continue;
        }

        // 确保索引在有效范围内
        int maxIndex = std::max({lonGridIndex, latGridIndex, lonIndex, latIndex, magValueIndex, spacingIndex});
        if (maxIndex >= fields.size()) {
            std::cerr << "Warning: Field index out of range on line " << lineNumber
                      << " (max index: " << maxIndex << ", fields: " << fields.size() << ")" << std::endl;
            continue;
        }

        // 解析数据字段
        bool ok[6] = {false, false, false, false, false, false};

        // 这些字段我们读取但不使用
        /*int lonGrid =*/ fields[lonGridIndex].toInt(&ok[0]);
        /*int latGrid =*/ fields[latGridIndex].toInt(&ok[1]);

        // 这些字段我们需要使用
        double lon = fields[lonIndex].toDouble(&ok[2]);
        double lat = fields[latIndex].toDouble(&ok[3]);
        double magValue = fields[magValueIndex].toDouble(&ok[4]);
        double spacing = fields[spacingIndex].toDouble(&ok[5]);

        // 验证所有必要字段是否都成功解析
        bool allValid = true;
        for (int i = 0; i < 6; i++) {
            if (!ok[i]) {
                allValid = false;
                break;
            }
        }

        if (!allValid) {
            std::cerr << "Warning: Invalid numeric data on line " << lineNumber
                      << ", skipping: " << line.toStdString() << std::endl;
            continue;
        }

        // 创建复杂度点对象并添加到结果集合
        Geomagnetic::ComplexPoint point;
        point.lon = lon;
        point.lat = lat;
        point.complexity = magValue;     // 用磁异常值作为基础磁场
        point.spacing = spacing;      // 测线间距
        point.baseMag = 0.0;       // 默认复杂度值为0（如果数据中没有）

        // 如果有复杂度字段且不同于已知字段，则尝试读取
        if (fields.size() > 6) {
            bool ok = false;
            double baseMag = fields[6].toDouble(&ok);
            if (ok) {
                point.baseMag = baseMag;
            }
        }

        result.append(point);
    }

    file.close();

    // 检查是否读取到数据
    if (result.isEmpty()) {
        std::cerr << "Error: No valid data found in file: " << filename.toStdString() << std::endl;
        return false;
    }

    std::cout << "Successfully read " << result.size() << " data points from file." << std::endl;

    return true;
}

void Accuracy::processData(const QString& filename1, const QString& filename2) {
    // 背景场数据
    Geomagnetic::Datapoint dataPoints1;
    // 检核线数据数据
    Geomagnetic::Datapoint dataPoints2;
    Geomagnetic::Datapoint dataresult;
    // 读取数据
    readData(filename1, dataPoints1);
    readData(filename2, dataPoints2);
    dataPoints2 = filterGeoMagneticData(dataPoints2);
    dataresult = dataPoints2;
    for(auto& elem : dataresult)
        elem.second.tMagnetic = 0;
    if (dataPoints1.empty() || dataPoints2.empty()) {
        std::cerr << "Failed to read data files." << std::endl;
        return;
    }
    // 提取数据点坐标和值
    std::vector<double> sfq_all_x, sfq_all_y, sfq_all_z;
    std::vector<double> line_data_x, line_data_y;

    // 提取背景场数据
    for (const auto& pair : dataPoints1) {
        sfq_all_x.push_back(pair.second.X);
        sfq_all_y.push_back(pair.second.Y);
        sfq_all_z.push_back(pair.second.tMagnetic);
    }

    // 提取检核线坐标
    for (const auto& pair : dataPoints2) {
        line_data_x.push_back(pair.second.X);
        line_data_y.push_back(pair.second.Y);
    }
    // 使用三次插值函数
    Geomagnetic::OptimizedCubicInterpolator cubicInterpolator;
    std::vector<double> extract_data_mag = cubicInterpolator.interpolate(
        sfq_all_x, sfq_all_y, sfq_all_z,
        line_data_x, line_data_y
        );

    // 将插值结果赋值给结果数据结构
    int i = 0;
    for (auto& pair : dataresult) {
        pair.second.tMagnetic = extract_data_mag[i++];
    }

    // 确保首点一致（与MATLAB代码保持一致）
    if (!dataPoints2.empty() && !dataresult.empty()) {
        dataresult.begin()->second.tMagnetic = dataPoints2.begin()->second.tMagnetic;
    }
    // 提取数据到向量中以便后续处理
    std::vector<double> line_data_mag;     // 检核线实际磁场值

    for (const auto& pair : dataPoints2) {
        line_data_mag.push_back(pair.second.tMagnetic);
    }

    // 确保两个向量长度相同
    if (line_data_mag.size() != extract_data_mag.size()) {
        std::cerr << "Error: Data vector lengths don't match!" << std::endl;
        return;
    }
    // 计算原始RMSE
    double sum_squared_diff = 0.0;
    std::vector<double> error_value(line_data_mag.size());
    for (size_t i = 0; i < line_data_mag.size(); ++i) {
        double diff = line_data_mag[i] - extract_data_mag[i];
        error_value[i] = diff;
        sum_squared_diff += diff * diff;
    }
    double rmse_o = std::sqrt(sum_squared_diff / line_data_mag.size());
    std::cout << "原始RMSE: " << rmse_o << " nT" << std::endl;

    // 使用移动平均提取趋势
    int windowSize = 140 * 8;  // 与MATLAB代码保持一致
    std::vector<double> trend_extract_data = movingAverage(extract_data_mag, windowSize);
    std::vector<double> trend_line_data = movingAverage(line_data_mag, windowSize);

    // 趋势线对齐和RMSE计算
    double rmse_trend_o, removeLength;
    std::vector<double> modified_trend;
    std::tie(rmse_trend_o, modified_trend, removeLength) = removeTrendLine(trend_extract_data, trend_line_data);

    // 输出结果
    std::cout << "趋势对齐后的RMSE: " << rmse_trend_o << " nT" << std::endl;
    std::cout << "最佳垂直偏移量: " << removeLength << " nT" << std::endl;

    for (auto& pair : dataresult) {
        pair.second.tMagnetic += removeLength;
    }

    // 可选：重新计算应用偏移后的RMSE
    // sum_squared_diff = 0.0;
    // for (size_t i = 0; i < dataPoints2.size(); ++i) {
    //     double diff = dataPoints2.at(i).tMagnetic - dataresult.at(i).tMagnetic;
    //     sum_squared_diff += diff * diff;
    // }
    // double final_rmse = std::sqrt(sum_squared_diff / dataPoints2.size());
    // std::cout << "应用偏移后的RMSE: " << final_rmse << " nT" << std::endl;
    // 新增：计算偏移后的两条平滑趋势线之间的RMSE
    std::vector<double> shifted_trend_extract(trend_extract_data.size());
    for (size_t i = 0; i < trend_extract_data.size(); ++i) {
        shifted_trend_extract[i] = trend_extract_data[i] + removeLength;
    }
    double trend_final_rmse = calculateRMSE(trend_line_data, shifted_trend_extract);
    std::cout << "应用偏移后的平滑趋势线RMSE: " << trend_final_rmse << " nT" << std::endl;

    // 可选：保存结果到文件
    // saveResults(dataPoints2, dataresult, modified_trend, trend_line_data);

}
// 移动平均函数
std::vector<double> Accuracy::movingAverage(const std::vector<double>& data, int windowSize) {
    std::vector<double> result(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        double sum = 0.0;
        int count = 0;

        int half_window = windowSize / 2;
        int start = std::max(0, static_cast<int>(i) - half_window);
        int end = std::min(static_cast<int>(data.size()) - 1, static_cast<int>(i) + half_window);

        for (int j = start; j <= end; ++j) {
            sum += data[j];
            count++;
        }

        result[i] = sum / count;
    }

    return result;
}

// 趋势线对齐和误差计算
std::tuple<double, std::vector<double>, double> Accuracy::removeTrendLine(
    const std::vector<double>& v1,
    const std::vector<double>& v2
    ) {
    if (v1.size() != v2.size() || v1.empty()) {
        return std::make_tuple(-1.0, std::vector<double>(), 0.0);
    }

    // 计算初始差异
    std::vector<double> c(v1.size());
    for (size_t i = 0; i < v1.size(); ++i) {
        c[i] = v2[i] - v1[i];
    }

    // 找到最小和最大差值
    double min_c = *std::min_element(c.begin(), c.end());
    double max_c = *std::max_element(c.begin(), c.end());

    // 在一个范围内寻找最优偏移量
    double best_offset = 0.0;
    double min_rmse = std::numeric_limits<double>::max();
    double step = 0.01;

    for (double offset = min_c - 5.0; offset <= max_c + 5.0; offset += step) {
        std::vector<double> modified_v1(v1.size());
        for (size_t i = 0; i < v1.size(); ++i) {
            modified_v1[i] = v1[i] + offset;
        }

        // 计算当前偏移下的RMSE
        double sum_squared_diff = 0.0;
        for (size_t i = 0; i < v1.size(); ++i) {
            double diff = modified_v1[i] - v2[i];
            sum_squared_diff += diff * diff;
        }
        double rmse = std::sqrt(sum_squared_diff / v1.size());

        if (rmse < min_rmse) {
            min_rmse = rmse;
            best_offset = offset;
        }
    }

    // 应用最佳偏移量
    std::vector<double> modified_vector(v1.size());
    for (size_t i = 0; i < v1.size(); ++i) {
        modified_vector[i] = v1[i] + best_offset;
    }

    return std::make_tuple(min_rmse, modified_vector, best_offset);
}
double Accuracy::calculateRMSE(const std::vector<double>& v1, const std::vector<double>& v2) {
    if (v1.size() != v2.size() || v1.empty()) {
        return -1.0;
    }

    double sum_squared_diff = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        double diff = v1[i] - v2[i];
        sum_squared_diff += diff * diff;
    }

    return std::sqrt(sum_squared_diff / v1.size());
}

Geomagnetic::Datapoint Accuracy::filterGeoMagneticData(
    const Geomagnetic::Datapoint& inputData,
    double minLon, double maxLon,
    double minLat, double maxLat
    ) {
    Geomagnetic::Datapoint filteredData;
    for (const auto& pair : inputData) {
        if (pair.second.X >= minLon && pair.second.X <= maxLon &&
            pair.second.Y >= minLat && pair.second.Y <= maxLat) {
            filteredData.insert(pair);
        }
    }
    return filteredData;
}

void Accuracy::dataResult(const Geomagnetic::Datapoint& dataPoints, QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Failed to open file: " << filename.toStdString() << std::endl;
        return;
    }
    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);
    for (const auto& pair : dataPoints) {
        out << pair.second.X << "," << pair.second.Y << "," << pair.second.tMagnetic << Qt::endl;
    }
    file.close();
}

void Accuracy::dataResult_for_autoreferencemap(const Geomagnetic::Datapoint& dataPoints, int height, QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Failed to open file: " << filename.toStdString() << std::endl;
        return;
    }
    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);
    for (const auto& pair : dataPoints) {
        out << pair.second.X << "," << pair.second.Y << "," << height << "," << pair.second.tMagnetic << Qt::endl;
    }
    file.close();
}

double Accuracy::computeCheckLineAccuracy(const QString& backgroundDataFile,
                                          const QString& checkLineDataFile,
                                          const QString& complexityDataFile,
                                          double targetSpacing,
                                          double spacingTolerance,
                                          double searchRadius)
{
    try {
        std::cout << "开始计算检核线精度..." << std::endl;

        // 背景场数据
        Geomagnetic::Datapoint backgroundData;
        // 检核线数据
        Geomagnetic::Datapoint checkLineData;
        Geomagnetic::Datapoint interpolatedData;

        // 读取数据
        std::cout << "正在读取背景场数据..." << std::endl;
        readData(backgroundDataFile, backgroundData);
        std::cout << "读取到 " << backgroundData.size() << " 个背景场数据点" << std::endl;

        std::cout << "正在读取检核线数据..." << std::endl;
        readData(checkLineDataFile, checkLineData);
        std::cout << "读取到 " << checkLineData.size() << " 个检核线数据点" << std::endl;

        // 过滤检核线数据
        std::cout << "正在过滤检核线数据..." << std::endl;
        checkLineData = filterGeoMagneticData(checkLineData);
        interpolatedData = checkLineData;

        // 检查数据是否成功读取
        if (backgroundData.empty()) {
            std::cerr << "错误: 背景场数据为空。" << std::endl;
            return -1.0;
        }

        if (checkLineData.empty()) {
            std::cerr << "错误: 检核线数据为空。" << std::endl;
            return -1.0;
        }

        // 复杂度数据（必须提供）
        QVector<Geomagnetic::ComplexPoint> complexityData;

        std::cout << "正在读取复杂度数据..." << std::endl;
        if (complexityDataFile.isEmpty()) {
            std::cerr << "错误: 未提供复杂度数据文件。" << std::endl;
            return -1.0;
        }

        bool readSuccess = readDatawithComplexity(complexityDataFile, complexityData);
        if (!readSuccess || complexityData.isEmpty()) {
            std::cerr << "错误: 无法读取复杂度数据或复杂度数据为空。" << std::endl;
            return -1.0;
        }

        std::cout << "读取到 " << complexityData.size() << " 个复杂度数据点" << std::endl;

        // 初始化插值结果数据
        for (auto& elem : interpolatedData) {
            elem.second.tMagnetic = 0;
        }

        // 提取数据点坐标和值
        std::cout << "正在提取坐标和磁场值..." << std::endl;
        std::vector<double> background_x, background_y, background_z;
        std::vector<double> checkLine_x, checkLine_y;
        std::vector<double> checkLine_values;   // 检核线实际磁场值

        // 提取背景场数据
        background_x.reserve(backgroundData.size());
        background_y.reserve(backgroundData.size());
        background_z.reserve(backgroundData.size());

        for (const auto& pair : backgroundData) {
            if (std::isfinite(pair.second.X) && std::isfinite(pair.second.Y) && std::isfinite(pair.second.tMagnetic)) {
                background_x.push_back(pair.second.X);
                background_y.push_back(pair.second.Y);
                background_z.push_back(pair.second.tMagnetic);
            }
        }

        // 检查提取后的背景场数据是否有效
        if (background_x.empty() || background_x.size() != background_y.size() || background_x.size() != background_z.size()) {
            std::cerr << "错误: 提取的背景场数据无效或不完整。" << std::endl;
            return -1.0;
        }

        // 提取检核线坐标和磁场值
        checkLine_x.reserve(checkLineData.size());
        checkLine_y.reserve(checkLineData.size());
        checkLine_values.reserve(checkLineData.size());

        for (const auto& pair : checkLineData) {
            if (std::isfinite(pair.second.X) && std::isfinite(pair.second.Y) && std::isfinite(pair.second.tMagnetic)) {
                checkLine_x.push_back(pair.second.X);
                checkLine_y.push_back(pair.second.Y);
                checkLine_values.push_back(pair.second.tMagnetic);
            }
        }

        // 检查提取后的检核线数据是否有效
        if (checkLine_x.empty() || checkLine_x.size() != checkLine_y.size() || checkLine_x.size() != checkLine_values.size()) {
            std::cerr << "错误: 提取的检核线数据无效或不完整。" << std::endl;
            return -1.0;
        }

        // 使用三次插值函数
        std::cout << "正在进行插值计算..." << std::endl;
        Geomagnetic::OptimizedCubicInterpolator cubicInterpolator;
        std::vector<double> interpolated_values;

        try {
            interpolated_values = cubicInterpolator.interpolate(
                background_x, background_y, background_z,
                checkLine_x, checkLine_y
                );
        } catch (const std::exception& e) {
            std::cerr << "插值计算异常: " << e.what() << std::endl;
            return -1.0;
        }

        // 检查插值结果
        if (interpolated_values.size() != checkLine_values.size()) {
            std::cerr << "错误: 插值结果数量(" << interpolated_values.size()
                << ")与检核线数据点数量(" << checkLine_values.size() << ")不匹配。" << std::endl;
            return -1.0;
        }

        // 将插值结果赋值给结果数据结构
        std::cout << "正在更新插值结果..." << std::endl;
        int i = 0;
        for (auto& pair : interpolatedData) {
            if (i < static_cast<int>(interpolated_values.size())) {
                pair.second.tMagnetic = interpolated_values[i++];
            }
        }

        // 确保首点一致
        if (!checkLineData.empty() && !interpolatedData.empty() && !interpolated_values.empty() && !checkLine_values.empty()) {
            interpolatedData.begin()->second.tMagnetic = checkLineData.begin()->second.tMagnetic;
            interpolated_values[0] = checkLine_values[0];
        }

        // 计算原始RMSE（使用所有点）
        std::cout << "计算原始RMSE..." << std::endl;
        double sum_squared_diff = 0.0;
        int valid_count = 0;

        for (size_t i = 0; i < checkLine_values.size(); ++i) {
            if (i < interpolated_values.size() &&
                std::isfinite(checkLine_values[i]) &&
                std::isfinite(interpolated_values[i])) {

                double diff = checkLine_values[i] - interpolated_values[i];
                sum_squared_diff += diff * diff;
                valid_count++;
            }
        }

        if (valid_count == 0) {
            std::cerr << "错误: 没有有效数据点用于计算RMSE。" << std::endl;
            return -1.0;
        }

        double rmse_original = std::sqrt(sum_squared_diff / valid_count);
        std::cout << "所有点的原始RMSE: " << rmse_original << " nT (基于 " << valid_count << " 个有效点)" << std::endl;

        // 1. 先对所有点进行滑动平均和趋势对齐
        // 使用移动平均提取趋势
        std::cout << "正在进行滑动平均处理..." << std::endl;
        int windowSize = std::min(140 * 8, valid_count / 4);  // 确保窗口大小合理
        windowSize = std::max(windowSize, 5);  // 确保窗口至少有5个点

        if (windowSize % 2 == 0) {
            windowSize++;  // 确保窗口大小为奇数
        }

        std::cout << "使用窗口大小: " << windowSize << " 进行移动平均" << std::endl;

        std::vector<double> trend_interpolated, trend_checkLine;

        try {
            trend_interpolated = movingAverage(interpolated_values, windowSize);
            trend_checkLine = movingAverage(checkLine_values, windowSize);
        } catch (const std::exception& e) {
            std::cerr << "移动平均计算异常: " << e.what() << std::endl;
            return -1.0;
        }

        // 检查趋势线计算结果
        if (trend_interpolated.size() != trend_checkLine.size() || trend_interpolated.empty()) {
            std::cerr << "错误: 趋势线计算结果无效。" << std::endl;
            return -1.0;
        }

        // 趋势线对齐和RMSE计算
        std::cout << "正在进行趋势对齐..." << std::endl;
        double rmse_trend = 0.0, removeLength = 0.0;
        std::vector<double> modified_trend;

        try {
            std::tie(rmse_trend, modified_trend, removeLength) = removeTrendLine(trend_interpolated, trend_checkLine);
        } catch (const std::exception& e) {
            std::cerr << "趋势对齐计算异常: " << e.what() << std::endl;
            return -1.0;
        }

        // 输出结果
        std::cout << "所有点的趋势对齐后RMSE: " << rmse_trend << " nT" << std::endl;
        std::cout << "最佳垂直偏移量: " << removeLength << " nT" << std::endl;

        // 应用偏移到所有插值数据
        std::cout << "正在应用偏移校正..." << std::endl;
        std::vector<double> shifted_interpolated(interpolated_values.size());
        for (size_t i = 0; i < interpolated_values.size(); ++i) {
            shifted_interpolated[i] = interpolated_values[i] + removeLength;
        }

        // 更新interpolatedData中的值
        i = 0;
        for (auto& pair : interpolatedData) {
            if (i < static_cast<int>(shifted_interpolated.size())) {
                pair.second.tMagnetic = shifted_interpolated[i++];
            }
        }

        // 2. 现在，对所有校正后的数据筛选测线间距为目标值的点
        std::cout << "正在筛选测线间距为 " << targetSpacing << " 的点..." << std::endl;

        // 搜索半径（经纬度）
        searchRadius = 0.002;  // 约1000米的经纬度差

        // 首先从复杂度数据中筛选出测线间距接近目标值的点
        std::vector<Geomagnetic::ComplexPoint> targetSpacingPoints;
        targetSpacingPoints.reserve(complexityData.size() / 2); // 预分配内存

        for (const auto& point : complexityData) {
            if (std::isfinite(point.spacing) && std::abs(point.spacing - targetSpacing) <= spacingTolerance) {
                targetSpacingPoints.push_back(point);
            }
        }

        std::cout << "找到 " << targetSpacingPoints.size() << " 个测线间距接近 " << targetSpacing << " 的点" << std::endl;

        if (targetSpacingPoints.empty()) {
            std::cerr << "错误: 没有找到测线间距为 " << targetSpacing << " 的点!" << std::endl;
            return -2.0;
        }

        // 将这些点与校正后的检核线点匹配
        std::vector<size_t> selectedIndices;     // 存储符合条件的点索引
        std::vector<double> selected_checkLine;   // 筛选后的检核线磁场值
        std::vector<double> selected_interpolated; // 筛选后的插值磁场值

        selectedIndices.reserve(checkLineData.size() / 2);
        selected_checkLine.reserve(checkLineData.size() / 2);
        selected_interpolated.reserve(checkLineData.size() / 2);

        std::cout << "正在匹配检核线点..." << std::endl;
        int progressCounter = 0;
        int progressInterval = std::max(1, static_cast<int>(checkLineData.size() / 10)); // 每10%报告一次进度

        i = 0;
        for (const auto& pair : checkLineData) {
            // 定期报告进度
            if (++progressCounter % progressInterval == 0) {
                int percent = (progressCounter * 100) / checkLineData.size();
                std::cout << "匹配进度: " << percent << "%" << std::endl;
            }

            const Geomagnetic::SinglePoint& checkPoint = pair.second;

            // 检查点坐标是否有效
            if (!std::isfinite(checkPoint.lon) || !std::isfinite(checkPoint.lat)) {
                i++;
                continue;
            }

            // 查找最近的复杂度点
            double minDistance = std::numeric_limits<double>::max();
            bool isTargetSpacingPoint = false;

            for (const Geomagnetic::ComplexPoint& complexPoint : targetSpacingPoints) {
                if (!std::isfinite(complexPoint.lon) || !std::isfinite(complexPoint.lat)) {
                    continue;
                }

                double dx = checkPoint.lon - complexPoint.lon;
                double dy = checkPoint.lat - complexPoint.lat;
                double distance = std::sqrt(dx*dx + dy*dy);

                if (distance < minDistance && distance <= searchRadius) {
                    minDistance = distance;
                    isTargetSpacingPoint = true;
                }
            }

            // 如果该点与目标测线间距的复杂度点匹配，则记录
            if (isTargetSpacingPoint && i < static_cast<int>(checkLine_values.size()) && i < static_cast<int>(shifted_interpolated.size())) {
                if (std::isfinite(checkLine_values[i]) && std::isfinite(shifted_interpolated[i])) {
                    selectedIndices.push_back(i);
                    selected_checkLine.push_back(checkLine_values[i]);
                    selected_interpolated.push_back(shifted_interpolated[i]);
                }
            }

            i++;
        }

        std::cout << "匹配到 " << selectedIndices.size() << " 个符合条件的检核线点" << std::endl;

        if (selectedIndices.size() < 10) {  // 至少需要一定数量的点才有意义
            std::cerr << "错误: 匹配到的点数量太少，无法进行有效计算!" << std::endl;
            return -3.0;
        }

        // 3. 仅使用筛选后的点计算RMSE
        double filtered_rmse = 0.0;
        try {
            filtered_rmse = calculateRMSE(selected_checkLine, selected_interpolated);
        } catch (const std::exception& e) {
            std::cerr << "计算筛选点RMSE异常: " << e.what() << std::endl;
            return -1.0;
        }

        std::cout << "应用偏移后的筛选点RMSE (仅测线间距=" << targetSpacing << "的点): "
                  << filtered_rmse << " nT" << std::endl;

        // 计算原始筛选点的RMSE（无偏移校正）
        std::vector<double> original_interpolated;
        original_interpolated.reserve(selectedIndices.size());

        for (size_t idx : selectedIndices) {
            if (idx < interpolated_values.size()) {
                original_interpolated.push_back(interpolated_values[idx]);
            }
        }

        double original_filtered_rmse = 0.0;
        try {
            if (original_interpolated.size() == selected_checkLine.size() && !original_interpolated.empty()) {
                original_filtered_rmse = calculateRMSE(selected_checkLine, original_interpolated);
                std::cout << "原始筛选点的RMSE (仅测线间距=" << targetSpacing << "的点，无偏移校正): "
                          << original_filtered_rmse << " nT" << std::endl;
                std::cout << "偏移校正改善了筛选点的RMSE: "
                          << (original_filtered_rmse - filtered_rmse) << " nT" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "计算原始筛选点RMSE异常: " << e.what() << " (忽略此错误，继续执行)" << std::endl;
        }

        // 可选：对筛选后的点再次计算移动平均和趋势线RMSE
        try {
            if (selected_checkLine.size() >= windowSize) {
                std::vector<double> trend_selected_checkLine = movingAverage(selected_checkLine, windowSize);
                std::vector<double> trend_selected_interpolated = movingAverage(selected_interpolated, windowSize);

                if (trend_selected_checkLine.size() == trend_selected_interpolated.size() && !trend_selected_checkLine.empty()) {
                    double trend_filtered_rmse = calculateRMSE(trend_selected_checkLine, trend_selected_interpolated);
                    std::cout << "筛选点的平滑趋势线RMSE (仅测线间距=" << targetSpacing << "的点): "
                              << trend_filtered_rmse << " nT" << std::endl;
                    return trend_filtered_rmse;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "计算趋势线RMSE异常: " << e.what() << " (忽略此错误，继续执行)" << std::endl;
        }

        std::cout << "检核线精度计算完成。" << std::endl;

        // 返回使用筛选后点计算的RMSE作为最终结果
        return filtered_rmse;

    } catch (const std::exception& e) {
        std::cerr << "发生未预期的异常: " << e.what() << std::endl;
        return -999.0;
    } catch (...) {
        std::cerr << "发生未知异常!" << std::endl;
        return -999.0;
    }
}

double Accuracy::computeSparseCheckLineRMSE(const QStringList& checkLineDataFile,
                                            const QString& complexityDataFile,
                                            double searchRadius)
{
    try {
        // 背景场数据
        Geomagnetic::Datapoint dataPoints1;
        readData(complexityDataFile, dataPoints1);
        if (dataPoints1.empty()) {
            std::cerr << "Failed to read background data file." << std::endl;
            return -1;
        }

        // 分离可稀疏和不可稀疏区域的背景场数据
        Geomagnetic::Datapoint sparsableDataPoints;  // index=1的点
        Geomagnetic::Datapoint nonSparsableDataPoints;  // index=0的点

        for (const auto& pair : dataPoints1) {
            if (pair.second.index == 1) {
                sparsableDataPoints[pair.first] = pair.second;
            } else if (pair.second.index == 0) {
                nonSparsableDataPoints[pair.first] = pair.second;
            }
        }

        if (sparsableDataPoints.empty()) {
            std::cerr << "No sparsable data points found (index=1)." << std::endl;
            return -1;
        }

        // 计算可稀疏区域的测区范围和格网间距
        std::vector<double> lons, lats;
        lons.reserve(sparsableDataPoints.size());
        lats.reserve(sparsableDataPoints.size());

        for (const auto& pair : sparsableDataPoints) {
            lons.push_back(pair.second.X);  // 经度
            lats.push_back(pair.second.Y);  // 纬度
        }

        // 计算测区范围
        double min_lon = *std::min_element(lons.begin(), lons.end());
        double max_lon = *std::max_element(lons.begin(), lons.end());
        double min_lat = *std::min_element(lats.begin(), lats.end());
        double max_lat = *std::max_element(lats.begin(), lats.end());

        // 计算平面距离范围
        double center_lat = (min_lat + max_lat) / 2.0;
        double lon_range_m = max_lon - min_lon;
        double lat_range_m = max_lat - min_lat;

        // 计算格网间距（假设为规则格网）
        std::set<double> unique_lons(lons.begin(), lons.end());
        std::set<double> unique_lats(lats.begin(), lats.end());

        double lon_spacing = 0.0;
        double lat_spacing = 0.0;

        if (unique_lons.size() > 1) {
            auto it = unique_lons.begin();
            double prev_lon = *it++;
            std::vector<double> lon_diffs;
            while (it != unique_lons.end()) {
                lon_diffs.push_back(*it - prev_lon);
                prev_lon = *it++;
            }
            lon_spacing = *std::min_element(lon_diffs.begin(), lon_diffs.end());
        }

        if (unique_lats.size() > 1) {
            auto it = unique_lats.begin();
            double prev_lat = *it++;
            std::vector<double> lat_diffs;
            while (it != unique_lats.end()) {
                lat_diffs.push_back(*it - prev_lat);
                prev_lat = *it++;
            }
            lat_spacing = *std::min_element(lat_diffs.begin(), lat_diffs.end());
        }

        // 转换为平面距离
        double lon_spacing_m = lon_spacing * 111319 * std::cos(center_lat * M_PI / 180.0);
        double lat_spacing_m = lat_spacing * 111319;
        searchRadius = lon_spacing_m * 2;

        // 构建空间索引来优化性能
        HierarchicalGridIndex spatialIndex;
        spatialIndex.buildIndex(sparsableDataPoints, center_lat);

        // 提取可稀疏区域的背景场数据（移到循环外，避免重复计算）
        std::vector<double> sfq_all_x, sfq_all_y, sfq_all_z;
        sfq_all_x.reserve(sparsableDataPoints.size());
        sfq_all_y.reserve(sparsableDataPoints.size());
        sfq_all_z.reserve(sparsableDataPoints.size());

        for (const auto& pair : sparsableDataPoints) {
            sfq_all_x.push_back(pair.second.X);
            sfq_all_y.push_back(pair.second.Y);
            sfq_all_z.push_back(pair.second.tMagnetic);
        }

        // 用于加权RMSE的变量（需要线程安全）
        double sum_weighted_sq_rmse = 0.0;
        int total_points = 0;
        int total_filtered_points = 0;

        // 使用互斥锁保护进度更新和输出
        std::mutex progress_mutex;
        std::mutex output_mutex;
        std::atomic<int> completed_files(0);

        // 验证输入数据
        if (checkLineDataFile.isEmpty()) {
            std::cerr << "No checkline files provided." << std::endl;
            return -1;
        }

// OpenMP并行处理检核线
#pragma omp parallel for reduction(+:sum_weighted_sq_rmse,total_points,total_filtered_points) schedule(dynamic)
        for (int i = 0; i < checkLineDataFile.size(); ++i) {
            try {
                const QString& filepath = checkLineDataFile[i];

                Geomagnetic::Datapoint checkLineData_old;
                Geomagnetic::Datapoint checkLineData;

                // 安全的文件读取
                readData(filepath, checkLineData_old);


                // 每隔五个点选取一个点 - 改进版本
                checkLineData.clear();
                int index_sparse = 0;
                for (const auto& pair : checkLineData_old) {
                    if (index_sparse % 5 == 0) {
                        checkLineData[pair.first] = pair.second;
                    }
                    index_sparse++;
                }

                if (checkLineData.empty()) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    std::cerr << "No data after sparse sampling for file: " << filepath.toStdString() << std::endl;
                    continue;
                }

                // 过滤数据
                checkLineData = filterGeoMagneticData(checkLineData);
                if (checkLineData.empty()) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    std::cerr << "No data after filtering for file: " << filepath.toStdString() << std::endl;
                    continue;
                }

                // 筛选检核线点：只保留在可稀疏区域附近的点
                std::vector<double> line_data_x, line_data_y, line_data_mag;
                int original_points = checkLineData.size();

                // 预分配空间
                line_data_x.reserve(original_points);
                line_data_y.reserve(original_points);
                line_data_mag.reserve(original_points);

                for (const auto& pair : checkLineData) {
                    double check_x = pair.second.X;
                    double check_y = pair.second.Y;

                    // 使用空间索引优化查询
                    bool inSparsableArea = spatialIndex.isInSparsableArea(check_x, check_y, searchRadius);

                    if (inSparsableArea) {
                        line_data_x.push_back(check_x);
                        line_data_y.push_back(check_y);
                        line_data_mag.push_back(pair.second.tMagnetic);
                    }
                }

                int filtered_points = original_points - line_data_x.size();
                total_filtered_points += filtered_points;

                int n_points = line_data_x.size();
                if (n_points == 0) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    std::cout << "[文件 " << filepath.toStdString() << "] 无可稀疏区域内的数据点，跳过" << std::endl;
                    continue;
                }

                // 插值（使用可稀疏区域的背景场数据）
                // 注意：每个线程需要创建自己的插值器实例
                Geomagnetic::OptimizedCubicInterpolator cubicInterpolator;
                std::vector<double> extract_data_mag;

                try {
                    extract_data_mag = cubicInterpolator.cubic_interpolate(
                        sfq_all_x, sfq_all_y, sfq_all_z,
                        line_data_x, line_data_y
                        );
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    std::cerr << "Interpolation failed for file: " << filepath.toStdString()
                              << ", error: " << e.what() << std::endl;
                    continue;
                }

                // 检查长度
                if (extract_data_mag.size() != line_data_mag.size()) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    std::cerr << "Data length mismatch for file: " << filepath.toStdString()
                              << " (extracted: " << extract_data_mag.size()
                              << ", original: " << line_data_mag.size() << ")" << std::endl;
                    continue;
                }

                // 原始RMSE
                double sum_squared_diff = 0.0;
                for (size_t j = 0; j < line_data_mag.size(); ++j) {
                    double diff = line_data_mag[j] - extract_data_mag[j];
                    sum_squared_diff += diff * diff;
                }
                double rmse_o = std::sqrt(sum_squared_diff / line_data_mag.size());

                // 移动平均提取趋势
                int windowSize = 140 * 8;
                if (windowSize >= static_cast<int>(extract_data_mag.size())) {
                    windowSize = std::max(1, static_cast<int>(extract_data_mag.size()) / 4);
                }

                std::vector<double> trend_extract_data = movingAverage(extract_data_mag, windowSize);
                std::vector<double> trend_line_data = movingAverage(line_data_mag, windowSize);

                // 趋势线对齐和RMSE
                double rmse_trend_o, removeLength;
                std::vector<double> modified_trend;

                try {
                    std::tie(rmse_trend_o, modified_trend, removeLength) =
                        removeTrendLine(trend_extract_data, trend_line_data);
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    std::cerr << "Trend removal failed for file: " << filepath.toStdString()
                              << ", error: " << e.what() << std::endl;
                    rmse_trend_o = rmse_o; // 使用原始RMSE作为备选
                }

                // 加权统计（使用reduction自动处理）
                sum_weighted_sq_rmse += rmse_trend_o * rmse_trend_o * n_points;
                total_points += n_points;

            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cerr << "Exception in processing file " << i << ": " << e.what() << std::endl;
                continue;
            }

            // 线程安全的进度更新 - 移到并行区域外
            int current_completed = ++completed_files;
            if (current_completed % 10 == 0 || current_completed == checkLineDataFile.size()) {
                std::lock_guard<std::mutex> lock(progress_mutex);
// 在主线程中发送信号
#pragma omp critical
                {
                }
            }
        }

        if (total_points == 0) {
            std::cerr << "No valid checkline data found in sparsable areas." << std::endl;
            return -1;
        }

        // 显示可稀疏区域背景场测区信息
        std::cout << "========== 背景场测区信息 ==========" << std::endl;
        std::cout << "经度范围: " << std::fixed << std::setprecision(6)
                  << min_lon << "° ~ " << max_lon << "° (跨度: "
                  << std::setprecision(6) << lon_range_m << " °)" << std::endl;
        std::cout << "纬度范围: " << std::fixed << std::setprecision(6)
                  << min_lat << "° ~ " << max_lat << "° (跨度: "
                  << std::setprecision(6) << lat_range_m << " °)" << std::endl;
        std::cout << "格网间距: 经向 " << std::fixed << std::setprecision(1)
                  << lon_spacing_m - 2 << " m, 纬向 " << lat_spacing_m - 2 << " m" << std::endl;
        std::cout << "====================================" << std::endl;

        double overall_rmse = std::sqrt(sum_weighted_sq_rmse / total_points);
        std::cout << "所有检核线总RMSE: " << std::setprecision(4) << overall_rmse << " nT" << std::endl;

        return overall_rmse;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error in computeSparseCheckLineRMSE: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown fatal error in computeSparseCheckLineRMSE" << std::endl;
        return -1;
    }
}
