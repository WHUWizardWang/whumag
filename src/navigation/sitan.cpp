#include "sitan.h"
namespace Geomagnetic
{
using namespace Geomagnetic;
    KalmanFilter::KalmanFilter(const QVector<QVector<double>>& b)
    {
        x = VectorXd::Zero(2);
        P = MatrixXd::Identity(2, 2);
        F = MatrixXd::Identity(2, 2);
        H = RowVector2d();
        R = MatrixXd::Identity(2, 2);
        R(0, 0) = 1e-3; // 观测x的噪声
        R(1, 1) = 1e-3; // 观测y的噪声
        Q = MatrixXd::Identity(2, 2);
        background = b ;
    }
    void KalmanFilter::init(const VectorXd &initialState)
    {
        int n = initialState.size(); // 状态的维度，例如：[x, y]
        int m = 1; // 观测的维度
        x = initialState;
        P = MatrixXd::Identity(n, n) * 1;
        F = MatrixXd::Identity(n, n);
        R = MatrixXd::Identity(m, m) * 0.1 * 0.1; // 假设观测噪声的标准差为 0.1
        Q = MatrixXd::Identity(n, n) * 0.01 * 0.01; // 假设过程噪声的标准差为 0.01
    }
    void KalmanFilter::predict(const VectorXd &controlInput)
    {
        x = controlInput;
        x = F * x;
        P = F * P * F.transpose() + Q;

    }
    VectorXd KalmanFilter::update(const double measurement,const VectorXd &controlInput)
    {
        VectorXd z = VectorXd::Zero(1);
        z(0) = measurement;
        double magneticAnomalyX;
        double magneticAnomalyY;
        double c;
        getGeomagneticIntensity(x,background,magneticAnomalyX,magneticAnomalyY,c);
        H << magneticAnomalyX , magneticAnomalyY;
        Eigen::VectorXd c_vec = Eigen::VectorXd::Constant(z.rows(), c);
        VectorXd y = z - H * (x - controlInput) - c_vec; // 观测残差
        MatrixXd S = H * P * H.transpose() + R; // 残差协方差
        MatrixXd K = P * H.transpose() * S.inverse(); // 卡尔曼增益
//        //损失函数
        double delta = 4.0;
//        VectorXd robust_y = VectorXd::Zero(y.size());
//                for (int i = 0; i < y.size(); ++i) {
//                    if (std::abs(y(i)) <= delta) {
//                        robust_y(i) = y(i);
//                    } else {
//                        robust_y(i) = delta * ((y(i) > 0 ? 1 : -1));
//                    }
//                }
        x = x + K * y; // 更新状态
        P = (MatrixXd::Identity(P.rows(), P.cols()) - K * H) * P; // 更新协方差
        return x;
    }

    void KalmanFilter::getGeomagneticIntensity(const Vector2d& currentCoordinates, const QVector<QVector<double>>& backgroundField,
                                               double &a,double &b,double& c)
    {
        double gridSpacing = 0.5;

            int cols = backgroundField.size();
            int rows = backgroundField[0].size();

            // 计算当前坐标在格网中的相对位置
            double normalizedX = (currentCoordinates(0) - 0.0) / gridSpacing;
            double normalizedY = (currentCoordinates(1) - 0.0) / gridSpacing;

            // 计算中心索引
            int xCenter = static_cast<int>(std::round(normalizedX));
            int yCenter = static_cast<int>(std::round(normalizedY));
            double sigma_x=0.5;
            double sigma_y=0.5;
            // 确定拟合区域大小（5σx * 5σy）
            int regionSizeY = static_cast<int>(std::round(5 * sigma_y / gridSpacing));
            int regionSizeX = static_cast<int>(std::round(5 * sigma_x / gridSpacing));

            // 构建观测矩阵
            std::vector<Eigen::Vector3d> B_list;
            std::vector<double> M_list;

            for (int i = -regionSizeY / 2; i <= regionSizeY / 2; ++i)
            {
                for (int j = -regionSizeX / 2; j <= regionSizeX / 2; ++j)
                {
                    int XIdx = std::clamp(xCenter + i, 0, cols - 1);
                    int YIdx = std::clamp(yCenter + j, 0, rows - 1);

                    double h_val = backgroundField[XIdx][YIdx];

                    M_list.push_back(h_val);
                    B_list.push_back(Eigen::Vector3d(1, YIdx - yCenter, XIdx - xCenter));
                }
            }

            // 将 vector 转换为 Eigen Matrix 和 Vector
            int n = B_list.size();
            Eigen::MatrixXd B(n, 3);
            Eigen::VectorXd M(n);
            for (int i = 0; i < n; ++i)
            {
                B.row(i) = B_list[i].transpose();
                M(i) = M_list[i];
            }

            // 使用最小二乘法求解 L = (B^T * B)^-1 * B^T * M
            Eigen::VectorXd L = (B.transpose() * B).inverse() * B.transpose() * M;

            // 提取拟合参数
            c = L(0);
            a = L(1) / gridSpacing;
            b = L(2) / gridSpacing;
    }

    SitanMatching::SitanMatching()
    : kf(), background()
    {

    };

    QVector<QVector<double>> SitanMatching::ReadBackground(const QString &filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            qWarning()<<"open file failed!";
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if (line.isEmpty())
                continue;
            QStringList fields = line.split(QRegExp("\\s+|,"), QString::SkipEmptyParts);
            if (fields.size() != 3)
                continue;
            bool okX, okY, okZ;
            double xx = fields[0].toDouble(&okX);
            double yy = fields[1].toDouble(&okY);
            double zz = fields[2].toDouble(&okZ);
            x_bg.append(xx);
            y_bg.append(yy);
            z_bg.append(zz);
        }
        double dx = 0.5;
        double dy = 0.5;
        double xmax = std::numeric_limits<double>::lowest(); // 初始化为最小值
        double xmin = std::numeric_limits<double>::max();    // 初始化为最大值
        double ymax = std::numeric_limits<double>::lowest(); // 初始化为最小值
        double ymin = std::numeric_limits<double>::max();    // 初始化为最大值
        // 计算 x_bg 的最大值和最小值
        if (!x_bg.isEmpty()) {
            xmax = x_bg[0]; // 使用第一个元素初始化
            xmin = x_bg[0]; // 使用第一个元素初始化
            for (auto it = x_bg.constBegin(); it != x_bg.constEnd(); ++it) {
                xmax = qMax(xmax, *it);
                xmin = qMin(xmin, *it);
            }
        }

        // 计算 y_bg 的最大值和最小值
        if (!y_bg.isEmpty()) {
            ymax = y_bg[0]; // 使用第一个元素初始化
            ymin = y_bg[0]; // 使用第一个元素初始化
            for (auto it = y_bg.constBegin(); it != y_bg.constEnd(); ++it) {
                ymax = qMax(ymax, *it);
                ymin = qMin(ymin, *it);
            }
        }
        if (x_bg.isEmpty() || y_bg.isEmpty()) {
            qWarning() << "ReadBackground: no valid data points parsed from" << filePath << "- returning empty grid";
            file.close();
            return QVector<QVector<double>>();
        }
        int xSize = static_cast<int>((xmax - xmin) / dx) + 1;
        int ySize = static_cast<int>((ymax - ymin) / dy) + 1;
        QVector<QVector<double>> data(xSize, QVector<double>(ySize, 0.0));
        for (int i = 0;i<x_bg.size();i++)
        {
            int ix = round((x_bg[i] - xmin) / dx);
            int iy = round((y_bg[i] - ymin) / dy);
            ix = qBound(0, ix, xSize - 1);
            iy = qBound(0, iy, ySize - 1);
            data[ix][iy] = z_bg[i];
        }
        file.close();

        return data;
    }
    QVector<INSData> SitanMatching::ReadINS(const QString &filePath)
    {
        QVector<INSData> points;
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
            INSData p;
            p.x = x;
            p.y = y;
            p.magnetic = value;
            points.push_back(p);
        }
        file.close();
        return points;
    }
    void  SitanMatching::drawResult(const QVector<QPointF>& X,
                                    const QVector<QVector<double>>& matrix,
                                    const QVector<QPointF>& Real,
                                    const QString &back,
                                    const QVector<INSData>& insdata)
    {
        // 创建QCustomPlot对象
            customPlot = new QCustomPlot;
            QVector<double> x,y,tm;
            //
            QFile file(back);
            if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return;
            QTextStream in(&file);
            QString line;
            while (!in.atEnd())
            {
                QString line = in.readLine().trimmed();
                if (line.isEmpty())
                    continue;
                QStringList fields = line.split(QRegExp("\\s+|,"), QString::SkipEmptyParts);
                if (fields.size() != 3)
                    continue;
                bool okX, okY, okZ;
                double xx = fields[0].toDouble(&okX);
                double yy = fields[1].toDouble(&okY);
                double zz = fields[2].toDouble(&okZ);
                x.push_back(xx);
                y.push_back(yy);
                tm.push_back(zz);
            }
            // 设置窗口大小
            customPlot->resize(800, 600);

            // 设置图例
            customPlot->legend->setVisible(true);
            QFont legendFont = customPlot->font();
            legendFont.setPointSize(10);
            customPlot->legend->setFont(legendFont);
            customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));

            // 设置轴标签
            customPlot->xAxis->setLabel("X Axis");
            customPlot->yAxis->setLabel("Y Axis");

            // 找出背景数据的范围
            double minX = *(std::min_element(x.begin(),x.end()));
            double maxX = *(std::max_element(x.begin(),x.end()));
            double minY = *(std::min_element(y.begin(),y.end()));
            double maxY = *(std::max_element(y.begin(),y.end()));
            // 创建热力图数据结构
            int nx = maxX/0.5; // x方向上的点数
            int ny = maxY/0.5; // y方向上的点数
            QCPColorMap* colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
            colorMap->data()->setSize(nx, ny); // 设置数据大小
            colorMap->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY)); // 设置数据范围

            for (int X = 0; X < nx; ++X)
               {
                   for (int Y = 0; Y < ny; ++Y)
                   {
                       double lon = minX + X * (maxX - minX) / (nx - 1);
                       double lat = minY + Y * (maxY - minY) / (ny - 1);

                       // 简单的最近邻插值
                       double tMagnetic = 0;
                       double minDist = std::numeric_limits<double>::max();
                       for (int i = 0; i < x.size(); ++i)
                       {
                           double dist = std::sqrt(std::pow(x[i] - lon, 2) + std::pow(y[i] - lat, 2));
                           if (dist < minDist)
                           {
                               minDist = dist;
                               tMagnetic = tm[i];
                           }
                       }
                       colorMap->data()->setCell(X, Y, tMagnetic);
                   }
               }

            // 添加颜色条
            QCPColorScale *colorScale = new QCPColorScale(customPlot);
            customPlot->plotLayout()->addElement(0, 1, colorScale);
            colorMap->setColorScale(colorScale);
            colorScale->setDataRange(QCPRange(*std::min_element(tm.constBegin(), tm.constEnd()), *std::max_element(tm.constBegin(), tm.constEnd())));
            colorScale->setGradient(QCPColorGradient::gpJet);
            // 添加等值线
            colorMap->rescaleDataRange();

            // 绘制REAL数据点
            QVector<double> xReal, yReal;
            for(const auto& point : Real) {
                xReal.append(point.x());
                yReal.append(point.y());
            }
            customPlot->addGraph();
            customPlot->graph(0)->setName("Real Points");
            customPlot->graph(0)->setPen(QPen(Qt::red)); // 设置红色
            customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
            customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 4));
            customPlot->graph(0)->setData(xReal, yReal);

            // 绘制X数据点
            QVector<double> xX, yX;
            for(const auto& point : X) {
                xX.append(point.x());
                yX.append(point.y());
            }
            customPlot->addGraph();
            customPlot->graph(1)->setName("matched Points");
            customPlot->graph(1)->setPen(QPen(Qt::black)); // 设置绿色
            customPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
            customPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 4));
            customPlot->graph(1)->setData(xX, yX);

            // 绘制ins数据点
            QVector<double> xINS, yINS;
            for(const auto& point : insdata) {
                xINS.append(point.x);
                yINS.append(point.y);
            }
            customPlot->addGraph();
            customPlot->graph(2)->setName("INS points");
            customPlot->graph(2)->setPen(QPen(Qt::blue)); // 设置绿色
            customPlot->graph(2)->setLineStyle(QCPGraph::lsNone);
            customPlot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 4));
            customPlot->graph(2)->setData(xINS, yINS);

            // 自动缩放为显示所有内容
            customPlot->rescaleAxes();
            customPlot->replot();

            // 显示窗口
//            customPlot->show();

    }

    void SitanMatching::totxt(const QVector<QPointF>& p, const QString& file)
    {
        QFile dataFile(file);
        if (!dataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 如果文件打开失败，输出错误信息
            qWarning() << "fail" << file;
        }
        QTextStream out(&dataFile);
        // 遍历点集并写入文件
        for (const QPointF &point : p) {
            out << point.x() << "," << point.y() << Qt::endl;
        }
        // 关闭文件
        dataFile.close();
    }

    QVector<QPointF> SitanMatching::SITANAlgorithm(const QVector<QVector<double>>& background,const QVector<INSData>& insdata)
    {
        QVector<QPointF> X;
        kf = KalmanFilter(background);
        VectorXd init(2);
        init[0] = insdata[0].x;
        init[1] = insdata[0].y;
        kf.init(init);
        std::vector<VectorXd> coordinate;
        for(int i =0 ;i<insdata.size()-1;i++)
        {
            VectorXd current(2);
            VectorXd next(2);
            VectorXd coord;
            current[0] = insdata[i].x;
            current[1] = insdata[i].y;
            kf.predict(current);
            next[0] = insdata[i+1].x;
            next[1] = insdata[i+1].y;
            coord = kf.update(insdata[i].magnetic,next);
            coordinate.push_back(coord);
        }
        for(const auto& vec:coordinate)
        {
            QPointF point(vec(0),vec(1));
            X.push_back(point);
        }
        totxt(X,"sitan_out.txt");
        return X;
    }
}

