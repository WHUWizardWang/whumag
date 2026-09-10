#include "iccp.h"


double ICCP::azimuth(double dx, double dy)
{
    double a_orb = 0.0;
    if(dy >0 && dx > 0)
        a_orb = atan(dy / dx);
    else if(dy >0 && dx < 0)
        a_orb = M_PI - atan(dy / abs(dx));
    else if(dy <0 && dx < 0)
        a_orb = M_PI + atan(dy / dx);
    else if(dy <0 && dx > 0)
        a_orb = 2*M_PI - atan(abs(dy) / dx);
    return a_orb;
}

QDebug operator<<(QDebug dbg, const Eigen::Matrix2d& matrix)
{
    dbg.nospace() << "Eigen::Matrix3f(" << matrix(0,0) << ", " << matrix(0,1)
                  << "; " << matrix(1,0) << ", " << matrix(1,1) << ")";
    return dbg.space();
}

void ICCP::setMinMax(double xmin, double xmax, double ymin, double ymax)
{
    this->xmin = xmin;
    this->xmax = xmax;
    this->ymin = ymin;
    this->ymax = ymax;
    xSize = qMax(1, int((xmax - xmin) / dx) + 1);
    ySize = qMax(1, int((ymax - ymin) / dy) + 1);
}

void ICCP::setDxDy(double x_step, double y_step)
{
    dx = x_step;
    dy = y_step;
    xSize = qMax(1, int((xmax - xmin) / dx) + 1);
    ySize = qMax(1, int((ymax - ymin) / dy) + 1);
}

QVector<QPointF> ICCP::eigenMatrixToQVector(const Eigen::MatrixXd& matrix)
{
    QVector<QPointF> points;
    points.reserve(matrix.cols()); // 预分配内存以提高效率

    for (int i = 0; i < matrix.cols(); ++i)
    {
        QPointF point(matrix(0,i), matrix(1,i));
        points.push_back(point);
    }
    return points;
}

QVector<QVector<double>> ICCP::ReadFile(const QString &fileName)
{
    xmin = 0.0;
    xmax = 5.0;
    ymin = 0.0;
    ymax = 5.0;
    dx = 1.0;
    dy = 1.0;
    QVector<QVector<double>> matrix;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file:" << fileName;
        return matrix;
    }
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList values = line.split(QRegExp("\\s+|,"), QString::SkipEmptyParts);
        QVector<double> rowData;
        foreach (const QString &value, values)
        {
            bool ok;
            double num = value.toDouble(&ok);
            if (ok)
            {
                rowData.push_back(num);
            }
            else
            {
                qWarning() << "Invalid number in file:" << fileName << ":" << value;
            }
        }
        if (!rowData.isEmpty())
        {
            matrix.push_back(rowData);
        }
    }
    file.close();
    return matrix;
}

QVector<QVector<double>> ICCP::ReadBackground(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ReadBackground: failed to open" << filePath;
        return {};  // 提前返回空
    }

    // 暂存合法点，并跟踪边界
    struct Triple { double x, y, z; };
    QVector<Triple> triples;
    triples.reserve(1024);

    double minX = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();

    QTextStream in(&file);
    int lineNo = 0;
    while (!in.atEnd()) {
        ++lineNo;
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // 用正则拆分空白或逗号
        QStringList fld = line.split(QRegularExpression("\\s+|,"), Qt::SkipEmptyParts);
        if (fld.size() != 3) {
            qWarning() << "ReadBackground: line" << lineNo
                       << "skipped (expect 3 fields, got" << fld.size() << ")";
            continue;
        }

        bool ok;
        double xx = fld[0].toDouble(&ok);
        if (!ok) { qWarning() << "ReadBackground: invalid X at line" << lineNo; continue; }
        double yy = fld[1].toDouble(&ok);
        if (!ok) { qWarning() << "ReadBackground: invalid Y at line" << lineNo; continue; }
        double zz = fld[2].toDouble(&ok);
        if (!ok) { qWarning() << "ReadBackground: invalid Z at line" << lineNo; continue; }

        // 更新边界
        minX = std::min(minX, xx);
        maxX = std::max(maxX, xx);
        minY = std::min(minY, yy);
        maxY = std::max(maxY, yy);

        triples.append({xx, yy, zz});
    }
    file.close();

    if (triples.isEmpty()) {
        qWarning() << "ReadBackground: no valid data in" << filePath;
        return {};
    }



    // 记录到成员变量
    xmin = minX;  xmax = maxX;
    ymin = minY;  ymax = maxY;

    // 计算网格尺寸
    xSize = qMax(1, int((xmax - xmin)/dx) + 1);
    ySize = qMax(1, int((ymax - ymin)/dy) + 1);

    // 分配并初始化网格
    QVector<QVector<double>> grid(xSize, QVector<double>(ySize, 0.0));

    // 填值：使用 qRound 保证围绕最近的格中心
    for (const auto &t : triples) {
        int ix = qRound((t.x - xmin) / dx);
        int iy = qRound((t.y - ymin) / dy);
        if (ix >= 0 && ix < xSize && iy >= 0 && iy < ySize) {
            grid[ix][iy] = t.z;
        }
    }

    return grid;
}

QVector<magPoint> ICCP::ReadINS(const QString &filePath)
{
    QVector<magPoint> points;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
           return points;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList fields = line.split(","); // 假设CSV字段由逗号分隔
        if (fields.size() < 3)
                continue;
        bool okX, okY, okValue;
        double x = fields[0].toDouble(&okX);
        double y = fields[1].toDouble(&okY);
        double value = fields[2].toDouble(&okValue);

        if (!okX || !okY || !okValue)
                continue;
        magPoint p;
        p.point = QPointF(x, y);
        p.value = value;
        points.push_back(p);
    }
    file.close();
    return points;
}

QVector<QVector<bool>> ICCP::isoCellGrid(QVector<QVector<double>> data, double isoValue)
{
    int m = data.size();
    int n = data[0].size();
    QVector<QVector<bool>> result(m,QVector<bool>(n));
    for (int i = 0; i<m ; i++)
    {
        for (int j = 0; j<n ; j++)
        {
            result[i][j] = ( data[i][j] - isoValue >= 0);
        }
    }
    return result;
}

int ICCP::getCellBit(bool v1,bool v2,bool v3, bool v4)
{
    // v1 左下, v2 右下, v3 右上, v4 左上
    return (v4 ? 1 : 0) << 3
           | (v3 ? 1 : 0) << 2
           | (v2 ? 1 : 0) << 1
           | (v1 ? 1 : 0);
}

QVector<QVector<int>> ICCP::getCellShift(QVector<QVector<bool>> bit)
{
    int m = bit.size();
    int n = bit[0].size();
    QVector<QVector<int>> result(m-1,QVector<int>(n-1));
    for (int i = 0; i<m-1 ; i++)
    {
        for (int j = 0; j<n-1 ; j++)
        {
            result[i][j] = getCellBit(bit[i+1][j],bit[i+1][j+1],bit[i][j+1],bit[i][j]);

        }
    }
    return result;
}

void ICCP::getLines(CountLine &result,
                    QVector<QVector<int>> matrix,
                    double isoValue,
                    QVector<QVector<double>> data)
{
    // 1. 定义 marching-squares 查表：每个 case 对应哪些边要连线
    // 边编号：0=bottom, 1=right, 2=top, 3=left
    static const QVector<QVector<QPair<int,int>>> edgeTable = {
        /*  0 */ {},
        /*  1 */ {{0,1}},
        /*  2 */ {{1,2}},
        /*  3 */ {{0,2}},
        /*  4 */ {{3,2}},
        /*  5 */ {{0,3},{1,2}},
        /*  6 */ {{3,1}},
        /*  7 */ {{0,3}},
        /*  8 */ {{0,3}},
        /*  9 */ {{3,1}},
        /* 10 */ {{0,1},{3,2}},
        /* 11 */ {{3,2}},
        /* 12 */ {{0,2}},
        /* 13 */ {{1,2}},
        /* 14 */ {{0,1}},
        /* 15 */ {}
    };

    int m = matrix.size();
    if (m == 0) return;
    int n = matrix[0].size();
    QVector<QLineF> segments;

    // 边插值函数：给出 cell (i,j) 上的哪一条边，计算等值线交点
    auto interp = [&](int i, int j, int edge)->QPointF {
        QPointF A, B;
        double vA, vB;
        switch (edge) {
        case 0: // bottom edge: (i,j)->(i+1,j)
            A = {double(i),   double(j)};
            B = {double(i+1), double(j)};
            vA = data[i][j];      vB = data[i+1][j];
            break;
        case 1: // right edge: (i+1,j)->(i+1,j+1)
            A = {double(i+1), double(j)};
            B = {double(i+1), double(j+1)};
            vA = data[i+1][j];    vB = data[i+1][j+1];
            break;
        case 2: // top edge: (i+1,j+1)->(i,j+1)
            A = {double(i+1), double(j+1)};
            B = {double(i),   double(j+1)};
            vA = data[i+1][j+1];  vB = data[i][j+1];
            break;
        case 3: // left edge: (i,j+1)->(i,j)
            A = {double(i),   double(j+1)};
            B = {double(i),   double(j)};
            vA = data[i][j+1];    vB = data[i][j];
            break;
        default:
            return {};
        }
        // 线性插值 t = (iso - vA)/(vB - vA)
        double t = (vB == vA) ? 0.5 : (isoValue - vA) / (vB - vA);
        // 限制 t 在 [0,1]
        t = qBound(0.0, t, 1.0);
        return QPointF(A.x() + t*(B.x()-A.x()),
                       A.y() + t*(B.y()-A.y()));
    };

    // 2. 遍历每个单元格（排除最外一圈，避免越界）
    for (int i = 0; i < m-1; ++i) {
        for (int j = 0; j < n-1; ++j) {
            int code = matrix[i][j] & 0xF;  // 保证在 [0,15]
            const auto &edges = edgeTable[code];
            // 对每一对边，生成一条线段
            for (const auto &pr : edges) {
                QPointF p1 = interp(i, j, pr.first);
                QPointF p2 = interp(i, j, pr.second);
                segments.push_back(QLineF(p1, p2));
            }
        }
    }

    // 3. 输出结果
    result.line  = segments;
    result.value = isoValue;
}

void ICCP::draw_lines(CountLine lines,double m,double n)
{
    int fileint = 0;
    QCustomPlot *customPlot = new QCustomPlot();
//    customPlot->yAxis->setRangeReversed(true);
    customPlot->xAxis->setRange(0,n);
    customPlot->yAxis->setRange(0,m);
    for (const QLineF &l : lines.line)
    {
        QCPGraph *graph = customPlot->addGraph();
        QVector<double> keys, values;
        keys.push_back(l.x1());
        values.push_back(l.y1());
        keys.push_back(l.x2());
        values.push_back(l.y2());
        graph->addData(keys,values);
    }
    customPlot->replot();
    // 创建并显示弹出窗口
//    QMainWindow *plotWindow = new QMainWindow();
//    QWidget *centralWidget = new QWidget();
//    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
//    layout->addWidget(customPlot);
//    plotWindow->setCentralWidget(centralWidget);
//    plotWindow->resize(800,600);
//    plotWindow->setWindowTitle("show");
//    plotWindow->show();
    QString str = "temp_" + QString::number(fileint) +"_"+QString::number(lines.value)+ ".jpg";
    customPlot->saveJpg(str,customPlot->width(),customPlot->height());
    fileint++;
}

QPointF ICCP::findNearestPointOnLine(const QLineF line, const QPointF p)
{
    QPointF nearestPoint;
    QVector2D lineVec(line.dx(), line.dy());
    QVector2D pointVec(p.x() - line.x1(), p.y() - line.y1());

    // 计算点p到线段的垂足
    float dot = QVector2D::dotProduct(lineVec, pointVec);
    float lenSq = lineVec.lengthSquared();

    if (dot <= 0.0f)
    {
        // 垂足在线段起点之前，返回起点
        nearestPoint = line.p1();
    }
    else if (dot >= lenSq)
    {
        // 垂足在线段终点之后，返回终点
        nearestPoint = line.p2();
    }
    else
    {
        // 垂足在线段上，计算其位置
        float t = dot / lenSq;
        nearestPoint = QPointF(line.x1() + t * line.dx(), line.y1() + t * line.dy());
    }
    return nearestPoint;
}

QPointF ICCP::findNearestPointOnLines(const CountLine result, const QPointF p)
{
    QPointF nearestPoint;
    double minDistance = std::numeric_limits<double>::max();

    for (const QLineF &line : result.line)
    {
        QPointF currentNearestPoint = findNearestPointOnLine(line, p);
        double distance = QLineF(p, currentNearestPoint).length();
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestPoint = currentNearestPoint;
        }
    }
//    nearestPoint = nearestPoint + QPointF(0.0,1.0);
    return nearestPoint;
}

QVector<QPointF> ICCP::get_PP(QVector<QPointF> insP,const CountLine result)
{
    QVector<QPointF> PP;
    for (const QPointF &p : insP)
    {
        PP.push_back(findNearestPointOnLines(result,p));
    }
    return PP;
}

QPointF ICCP::computeCentroid(const QVector<QPointF>& points)
{
    QPointF centroid(0.0, 0.0);
    for (const auto &point: points)
    {
        centroid += point;
    }
    centroid /= points.size();
    return centroid;
}

void ICCP::computeRotationMatrix(const QVector<QPointF>& points1, const QVector<QPointF>& points2,Eigen::Matrix2d &R,Eigen::MatrixXd &T)
{
    // 1. 计算质心
    Eigen::Vector2d centroid1 = Eigen::Vector2d::Zero(), centroid2 = Eigen::Vector2d::Zero();
    for (const QPointF &p : points1) centroid1 += Eigen::Vector2d(p.x(), p.y());
    for (const QPointF &p : points2) centroid2 += Eigen::Vector2d(p.x(), p.y());
    centroid1 /= points1.size();
    centroid2 /= points2.size();

    // 2. 对齐质心
    QVector<Eigen::Vector2d> alignedPoints1, alignedPoints2;
    for (const QPointF &p : points1) alignedPoints1.push_back(Eigen::Vector2d(p.x() - centroid1.x(), p.y() - centroid1.y()));
    for (const QPointF &p : points2) alignedPoints2.push_back(Eigen::Vector2d(p.x() - centroid2.x(), p.y() - centroid2.y()));


    // 3. 使用最小二乘法求解旋转角度（这里简化处理，使用Eigen的SVD）
    Eigen::MatrixXd H(2, 2);
    H.setZero();
    for (int i = 0; i < alignedPoints1.size(); ++i) {
        H += alignedPoints1[i] * alignedPoints2[i].transpose();
    }
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::Matrix2d U = svd.matrixU();
    Eigen::Matrix2d V = svd.matrixV();
    R = V * U.transpose();
    double angle = atan2(R(1, 0), R(0, 0)); // 弧度制
    double degrees = angle * 180.0 / M_PI; // 转换为角度制
//    qDebug()<<"angle: "<<degrees;
    // 如果需要，可以确保R是旋转矩阵（行列式为1），如果不是则取其转置
    if (R.determinant() < 0) R = R * Eigen::Matrix2d::Identity() * -1;

    // Step 6: 计算平移量。
    Eigen::Vector2d translation = centroid2 - (R * centroid1.cast<double>());
    for (int i = 0; i < T.cols(); ++i)
    {
        T(0,i) = translation(0,0);
        T(1,i) = translation(1,0);
    }
//    qDebug()<<"T: "<<T(0,0)<<"  "<<T(1,0);
//    // 验证
//    Eigen::MatrixXd P1(2, points1.size());
//    for (int i = 0; i < points1.size(); ++i)
//    {
//        P1(0,i) = points1[i].x(); // 将x坐标存储到第一列
//        P1(1,i) = points1[i].y(); // 将y坐标存储到第二列
//    }
//    Eigen::MatrixXd mx = R * P1 + T;
}

QVector<QPointF> ICCP::computeRotationMatrix_2(QVector<QPointF> points1,QVector<QPointF> points2)
{
    Eigen::Vector2d centroid1 = Eigen::Vector2d::Zero(), centroid2 = Eigen::Vector2d::Zero();
    for (const QPointF &p : points1) centroid1 += Eigen::Vector2d(p.x(), p.y());
    for (const QPointF &p : points2) centroid2 += Eigen::Vector2d(p.x(), p.y());
    centroid1 /= points1.size();
    centroid2 /= points2.size();

    double dx1 =  centroid2(0,0) - centroid1(0,0);
    double dy1 =  centroid2(1,0) - centroid1(1,0);
    // 计算旋转角度
    int num = points1.size();
    double points1_dy = points1[num-1].y()-points1[0].y();
    double points1_dx = points1[num-1].x()-points1[0].x();
    double points2_dy = points2[num-1].y()-points2[0].y();
    double points2_dx = points2[num-1].x()-points2[0].x();

    // xoy方位角计算 判断
    double a1 = azimuth(points1_dx, points1_dy);
    // XOY_C方位角计算 判断
    double a2 = azimuth(points2_dx, points2_dy);

    double a =  a2 - a1; // 旋转角度
    double trans_dx = dx1; // x平移
    double trans_dy = dy1; // y平移
    double m = 0;

    QVector<QPointF> temp=points1;
    while(true)
    {
        //定义矩阵

        Eigen::MatrixXd B(num*2, 4);
        Eigen::MatrixXd l(num*2, 1);
        //矩阵初始化
        for (int i = 0; i < num; i++)
        {
            B(i*2, 0) = 1;
            B(i*2, 1) = 0;
            B(i*2, 2) = qCos(a)*temp[i].x()+qSin(a)*temp[i].y();
            B(i*2, 3) = (1+m)*(-qSin(a)*temp[i].x()+qCos(a)*temp[i].y());
            B(i*2+1, 0) = 0;
            B(i*2+1, 1) = 1;
            B(i*2+1, 2) = -qSin(a)*temp[i].x()+qCos(a)*temp[i].y();
            B(i*2+1, 3) = (1+m)*(-qCos(a)*temp[i].x()-qSin(a)*temp[i].y());
            l(i*2, 0) = points2[i].x() - temp[i].x();
            l(i*2+1, 0) = points2[i].y() - temp[i].y();
        }
        //下面进行矩阵计算，并进行内符合指标的计算，此处P为单位矩阵，故略去
        Eigen::MatrixXd BTB = B.transpose()*B;
        Eigen::MatrixXd W = B.transpose()*l;
        Eigen::MatrixXd Para = BTB.inverse()*W;
        if (qAbs(Para(0)) < 0.01 && qAbs(Para(1)) < 0.01 && qAbs(Para(2)) < 0.01 && qAbs(Para(3)) < 0.01)
            break;
        trans_dx = trans_dx + Para(0);
        trans_dy = trans_dy + Para(1);
        m = m + Para(2);
        a = a + Para(3);
        Eigen::MatrixXd V = B * Para - l;
        Eigen::MatrixXd sigma = V.transpose()*V;
//        if (qSqrt(sigma(0,0)/num) < 0.001)
//            break;
//        Eigen::MatrixXd x1 = B * Para + l;
        QVector<QPointF> points;
        points.reserve(num); // 预分配内存以提高效率
        for (int i = 0; i < num; ++i)
        {
            // 假设第一列是x，第二列是y
            double x = trans_dx + (1+m)*(qCos(a)*points1[i].x()+qSin(a)*points1[i].y());
            double y = trans_dy + (1+m)*(-qSin(a)*points1[i].x()+qCos(a)*points1[i].y());
            points.append(QPointF(x, y));
        }
        QVector<QPointF> pNullVector1;
        temp.swap(pNullVector1);
        temp = points;
    }

    QVector<QPointF> points;
    points.reserve(num); // 预分配内存以提高效率
    for (int i = 0; i < num; ++i)
    {
        // 假设第一列是x，第二列是y
        double x = trans_dx + (1+m)*(qCos(a)*points1[i].x()+qSin(a)*points1[i].y());
        double y = trans_dy + (1+m)*(-qSin(a)*points1[i].x()+qCos(a)*points1[i].y());
        points.append(QPointF(x, y));
    }
    return points;
}

QVector<QPointF> ICCP::computeMatrixNew(const QVector<QPointF>& insP,Eigen::Matrix2d &R,Eigen::MatrixXd &T)
{
    Eigen::MatrixXd P1(2, insP.size());
    for (int i = 0; i < insP.size(); ++i)
    {
        P1(0,i) = insP[i].x(); // 将x坐标存储到第一列
        P1(1,i) = insP[i].y(); // 将y坐标存储到第二列
    }
    Eigen::MatrixXd mx = R * P1 + T;
    return eigenMatrixToQVector(mx);
}

double ICCP::calculateDifferences(const QVector<QPointF>& points1, const QVector<QPointF>& points2)
{
    double sum = 0;
    for (int i = 0; i < points1.size(); ++i)
    {
//        qDebug()<<"points1: "<<points1[i].x()<<" "<<points1[i].y()<<"  points2: "<<points2[i].x()<<" "<<points2[i].y();
        sum = (points1[i].x() - points2[i].x()) * (points1[i].x() - points2[i].x())
                + (points1[i].y() - points2[i].y()) * (points1[i].y() - points2[i].y());
    }
    return sum/points1.size();
}

QVector<QPointF> ICCP::iccp(QVector<QVector<double>> data,
                            const QVector<magPoint>& insP,
                            double threshold)
{
    const int n = insP.size();
    const int maxIter = 200;

    // 1) 构建所有等值线，只做一次
    QVector<CountLine> countlines;
    countlines.reserve(n);
    for (const auto &pt : insP) {
        auto bit     = isoCellGrid(data, pt.value);
        auto matrix  = getCellShift(bit);
        CountLine cl;
        getLines(cl, matrix, pt.value, data);
        countlines.push_back(std::move(cl));
    }

    // 2) 把 insP 转到网格坐标系下，作为初始 origin
    QVector<QPointF> origin(n);
    for (int i = 0; i < n; ++i) {
        const auto &magp = insP[i];
        origin[i].setX((magp.point.x() - xmin) / dx);
        origin[i].setY((magp.point.y() - ymin) / dy);
    }

    // 3) 准备循环中要用的容器
    QVector<QPointF> PP(n);
    QVector<QPointF> X(n);

    double prevDiff = std::numeric_limits<double>::infinity();

    // 4) 迭代优化（ICP 主循环）
    for (int iter = 0; iter < maxIter; ++iter) {

        // 4.1) 找到每条等值线上 origin 对应的最近点 PP[i]
        // 如果启用了 OpenMP，可以在这里并行加速
#pragma omp parallel for if(n>100) schedule(static)
        for (int i = 0; i < n; ++i) {
            PP[i] = findNearestPointOnLines(countlines[i], origin[i]);
        }

        // 4.2) 计算新的 X = R*origin + T
        X = computeRotationMatrix_4(origin, PP);

        // 4.3) 计算误差
        double diff = calculateDifferences(origin, X);

        // 4.4) 检查收敛：误差下降不足或已小于阈值
        if (diff < threshold) {
            break;
        }
        prevDiff = diff;

        // 4.5) 用新的 X 作为下一轮的 origin
        origin = X;
    }

    // 5) 将网格坐标恢复到物理坐标
    for (int i = 0; i < n; ++i) {
        X[i].setX(X[i].x() * dx + xmin);
        X[i].setY(X[i].y() * dy + ymin);
    }
    return X;
}

void ICCP::outResult(QVector<QPointF> X)
{
//    for (int i=0;i<X.size();i++)
//    {
//        qDebug()<<X[i].x()<<" "<<X[i].y();
//    }
}

void ICCP::totxt(QVector<QPointF> p,QString file)
{
    QFile dataFile(file);
    if (!dataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 如果文件打开失败，输出错误信息
        qWarning() << "fail" << file;
    }
    QTextStream out(&dataFile);
    // 遍历点集并写入文件
    for (const QPointF &point : p) {
        out << point.x() << "," << point.y() << endl;
    }
    // 关闭文件
    dataFile.close();
}

QVector<QPointF> ICCP::computeRotationMatrix_3(QVector<QPointF> points1,QVector<QPointF> points2)
{
    Eigen::Vector2d centroid1 = Eigen::Vector2d::Zero(), centroid2 = Eigen::Vector2d::Zero();
    for (const QPointF &p : points1) centroid1 += Eigen::Vector2d(p.x(), p.y());
    for (const QPointF &p : points2) centroid2 += Eigen::Vector2d(p.x(), p.y());
    centroid1 /= points1.size();
    centroid2 /= points2.size();

    double dx1 =  centroid2(0,0) - centroid1(0,0);
    double dy1 =  centroid2(1,0) - centroid1(1,0);
    // 计算旋转角度
    int num = points1.size();
    double points1_dy = points1[num-1].y()-points1[0].y();
    double points1_dx = points1[num-1].x()-points1[0].x();
    double points2_dy = points2[num-1].y()-points2[0].y();
    double points2_dx = points2[num-1].x()-points2[0].x();

    // xoy方位角计算 判断
    double a1 = azimuth(points1_dx, points1_dy);
    // XOY_C方位角计算 判断
    double a2 = azimuth(points2_dx, points2_dy);

    double a =  0; // 旋转角度
    double trans_dx = 0; // x平移
    double trans_dy = 0; // y平移

//    QVector<QPointF> temp=points1;
//    while(true)
//    {
        //定义矩阵

        Eigen::MatrixXd B(num*2, 3);
        Eigen::MatrixXd l(num*2, 1);
        //矩阵初始化
        for (int i = 0; i < num; i++)
        {
            B(i*2, 0) = 1;
            B(i*2, 1) = 0;
            B(i*2, 2) = (-qSin(a)*points1[i].x()+qCos(a)*points1[i].y());
            B(i*2+1, 0) = 0;
            B(i*2+1, 1) = 1;
            B(i*2+1, 2) =(-qCos(a)*points1[i].x()-qSin(a)*points1[i].y());
            l(i*2, 0) = points2[i].x() - points1[i].x();
            l(i*2+1, 0) = points2[i].y() - points1[i].y();
        }
        //下面进行矩阵计算，并进行内符合指标的计算，此处P为单位矩阵，故略去
        Eigen::MatrixXd BTB = B.transpose()*B;
        Eigen::MatrixXd W = B.transpose()*l;
        Eigen::MatrixXd Para = BTB.inverse()*W;
//        if (qAbs(Para(0)) < 0.001 && qAbs(Para(1)) < 0.001 && qAbs(Para(2)) < 0.00001)
//            break;
        trans_dx = trans_dx + Para(0);
        trans_dy = trans_dy + Para(1);
        a = a + Para(2);
        Eigen::MatrixXd V = B * Para - l;
        Eigen::MatrixXd sigma = V.transpose()*V;
//        if (qSqrt(sigma(0,0)/num) < 0.001)
//            break;
//        Eigen::MatrixXd x1 = B * Para + l;
//        QVector<QPointF> points;
//        points.reserve(num); // 预分配内存以提高效率
//        for (int i = 0; i < num; ++i)
//        {
//            // 假设第一列是x，第二列是y
//            double x = trans_dx + (qCos(a)*points1[i].x()+qSin(a)*points1[i].y());
//            double y = trans_dy + (-qSin(a)*points1[i].x()+qCos(a)*points1[i].y());
//            points.append(QPointF(x, y));
//        }
//        QVector<QPointF> pNullVector1;
//        temp.swap(pNullVector1);
//        temp = points;
//    }
//    trans_dx = dx1; // x平移
//    trans_dy = dy1; // y平移
    QVector<QPointF> points;
    points.reserve(num); // 预分配内存以提高效率
    for (int i = 0; i < num; ++i)
    {
        // 假设第一列是x，第二列是y
        double x = trans_dx + (qCos(a)*points1[i].x()+qSin(a)*points1[i].y());
        double y = trans_dy + (-qSin(a)*points1[i].x()+qCos(a)*points1[i].y());
        points.append(QPointF(x, y));
    }
    return points;
}

QVector<QPointF> ICCP::computeRotationMatrix_4(QVector<QPointF> points11,QVector<QPointF> points22)
{

    const int n = points11.size();
    if (n == 0 || points22.size() != n)
        return {};

    // 1) 计算两个点集的质心
    double cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0;
    for (int i = 0; i < n; ++i) {
        cx1 += points11[i].x();
        cy1 += points11[i].y();
        cx2 += points22[i].x();
        cy2 += points22[i].y();
    }
    cx1 /= n;  cy1 /= n;
    cx2 /= n;  cy2 /= n;

    // 2) 中心化并累加内积（dot）和叉积（cross）
    double sumDot = 0, sumCross = 0;
    // 我们可以复用 points11 存放中心化后的坐标，避免额外分配
    for (int i = 0; i < n; ++i) {
        double x1 = points11[i].x() - cx1;
        double y1 = points11[i].y() - cy1;
        double x2 = points22[i].x() - cx2;
        double y2 = points22[i].y() - cy2;
        // dot = x1*x2 + y1*y2
        sumDot   += x1 * x2 + y1 * y2;
        // cross = x1*y2 - y1*x2
        sumCross += x1 * y2 - y1 * x2;
        // 覆盖 points11 为中心化后的
        points11[i].setX(x1);
        points11[i].setY(y1);
    }

    // 3) 求最佳旋转角 θ = atan2(sumCross, sumDot)
    double angle = std::atan2(sumCross, sumDot);
    double c = std::cos(angle);
    double s = std::sin(angle);

    // 4) 应用旋转并加回第二个质心，生成输出
    QVector<QPointF> rotated;
    rotated.reserve(n);
    for (int i = 0; i < n; ++i) {
        double x1 = points11[i].x(), y1 = points11[i].y();
        double xr =  c * x1 - s * y1 + cx2;
        double yr =  s * x1 + c * y1 + cy2;
        rotated.push_back(QPointF(xr, yr));
    }

    return rotated;
}

void ICCP::drawResult(QVector<QPointF> X,QVector<QVector<double>> matrix,QVector<QPointF> Real,QVector<magPoint> insP)
{
    int nx = matrix.size();
    if (nx == 0) {
        qWarning() << "drawResult: matrix 行数为 0";
        return;
    }
    int ny = matrix[0].size();
    if (ny == 0) {
        qWarning() << "drawResult: matrix 列数为 0";
        return;
    }

    // 2) 创建 QCustomPlot
    customPlot = new QCustomPlot;
    customPlot->resize(800, 600);

    // 3) 基本样式设置
    customPlot->legend->setVisible(true);
    QFont lf = customPlot->font();
    lf.setPointSize(10);
    customPlot->legend->setFont(lf);
    customPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
    customPlot->xAxis->setLabel("X Axis");
    customPlot->yAxis->setLabel("Y Axis");

    // 4) 用 matrix 绘制热力图
    QCPColorMap* colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    // 设置格网尺寸
    colorMap->data()->setSize(nx, ny);
    // 设置物理范围
    colorMap->data()->setRange(
        QCPRange(xmin, xmax),
        QCPRange(ymin, ymax)
        );
    // 填充每个格点的磁场值
    for (int ix = 0; ix < nx; ++ix) {
        for (int iy = 0; iy < ny; ++iy) {
            colorMap->data()->setCell(ix, iy, matrix[ix][iy]);
        }
    }
    // 调色板
    QCPColorScale* cs = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, cs);
    colorMap->setColorScale(cs);
    // 根据 matrix 中的最小/最大值设置色条范围
    double lo = matrix[0][0], hi = matrix[0][0];
    for (int i = 0; i < nx; ++i)
        for (int j = 0; j < ny; ++j) {
            lo = qMin(lo, matrix[i][j]);
            hi = qMax(hi, matrix[i][j]);
        }
    cs->setDataRange(QCPRange(lo, hi));
    cs->setGradient(QCPColorGradient::gpJet);
    colorMap->rescaleDataRange();

    // 5) 绘制 Real 真实点
    QVector<double> xReal, yReal;
    for (auto &p : Real) { xReal.push_back(p.x()); yReal.push_back(p.y()); }
    customPlot->addGraph();
    customPlot->graph(0)->setName("Real Points");
    customPlot->graph(0)->setData(xReal, yReal);
    customPlot->graph(0)->setPen(QPen(Qt::red));
    customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCross, 4)
        );

    // 6) 绘制 匹配点 X
    QVector<double> xX, yX;
    for (auto &p : X) { xX.push_back(p.x()); yX.push_back(p.y()); }
    customPlot->addGraph();
    customPlot->graph(1)->setName("Matched Points");
    customPlot->graph(1)->setData(xX, yX);
    customPlot->graph(1)->setPen(QPen(Qt::green));
    customPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(1)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssPlus, 4)
        );

    // 7) 绘制 INS 原始点
    QVector<double> xIns, yIns;
    for (auto &mp : insP) { xIns.push_back(mp.point.x()); yIns.push_back(mp.point.y()); }
    customPlot->addGraph();
    customPlot->graph(2)->setName("INS Points");
    customPlot->graph(2)->setData(xIns, yIns);
    customPlot->graph(2)->setPen(QPen(Qt::black));
    customPlot->graph(2)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(2)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssPlus, 4)
        );

    // 8) 缩放并重绘
    customPlot->rescaleAxes();
    customPlot->replot();

}

QVector<QPointF> ICCP::cal(QString data,QString ins,QString real,double thr)
{
    QVector<QVector<double>> matrix = ReadBackground(data);
    QVector<QPointF> REAL = readPointsFromFile(real);
    QVector<magPoint> insP = ReadINS(ins);
    QVector<QPointF> X = iccp(matrix,insP,thr);
    // double sumOfSquares = 0.0;
    //     for (int i = 0; i < REAL.size(); ++i) {
    //         QPointF error = REAL[i] - X[i]; // 计算误差
    //         sumOfSquares += error.x() * error.x() + error.y() * error.y(); // 累加误差的平方
    //     }

    //     double rms = std::sqrt(sumOfSquares / REAL.size()); // 计算RMS

    //     // 将RMS保存到文件
    //     QFile file("navigation_accuracy.txt");
    //     if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //         throw std::runtime_error("Cannot open file for writing.");
    //     }

    //     QTextStream out(&file);
    // out << "RMS Error: " << rms << endl;
    // finalRMS = rms;
    totxt(X,"iccp_out.txt");
    drawResult(X,matrix,REAL,insP);
    outResult(X);
    return X;
}

QVector<QPointF> ICCP::cal(QString data,QString ins,QString tercomResult,QString real,double thr)
{
    QVector<QVector<double>> matrix = ReadBackground(data);
    QVector<QPointF> REAL = readPointsFromFile(real);
    QVector<magPoint> insP = ReadINS(tercomResult);
    QVector<magPoint> insOrigin = ReadINS(ins);
    QVector<QPointF> X = iccp(matrix,insP,thr);
    double sumOfSquares = 0.0;
        for (int i = 0; i < REAL.size(); ++i) {
            QPointF error = REAL[i] - X[i]; // 计算误差
            sumOfSquares += error.x() * error.x() + error.y() * error.y(); // 累加误差的平方
        }

        double rms = std::sqrt(sumOfSquares / REAL.size()); // 计算RMS

        // 将RMS保存到文件
        QFile file("navigation_accuracy.txt");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("Cannot open file for writing.");
        }

        QTextStream out(&file);
    out << "RMS Error: " << rms << endl;
    finalRMS = rms;
    totxt(X,"iccp_out.txt");
    drawResult(X,matrix,REAL,insOrigin);
    outResult(X);
    return X;
}
