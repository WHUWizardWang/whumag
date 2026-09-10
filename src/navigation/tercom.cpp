#include "tercom.h"
namespace Geomagnetic
{
	// Constructor
	TercomMatching::TercomMatching()
	{
	}
	TercomMatching::TercomMatching(const Datapoint& inputData, const Datapoint& inputBase, const Datapoint& inputINS)
	{
		data = inputData;
//		for (const auto& elem : inputData)
//		{
//			TruePath t;
//			t.x = elem.second.X;
//			t.y = elem.second.Y;
//			truePath.push_back(t);
//		}
		for (const auto& elem : inputBase)
		{
			MapData mapData;
			mapData.x = elem.second.X;
			mapData.y = elem.second.Y;
			mapData.magnetic = elem.second.Z;
			base.push_back(mapData);
		}
		for (const auto& elem : inputINS)
		{
			INSData insdata;
			insdata.x = elem.second.X;
			insdata.y = elem.second.Y;
			insdata.heading = elem.second.Z;
			insdata.magnetic = elem.second.tMagnetic;
			insData.push_back(insdata);
		}
	}
    int TercomMatching::ReadBackground(const QString &filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            qWarning()<<"open file failed!";
        if (file.size()==0)
            return -1;
        QTextStream in(&file);
        int index = 0;

        while (!in.atEnd())
        {
            index++;
            QString line = in.readLine().trimmed();
            if (line.isEmpty())
            {
                error_str = error_str + "错误3(空缺行): 第"+QString::number(index)+"行 "+"\n";
                continue;
            }
            QStringList fields = line.split(QRegExp("\\s+|,|;"), QString::SkipEmptyParts);
            if (fields.size() != 3)
            {
                error_str = error_str + "错误1(过少或过多): 第"+QString::number(index)+"行 "+line+"\n";
                continue;
            }
            if (!isNumeric(fields[0])||!isNumeric(fields[1])||!isNumeric(fields[2]))
            {
                error_str = error_str + "错误2(有其他字符): 第"+QString::number(index)+"行 "+line+"\n";
                continue;
            }
            double xx = fields[0].toDouble();
            double yy = fields[1].toDouble();
            double zz = fields[2].toDouble();
            MapData md;
            md.x = xx;
            md.y = yy;
            md.magnetic = zz;
            base.push_back(md);
        }
        file.close();

        cloud.pts.clear();
        for (auto &m : base) {
            cloud.pts.push_back({ m.x, m.y });
        }
        // 创建 KD-Tree，leaf max size 10
        kdtree.reset(new KDTree2D(2, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10)));
        kdtree->buildIndex();
        return 0;
    }

    void TercomMatching::ReadINS(const QString &filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
               qDebug("error");
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
            INSData ins;
            ins.x = x;
            ins.y = y;
            ins.magnetic = value;
            insData.push_back(ins);
//            cout<<x<<y<<value;
        }
        file.close();
    }

    void TercomMatching::ReadTruePath(const QString &filePath)
    {
        truePath.clear();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning()<<"打开真实路径文件失败:"<<filePath;
            return;
        }
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            // 支持逗号或任意空白分隔
            QStringList fields = line.split(QRegExp("[,\\s]+"), QString::SkipEmptyParts);
            if (fields.size() < 2) {
                qWarning()<<"RealPath 格式错误:"<<line;
                continue;
            }
            bool okX, okY;
            double x = fields[0].toDouble(&okX), y = fields[1].toDouble(&okY);
            if (!okX||!okY) {
                qWarning()<<"RealPath 数值转换失败:"<<line;
                continue;
            }
            truePath.push_back({x,y});
        }
        qDebug()<<"Loaded truePath points:"<<truePath.size();
    }

	void TercomMatching::setReferencePoint(const SinglePoint& refPoint)
	{
		referencePoint = refPoint;
	}
    void TercomMatching::saveResult(const QString &filePath,const Datapoint &result)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qWarning() << "fail" << file;
        }
        QTextStream out(&file);
        for (int i = 0;i<result.size();i++)
        {
            out << result.at(i).X <<" " <<result.at(i).Y << " "<< insData.at(i).magnetic<<endl;
        }
        file.close();
    }
	void TercomMatching::printData() const
	{
		for (auto& point : data)
		{
			std::cout << point.second.X << " " << point.second.Y << " " << point.second.Z << std::endl;
		}
	}
	double TercomMatching::calculateDistance(const INSData& insData, const MapData& base) const
	{
		// Calculate the distance between two points
		double dx = insData.x - base.x;
		double dy = insData.y - base.y;
		return sqrt(dx * dx + dy * dy);
	}

    double TercomMatching::IDW(const INSData& insData, size_t K) const
    {
        // 1) Build query point & squared radius
        double queryPt[2] = { insData.x, insData.y };
        double r          = insData.sigma * 3;
        double r2         = r * r;

        // 2) Radius search into a vector of ResultItem
        std::vector<nanoflann::ResultItem<size_t,double>> matches;
        matches.reserve(K);
        nanoflann::SearchParameters params;  // <— use SearchParameters, not SearchParams
        kdtree->radiusSearch(
            /*query*/    &queryPt[0],
            /*radius²*/  r2,
            /*output*/   matches,
            /*options*/  params
            );

        // 3) If empty, fall back to 1-NN
        if (matches.empty()) {
            std::vector<size_t> idx(1);
            std::vector<double> dist2(1);
            nanoflann::KNNResultSet<double> knnRS(1);
            knnRS.init(idx.data(), dist2.data());
            kdtree->findNeighbors(knnRS, &queryPt[0], params);
            return base[idx[0]].magnetic;
        }

        // 4) Keep only the K closest
        if (matches.size() > K) {
            std::nth_element(
                matches.begin(), matches.begin()+K, matches.end(),
                [](auto &a, auto &b){ return a.second < b.second; }
                );
            matches.resize(K);
        }

        // 5) Inverse-distance weighted interpolation
        double sumW = 0, sumM = 0;
        for (auto &it : matches) {
            double d = std::sqrt(it.second);
            if (d < 1e-6)
                return base[it.first].magnetic;
            double w = 1.0/d;
            sumW += w;
            sumM += base[it.first].magnetic * w;
        }
        return sumW>0 ? sumM/sumW : 0.0;
    }

	SinglePoint TercomMatching::rotatePoint(const SinglePoint& point, double angle, const SinglePoint& center) const
	{
		// Rotate a point around a center by a given angle
		double s = sin(angle);
		double c = cos(angle);

		// Translate point back to origin
		double x = point.X - center.X;
		double y = point.Y - center.Y;

		// Rotate point
		double xNew = x * c - y * s;
		double yNew = x * s + y * c;

		// Translate point back
		SinglePoint rotatedPoint = point;
		rotatedPoint.X = xNew + center.X;
		rotatedPoint.Y = yNew + center.Y;

		return rotatedPoint;
	}

    double TercomMatching::calculateMSD(const std::vector<INSData> track, const std::vector<INSData> insdata) const
	{
		// Calculate the mean square deviation between two tracks
		double sum = 0;
		for (int i = 0; i < track.size(); i++)
		{
            double dt = track[i].magnetic - insdata[i].magnetic;
            sum += dt * dt;
		}
		return sum / track.size();
	}

    Datapoint TercomMatching::matchWithAdaptiveRotation()
    {
        // ————— 准备工作 —————
        const double stepSize     = 0.5 * M_PI / 180.0;
        const double maxAngle     = 10  * M_PI / 180.0;
        const double fineStepSize = stepSize / 10.0;
        const double sigma3       = insData[0].sigma * 3.0;
        const int    maxIter      = 50;  // 定义最大迭代次数

        std::vector<INSData> bestTrack;
        double minMSD = std::numeric_limits<double>::infinity();

        // ————— 核心匹配循环 —————
        for (const auto& m : base) {
            // 1) 只处理在 sigma3 范围内的起点
            if (calculateDistance(insData[0], m) > sigma3) continue;

            // 2) 平移 INS 轨迹并做一次 IDW 插值
            std::vector<INSData> alterTrack;
            alterTrack.reserve(insData.size());
            double dx = insData[0].x - m.x;
            double dy = insData[0].y - m.y;
            for (const auto& ins : insData) {
                INSData tmp = ins;
                tmp.x -= dx;
                tmp.y -= dy;
                tmp.magnetic = IDW(tmp);
                alterTrack.push_back(tmp);
            }

            // 3) 粗搜索初始角度
            double bestAngle = 0.0;
            for (double ang = -maxAngle; ang <= maxAngle; ang += stepSize) {
                auto rotTrack = rotateTrack(alterTrack, ang, referencePoint);
                auto cand     = tercomMatch(rotTrack);
                double msd    = calculateMSD(cand, insData);
                if (msd < minMSD) {
                    minMSD    = msd;
                    bestTrack = std::move(cand);
                    bestAngle = ang;
                }
            }

            // 4) 细化搜索
            for (int iter = 0; iter < maxIter; ++iter) {
                bool improved = false;
                for (double ang = bestAngle - stepSize; ang <= bestAngle + stepSize; ang += fineStepSize) {
                    auto rotTrack = rotateTrack(alterTrack, ang, referencePoint);
                    auto cand     = tercomMatch(rotTrack);
                    double msd    = calculateMSD(cand, insData);
                    if (msd < minMSD) {
                        minMSD    = msd;
                        bestTrack = std::move(cand);
                        bestAngle = ang;
                        improved  = true;
                    }
                }
                if (!improved) break;  // 如果没有任何角度能改进，就提前退出
            }
        }

        // ————— 填充并返回 Datapoint（map） —————
        Datapoint result;
        result.clear();
        for (size_t i = 0; i < bestTrack.size(); ++i) {
            SinglePoint p;
            p.X = bestTrack[i].x;
            p.Y = bestTrack[i].y;
            p.tMagnetic = bestTrack[i].magnetic;
            result[static_cast<int>(i)] = p;  // map 的 operator[] 会插入或更新
        }
        return result;
    }

        std::vector<INSData> TercomMatching::rotateTrack(const std::vector<INSData> &track, double angle, const SinglePoint &center) const
    {
        std::vector<INSData> result;
        for (const auto &point : track) {
            INSData rotatedPoint = point;
            SinglePoint sp;
            sp.X = point.x;
            sp.Y = point.y;
            auto rotatedSP = rotatePoint(sp, angle, center);
            rotatedPoint.x = rotatedSP.X;
            rotatedPoint.y = rotatedSP.Y;
            result.push_back(rotatedPoint);
        }
        return result;
    }

        std::vector<INSData> TercomMatching::tercomMatch(const std::vector<INSData> &rotatedTrack) const
    {
        std::vector<INSData> matchTrack;
        for (const auto& elem : rotatedTrack) {
            INSData match = elem;
            match.magnetic = IDW(elem, 10);
            matchTrack.push_back(match);
        }
        return matchTrack;
    }

	Datapoint TercomMatching::match()
	{
        Datapoint result;
        result.clear();

        // 阈值：3σ
        const double sigma3 = insData[0].sigma * 3.0;

        // 1) 构造所有平移并做 IDW 的轨迹集合
        std::vector<std::vector<INSData>> alterTracks;
        alterTracks.reserve(base.size());
        for (const auto& m : base) {
            // 只处理起点在 3σ 范围内的基准点
            if (calculateDistance(insData[0], m) >= sigma3)
                continue;

            // 平移 INS 轨迹并做一次 IDW
            std::vector<INSData> alterTrack;
            alterTrack.reserve(insData.size());
            double dx = insData[0].x - m.x;
            double dy = insData[0].y - m.y;
            for (const auto& ins : insData) {
                INSData tmp = ins;
                tmp.x -= dx;
                tmp.y -= dy;
                // 调用新 IDW 接口，不再传 base
                tmp.magnetic = IDW(tmp);
                alterTrack.push_back(tmp);
            }
            alterTracks.push_back(std::move(alterTrack));
        }

        // 2) 在所有 alterTracks 中选出 MSD 最小的那条
        double minMSD = std::numeric_limits<double>::infinity();
        std::vector<INSData> bestTrack;
        for (auto& track : alterTracks) {
            double msd = calculateMSD(track, insData);
            if (msd < minMSD) {
                minMSD    = msd;
                bestTrack = std::move(track);
            }
        }

        // 3) 把 bestTrack 转成 Datapoint（map），键从 0,1,2,… 开始
        for (size_t i = 0; i < bestTrack.size(); ++i) {
            SinglePoint p;
            p.X = bestTrack[i].x;
            p.Y = bestTrack[i].y;
            p.Z = bestTrack[i].magnetic;
            result[static_cast<int>(i)] = p;
        }

        return result;
	}

	void TercomMatching::getCentroid(const Datapoint& data, double& xg, double& yg)
	{
		double xSum = 0;
		double ySum = 0;
		for (const auto& point : data)
		{
			xSum += point.second.X;
			ySum += point.second.Y;
		}
		xg = xSum / data.size();
		yg = ySum / data.size();
	}
	


    void TercomMatching::drawResult(Datapoint matchResult)
    {
        // 创建QCustomPlot对象
            customPlot = new QCustomPlot;
            // 设置窗口大小
            customPlot->resize(800, 600);

            // 设置图例
            customPlot->legend->setVisible(true);
            QFont legendFont = customPlot->font();
            legendFont.setPointSize(10);
            customPlot->legend->setFont(legendFont);
            customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));
            customPlot->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignLeft|Qt::AlignTop);

            // 设置轴标签
            customPlot->xAxis->setLabel("X Axis");
            customPlot->yAxis->setLabel("Y Axis");

            // 找出背景数据的范围
            QVector<double> tm,x,y;
            double minX = base[0].x;
            double maxX = base[0].x;
            double minY = base[0].y;
            double maxY = base[0].y;
            for(auto tmp = base.begin(); tmp != base.end(); tmp++)
            {
                if (tmp->x<minX)
                    minX = tmp->x;
                if (tmp->x>maxX)
                    maxX = tmp->x;
                if (tmp->y<minY)
                    minY = tmp->y;
                if (tmp->x>maxY)
                    maxY = tmp->y;
                tm.push_back(tmp->magnetic);
                x.push_back(tmp->x);
                y.push_back(tmp->y);
            }
            // 创建热力图数据结构
            int nx = maxX/x_step; // x方向上的点数
            int ny = maxY/y_step; // y方向上的点数
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
            for (auto &p : truePath) { xReal<<p.x; yReal<<p.y; }
            customPlot->addGraph();
            customPlot->graph(0)->setName("Real Points");
            customPlot->graph(0)->setPen(QPen(Qt::red)); // 设置红色
            customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
            customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 4));
            customPlot->graph(0)->setData(xReal, yReal);

            // 绘制X数据点
            QVector<double> xX, yX;
            for (const auto& pair : matchResult) {
                xX.append(pair.second.X);
                yX.append(pair.second.Y);
            }
            customPlot->addGraph();
            customPlot->graph(1)->setName("matched Points");
            customPlot->graph(1)->setPen(QPen(Qt::blue)); // 设置绿色
            customPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
            customPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 4));
            customPlot->graph(1)->setData(xX, yX);

            //        // 绘制ins数据点
            QVector<double> xins, yins;
            for(const auto& point : insData) {
                xins.append(point.x);
                yins.append(point.y);
            }
            customPlot->addGraph();
            customPlot->graph(2)->setName("Original INS Points");
            customPlot->graph(2)->setPen(QPen(Qt::black));
            customPlot->graph(2)->setLineStyle(QCPGraph::lsNone);
            customPlot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 4));
            customPlot->graph(2)->setData(xins, yins);

            // 自动缩放为显示所有内容
            for (int i = 0; i < customPlot->graphCount(); ++i)
                customPlot->graph(i)->rescaleAxes(true);
            customPlot->replot();

            // 显示窗口
    //        customPlot->show();
    //        return customPlot;

    }

}
