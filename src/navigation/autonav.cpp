#include "autonav.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <limits>
#include <algorithm>

namespace Geomagnetic {

//---------------------------
// 1. 数据读取接口
//---------------------------

void AUTONAV::readBackgroundFile(const QString &filePath) {
    base.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "readBackgroundFile: 无法打开文件：" << filePath;
        return;
    }
    QTextStream in(&file);
    int lineNo = 0;
    while (!in.atEnd()) {
        ++lineNo;
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList fld = line.split(QRegExp("\\s+|,|;"), Qt::SkipEmptyParts);
        if (fld.size() != 3) {
            qWarning() << QString("背景场：第%1行格式错误 (期望3列)").arg(lineNo);
            continue;
        }
        bool okX, okY, okZ;
        double xx = fld[0].toDouble(&okX);
        double yy = fld[1].toDouble(&okY);
        double zz = fld[2].toDouble(&okZ);
        if (!okX || !okY || !okZ) {
            qWarning() << QString("背景场：第%1行数值转换失败").arg(lineNo);
            continue;
        }
        MapData md;
        md.x = xx;
        md.y = yy;
        md.magnetic = zz;
        base.push_back(md);
    }
    file.close();
}

void AUTONAV::readINSFile(const QString &filePath) {
    insData.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "readINSFile: 无法打开文件：" << filePath;
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList fld = line.split(QRegExp("[,\\s]+"), Qt::SkipEmptyParts);
        // 这里只关心前三列：x, y, 磁测量值（heading 可忽略或放到结构里，但后面算法大多只用 x,y,magnetic）
        if (fld.size() < 3) continue;
        bool okX, okY, okM;
        double xx = fld[0].toDouble(&okX);
        double yy = fld[1].toDouble(&okY);
        double mm = fld[2].toDouble(&okM);
        if (!okX || !okY || !okM) continue;
        INSData ins;
        ins.x = xx;
        ins.y = yy;
        ins.magnetic = mm;
        // 如果需要的话，可以把 heading 放到 fld[2] 之后
        insData.push_back(ins);
    }
    file.close();
}

void AUTONAV::readTruePathFile(const QString &filePath) {
    truePath.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "readTruePathFile: 无法打开文件：" << filePath;
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList fld = line.split(QRegExp("[,\\s]+"), Qt::SkipEmptyParts);
        if (fld.size() < 2) continue;
        bool okX, okY;
        double xx = fld[0].toDouble(&okX);
        double yy = fld[1].toDouble(&okY);
        if (!okX || !okY) continue;
        TruePath tp;
        tp.x = xx;
        tp.y = yy;
        truePath.push_back(tp);
    }
    file.close();
}

//---------------------------
// 2. 数据格式转换接口
//---------------------------

QVector<QVector<double>> AUTONAV::convertBaseToGrid() const {
    QVector<QVector<double>> grid;
    if (base.empty()) return grid;

    // 假设网格间隔均为 0.5 (与 SITAN/ICCP 中保持一致)
    const double dx = 0.5, dy = 0.5;

    // 先求 xmin, xmax, ymin, ymax
    double xmin = base[0].x, xmax = base[0].x;
    double ymin = base[0].y, ymax = base[0].y;
    for (const auto &md : base) {
        xmin = std::min(xmin, md.x);
        xmax = std::max(xmax, md.x);
        ymin = std::min(ymin, md.y);
        ymax = std::max(ymax, md.y);
    }
    int nx = static_cast<int>(std::round((xmax - xmin) / dx)) + 1;
    int ny = static_cast<int>(std::round((ymax - ymin) / dy)) + 1;
    if (nx <= 0 || ny <= 0) return grid;

    grid.resize(nx);
    for (int i = 0; i < nx; ++i) {
        grid[i].resize(ny);
        std::fill(grid[i].begin(), grid[i].end(), 0.0);
    }

    // 填充：最近邻（四舍五入到最近网格中心）
    for (const auto &md : base) {
        int ix = qRound((md.x - xmin) / dx);
        int iy = qRound((md.y - ymin) / dy);
        if (ix < 0) ix = 0; if (ix >= nx) ix = nx - 1;
        if (iy < 0) iy = 0; if (iy >= ny) iy = ny - 1;
        grid[ix][iy] = md.magnetic;
    }
    return grid;
}

QVector<magPoint> AUTONAV::convertInsToMagPoints() const {
    QVector<magPoint> result;
    result.reserve(static_cast<int>(insData.size()));
    for (const auto &d : insData) {
        magPoint mp;
        mp.point = QPointF(d.x, d.y);
        mp.value = d.magnetic;
        result.push_back(mp);
    }
    return result;
}

QVector<magPoint> AUTONAV::convertDatapointToMagPoints(const Datapoint &dp) const {
    QVector<magPoint> result;
    result.reserve(dp.size());
    for (auto it = dp.begin(); it != dp.end(); ++it) {
        // it->first 是索引， it->second 是 SinglePoint
        const SinglePoint &sp = it->second;
        magPoint mp;
        mp.point = QPointF(sp.X, sp.Y);
        mp.value = sp.tMagnetic;  // TERCOM 中存储在 tMagnetic
        result.push_back(mp);
    }
    return result;
}

//---------------------------
// 3. 一次性运行所有算法
//---------------------------

void AUTONAV::runAll(const QString &backgroundFile,
                     const QString &insFile,
                     const QString &truePathFile)
{
    // 1) 读取三种原始数据并获得背景场xy范围
    readBackgroundFile(backgroundFile);
    readINSFile(insFile);
    readTruePathFile(truePathFile);

    if (base.empty() || insData.empty()) {
        qWarning() << "runAll: base 或 insData 为空，无法继续匹配";
        return;
    }
    if (truePath.empty()) {
        qWarning() << "runAll: truePath 为空，无法绘制真实轨迹";
    }

    // 获取背景场 xy 的最大最小值
    double xmin = std::numeric_limits<double>::max();
    double xmax = std::numeric_limits<double>::lowest();
    double ymin = std::numeric_limits<double>::max();
    double ymax = std::numeric_limits<double>::lowest();
    for (const auto &md : base) {
        xmin = std::min(xmin, md.x);
        xmax = std::max(xmax, md.x);
        ymin = std::min(ymin, md.y);
        ymax = std::max(ymax, md.y);
    }

    // 2) 将 base 转为网格矩阵
    QVector<QVector<double>> grid = convertBaseToGrid();
    if (grid.isEmpty()) {
        qWarning() << "runAll: 网格矩阵为空";
        return;
    }


    // ── 步骤 A：先执行 TERCOM ──
    {
        TercomMatching tm_local;
        tm_local.ReadBackground(backgroundFile);
        tm_local.ReadINS(insFile);
        tm_local.ReadTruePath(truePathFile);
        Datapoint tercomDp = tm_local.matchWithAdaptiveRotation();

        // 如果 tercomDp 为空，也直接返回
        if (tercomDp.empty()) {
            qWarning() << "runAll: TERCOM 返回空轨迹，无法继续后续算法";
            return;
        }

        // 2.A.1) 将 TERCOM 输出写文件（可选）
        {
            QFile fout(QDir::currentPath() + "/tercom_out_auto.txt");
            if (fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&fout);
                for (auto &pr : tercomDp) {
                    const SinglePoint &sp = pr.second;
                    out << sp.X << "," << sp.Y << "," << sp.tMagnetic << "\n";
                }
                fout.close();
            } else {
                qWarning() << "runAll: 无法打开 tercom_out_auto.txt 写入 TERCOM 结果";
            }
        }

        // 2.A.2) 把 TERCOM 输出转为 QVector<INSData>，供 SITAN 使用
        QVector<INSData> tercomInsData;
        tercomInsData.reserve(static_cast<int>(tercomDp.size()));
        for (auto &pr : tercomDp) {
            const SinglePoint &sp = pr.second;
            INSData id;
            id.x = sp.X;
            id.y = sp.Y;
            // 如果希望让 SITAN 使用 TERCOM 的磁强度作为初始值：
            id.magnetic = sp.tMagnetic;
            tercomInsData.append(id);
        }

        // ── 步骤 B：用 TERCOM 结果跑 SITAN ──
        QVector<QPointF> sitanResult;
        {
            // SITANAlgorithm 的签名是：
            //   QVector<QPointF> SITANAlgorithm(const QVector<QVector<double>>& background,
            //                                   const QVector<INSData>& insdata);
            sitanResult = sm.SITANAlgorithm(grid, tercomInsData);
        }

        // ── 步骤 C：用 ICCP 做匹配 ──
        // 您可以选择：
        //   C.1 用“原始 INS”跑 ICCP
        QVector<QPointF> iccp_origResult;
        {
            QVector<magPoint> origMagPts = convertInsToMagPoints();

            // 设置背景场xy最大最小值

            cp.setMinMax(xmin, xmax, ymin, ymax);
            cp.setDxDy(x_step, y_step);
            iccp_origResult = cp.iccp(grid, origMagPts, 1e-6);

            // 写出 iccp_origResult:
            cp.totxt(iccp_origResult, "iccp_orig_out_auto.txt");
        }

        //   C.2 用“TERCOM 结果”跑 ICCP（如果需要）
        QVector<QPointF> iccp_tercomResult;
        {
            // 首先把 tercomInsData 转为 QVector<magPoint>
            QVector<magPoint> tercomMagPts;
            tercomMagPts.reserve(tercomInsData.size());
            for (const INSData &d : tercomInsData) {
                magPoint mp;
                mp.point = QPointF(d.x, d.y);
                mp.value = d.magnetic;
                tercomMagPts.append(mp);
            }
            iccp_tercomResult = cp.iccp(grid, tercomMagPts, 1e-6);
            // 写出 iccp_tercomResult:
            cp.totxt(iccp_tercomResult, "iccp_tercom_out_auto.txt");
        }

        // ── 步骤 D：最后绘图 ──
        // drawResult 参数顺序： (ICCP 原始 INS) 、 (SITAN) 、 (TERCOM Datapoint) 、 (TERCOM+ICCP)
        drawResult(iccp_origResult, sitanResult, tercomDp, iccp_tercomResult);
    }
}


//---------------------------
// 4. 统一绘制结果（热力图 + 各种路径）
//---------------------------

void AUTONAV::drawResult(const QVector<QPointF> &iccpResult,
                         const QVector<QPointF> &sitanResult,
                         const Datapoint &tercomResult,
                         const QVector<QPointF> &tercomIccpResult)
{
    if (base.empty()) {
        qWarning() << "drawResult: base 为空，无法绘制热力图";
        return;
    }

    // 创建 QCustomPlot
    if (customPlot) {
        delete customPlot;
        customPlot = nullptr;
    }
    customPlot = new QCustomPlot;
    customPlot->resize(900, 650);

    // 1) 从 base 计算热力图范围 & 网格
    QVector<double> xs, ys, mags;
    xs.reserve(base.size());
    ys.reserve(base.size());
    mags.reserve(base.size());

    double minX = base.front().x, maxX = base.front().x;
    double minY = base.front().y, maxY = base.front().y;
    for (const auto &md : base) {
        xs.push_back(md.x);
        ys.push_back(md.y);
        mags.push_back(md.magnetic);
        minX = std::min(minX, md.x);
        maxX = std::max(maxX, md.x);
        minY = std::min(minY, md.y);
        maxY = std::max(maxY, md.y);
    }

    // 假设热力图分辨率同样以 0.5 为步长
    int nx = static_cast<int>(std::round((maxX - minX) / 0.5)) + 1;
    int ny = static_cast<int>(std::round((maxY - minY) / 0.5)) + 1;
    if (nx <= 1 || ny <= 1) {
        qWarning() << "drawResult: 热力图尺寸过小";
    } else {
        // 2) 创建 QCPColorMap 并填充数据（最近邻插值）
        QCPColorMap *colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
        colorMap->data()->setSize(nx, ny);
        colorMap->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY));

        for (int ix = 0; ix < nx; ++ix) {
            for (int iy = 0; iy < ny; ++iy) {
                double lon = minX + ix * (maxX - minX) / (nx - 1);
                double lat = minY + iy * (maxY - minY) / (ny - 1);

                // 简单最近邻
                double nearestMag = 0;
                double bestDist = std::numeric_limits<double>::max();
                for (int k = 0; k < xs.size(); ++k) {
                    double dx = xs[k] - lon;
                    double dy = ys[k] - lat;
                    double dist2 = dx * dx + dy * dy;
                    if (dist2 < bestDist) {
                        bestDist = dist2;
                        nearestMag = mags[k];
                    }
                }
                colorMap->data()->setCell(ix, iy, nearestMag);
            }
        }

        // 3) 添加色条
        QCPColorScale *colorScale = new QCPColorScale(customPlot);
        customPlot->plotLayout()->addElement(0, 1, colorScale);
        colorMap->setColorScale(colorScale);
        double lo = *std::min_element(mags.constBegin(), mags.constEnd());
        double hi = *std::max_element(mags.constBegin(), mags.constEnd());
        colorScale->setDataRange(QCPRange(lo, hi));
        colorScale->setGradient(QCPColorGradient::gpJet);
        colorMap->rescaleDataRange();
    }

    // 4) 设置图例和轴标签
    customPlot->legend->setVisible(true);
    QFont legendFont = customPlot->font();
    legendFont.setPointSize(10);
    customPlot->legend->setFont(legendFont);
    customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft | Qt::AlignTop);
    customPlot->xAxis->setLabel("X (m)");
    customPlot->yAxis->setLabel("Y (m)");

    // 5) 绘制真实轨迹（红色 ×）
    QVector<double> xTrue, yTrue;
    for (const auto &tp : truePath) {
        xTrue.push_back(tp.x);
        yTrue.push_back(tp.y);
    }
    customPlot->addGraph();
    customPlot->graph(0)->setName("True Path");
    customPlot->graph(0)->setData(xTrue, yTrue);
    customPlot->graph(0)->setPen(QPen(Qt::red));
    customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 6));

    // 6) 绘制 INS 原始点（黑色 +）
    QVector<double> xIns, yIns;
    for (const auto &d : insData) {
        xIns.push_back(d.x);
        yIns.push_back(d.y);
    }
    customPlot->addGraph();
    customPlot->graph(1)->setName("INS Raw");
    customPlot->graph(1)->setData(xIns, yIns);
    customPlot->graph(1)->setPen(QPen(Qt::black));
    customPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 6));

    // 7) 绘制 ICCP 初次匹配结果（蓝色 ○）
    QVector<double> xIccp, yIccp;
    for (const auto &pt : iccpResult) {
        xIccp.push_back(pt.x());
        yIccp.push_back(pt.y());
    }
    customPlot->addGraph();
    customPlot->graph(2)->setName("ICCP Result");
    customPlot->graph(2)->setData(xIccp, yIccp);
    customPlot->graph(2)->setPen(QPen(Qt::blue));
    customPlot->graph(2)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 6));

    // 8) 绘制 SITAN 匹配结果（绿色 □）
    QVector<double> xSitan, ySitan;
    for (const auto &pt : sitanResult) {
        xSitan.push_back(pt.x());
        ySitan.push_back(pt.y());
    }
    customPlot->addGraph();
    customPlot->graph(3)->setName("SITAN Result");
    customPlot->graph(3)->setData(xSitan, ySitan);
    customPlot->graph(3)->setPen(QPen(Qt::green));
    customPlot->graph(3)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(3)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 6));

    // 9) 绘制 TERCOM 原始匹配结果（magenta △）
    QVector<double> xTer, yTer;
    for (auto it = tercomResult.begin(); it != tercomResult.end(); ++it) {
        const SinglePoint &sp = it->second;
        xTer.push_back(sp.X);
        yTer.push_back(sp.Y);
    }
    customPlot->addGraph();
    customPlot->graph(4)->setName("TERCOM Result");
    customPlot->graph(4)->setData(xTer, yTer);
    customPlot->graph(4)->setPen(QPen(Qt::magenta));
    customPlot->graph(4)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(4)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssTriangle, 6));

    // 10) 绘制 TERCOM+ICCP 二次匹配结果（橙色 +）
    QVector<double> xTerIccp, yTerIccp;
    for (const auto &pt : tercomIccpResult) {
        xTerIccp.push_back(pt.x());
        yTerIccp.push_back(pt.y());
    }
    customPlot->addGraph();
    customPlot->graph(5)->setName("TERCOM + ICCP");
    customPlot->graph(5)->setData(xTerIccp, yTerIccp);
    customPlot->graph(5)->setPen(QPen(Qt::darkYellow));
    customPlot->graph(5)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(5)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 8));

    // 11) 自动缩放所有图形并重绘
    customPlot->rescaleAxes();
    customPlot->replot();

    // 最后把窗口显示出来，或嵌入到你的 GUI 界面中
    customPlot->show();
}

} // namespace Geomagnetic
