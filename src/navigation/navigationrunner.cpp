#include "navigationrunner.h"

#include "iccp.h"
#include "sitan.h"
#include "tercom.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

#include <exception>
#include <new>

namespace Nav {

QString methodTitle(Method method)
{
    switch (method) {
    case Method::Tercom: return QStringLiteral("TERCOM");
    case Method::Iccp: return QStringLiteral("ICCP");
    case Method::Sitan: return QStringLiteral("SITAN");
    case Method::TercomIccp: return QStringLiteral("TERCOM + ICCP");
    case Method::All: return QStringLiteral("全部算法对比");
    }
    return QString();
}

QString methodDescription(Method method)
{
    switch (method) {
    case Method::Tercom:
        return QStringLiteral("平移并旋转整条航迹，使沿途背景场值与实测值最接近。适合初始误差较大的情况。");
    case Method::Iccp:
        return QStringLiteral("把每个点迭代移向其实测值对应的等值线。要求初始误差不大。");
    case Method::Sitan:
        return QStringLiteral("先做 TERCOM，再用卡尔曼滤波沿背景场梯度逐点修正。");
    case Method::TercomIccp:
        return QStringLiteral("先做只平移的 TERCOM 粗匹配，再用 ICCP 精匹配。");
    case Method::All:
        return QStringLiteral("依次运行 TERCOM、SITAN、ICCP 和 TERCOM + ICCP，在同一张图中对比。");
    }
    return QString();
}

QString methodKey(Method method)
{
    switch (method) {
    case Method::Tercom: return QStringLiteral("tercom");
    case Method::Iccp: return QStringLiteral("iccp");
    case Method::Sitan: return QStringLiteral("sitan");
    case Method::TercomIccp: return QStringLiteral("tercom_iccp");
    case Method::All: return QStringLiteral("all_methods");
    }
    return QStringLiteral("navigation");
}

namespace {

class Run
{
public:
    Run(const Job &job, const std::atomic_bool *cancel, const ProgressFn &progress)
        : job_(job), cancel_(cancel), progress_(progress)
    {
    }

    Outcome execute();

private:
    void log(const QString &line)
    {
        if (progress_)
            progress_(line);
    }
    bool load();
    // Records a matched track: accuracy, file, log line.
    void addTrack(const QString &key, const QString &label, const Path &path, qint64 ms);
    void writeAccuracy();

    TercomResult runTercom(bool rotation);
    void fail(const QString &what, const QString &error);
    void runMethods();

    const Job &job_;
    const std::atomic_bool *cancel_;
    ProgressFn progress_;
    Outcome out_;
    QVector<MapPoint> map_;
    Track ins_;
    QElapsedTimer stepTimer_;
};

void Run::fail(const QString &what, const QString &error)
{
    // the form reports the outcome; nothing to log here
    if (error == QStringLiteral("已取消"))
        out_.cancelled = true;
    else
        out_.error = QStringLiteral("%1失败：%2").arg(what, error);
}

bool Run::load()
{
    if (!(job_.dx > 0) || !(job_.dy > 0)) {
        out_.error = QStringLiteral("背景场分辨率必须大于 0");
        return false;
    }
    if (!QDir().mkpath(job_.outputDir)) {
        out_.error = QStringLiteral("无法创建输出文件夹：%1").arg(job_.outputDir);
        return false;
    }

    LoadReport report;
    map_ = readMap(job_.mapFile, &report);
    if (!report.ok()) {
        out_.error = QStringLiteral("背景场：%1").arg(report.error);
        return false;
    }
    log(QStringLiteral("背景场 %1：%2").arg(QFileInfo(job_.mapFile).fileName(), report.summary()));
    for (const QString &line : report.skippedSamples)
        log(QStringLiteral("    %1").arg(line));

    ins_ = readTrack(job_.insFile, &report);
    if (!report.ok()) {
        out_.error = QStringLiteral("INS 航迹：%1").arg(report.error);
        return false;
    }
    log(QStringLiteral("INS 航迹 %1：%2").arg(QFileInfo(job_.insFile).fileName(), report.summary()));
    if (ins_.size() < 2) {
        out_.error = QStringLiteral("INS 航迹至少需要 2 个点");
        return false;
    }
    out_.ins = positionsOf(ins_);

    if (!job_.truthFile.trimmed().isEmpty()) {
        out_.truth = readPath(job_.truthFile, &report);
        if (!report.ok()) {
            log(QStringLiteral("真实航迹：%1，不计算精度").arg(report.error));
            out_.truth.clear();
        } else {
            log(QStringLiteral("真实航迹 %1：%2").arg(QFileInfo(job_.truthFile).fileName(), report.summary()));
            if (out_.truth.size() != ins_.size())
                log(QStringLiteral("注意：真实航迹 %1 个点，INS 航迹 %2 个点，精度按前 %3 个点逐点计算")
                        .arg(out_.truth.size()).arg(ins_.size()).arg(qMin(out_.truth.size(), ins_.size())));
        }
    }

    QStringList warnings;
    out_.grid = GridField::fromPoints(map_, job_.dx, job_.dy, &warnings);
    for (const QString &w : warnings)
        log(QStringLiteral("注意：%1").arg(w));
    if (out_.grid.isEmpty()) {
        out_.error = QStringLiteral("无法由背景场建立格网，请检查分辨率");
        return false;
    }
    log(QStringLiteral("背景场格网 %1 × %2，范围 X %3 ~ %4，Y %5 ~ %6")
            .arg(out_.grid.columns()).arg(out_.grid.rows())
            .arg(out_.grid.xMin(), 0, 'g', 8).arg(out_.grid.xMax(), 0, 'g', 8)
            .arg(out_.grid.yMin(), 0, 'g', 8).arg(out_.grid.yMax(), 0, 'g', 8));

    // an INS track far away from the map usually means different coordinate systems
    int outside = 0;
    for (const TrackPoint &p : ins_) {
        if (p.x < out_.grid.xMin() || p.x > out_.grid.xMax() || p.y < out_.grid.yMin() || p.y > out_.grid.yMax())
            ++outside;
    }
    if (outside == ins_.size()) {
        out_.error = QStringLiteral("INS 航迹完全在背景场范围之外，请确认两者使用同一坐标系");
        return false;
    }
    if (outside > 0)
        log(QStringLiteral("注意：INS 航迹有 %1 个点在背景场范围之外").arg(outside));
    return true;
}

void Run::addTrack(const QString &key, const QString &label, const Path &path, qint64 ms)
{
    MatchedTrack t;
    t.key = key;
    t.label = label;
    t.path = path;
    if (!out_.truth.isEmpty())
        t.rms = rmsError(path, out_.truth, &t.comparedPoints);

    // x, y and the measured value of each point
    QVector<double> measured;
    measured.reserve(path.size());
    for (int i = 0; i < path.size() && i < ins_.size(); ++i)
        measured.append(ins_[i].magnetic);
    t.file = QDir(job_.outputDir).filePath(key + QStringLiteral("_match.txt"));
    QString error;
    if (!writePath(t.file, path, measured, &error)) {
        log(error);
        t.file.clear();
    }

    QString line = QStringLiteral("%1 完成，用时 %2 ms").arg(label).arg(ms);
    if (std::isfinite(t.rms))
        line += QStringLiteral("，均方根误差 %1").arg(t.rms, 0, 'f', 4);
    log(line);
    out_.tracks.append(t);
}

void Run::writeAccuracy()
{
    if (out_.truth.isEmpty() || out_.tracks.isEmpty())
        return;
    const QString path = QDir(job_.outputDir).filePath(QStringLiteral("navigation_accuracy.txt"));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);
    out.setRealNumberPrecision(8);
    out << "# method, rms position error, compared points\n";
    out << "INS," << rmsError(out_.ins, out_.truth) << ',' << qMin(out_.ins.size(), out_.truth.size()) << '\n';
    for (const MatchedTrack &t : out_.tracks)
        out << t.label << ',' << t.rms << ',' << t.comparedPoints << '\n';
    out.flush();
    if (file.commit())
        out_.accuracyFile = path;
}

TercomResult Run::runTercom(bool rotation)
{
    TercomOptions options;
    options.searchRadius = job_.searchRadius;
    options.idwRadius = job_.searchRadius;
    options.searchRotation = rotation;
    log(rotation ? QStringLiteral("TERCOM 匹配（平移 + 航向）…") : QStringLiteral("TERCOM 粗匹配（仅平移）…"));
    stepTimer_.start();
    const TercomMatcher matcher(map_);
    TercomResult r = matcher.match(ins_, options, cancel_);
    if (r.ok())
        log(QStringLiteral("    试探起点 %1 个，平移 (%2, %3)，航向修正 %4°，磁场均方差 %5")
                .arg(r.candidates)
                .arg(r.shift.x(), 0, 'f', 3).arg(r.shift.y(), 0, 'f', 3)
                .arg(r.rotationDeg, 0, 'f', 2)
                .arg(r.msd, 0, 'f', 4));
    return r;
}

Outcome Run::execute()
{
    QElapsedTimer total;
    total.start();
    try {
        if (load())
            runMethods();
    } catch (const std::bad_alloc &) {
        out_.error = QStringLiteral("内存不足，请减小背景场范围或提高分辨率数值后重试");
    } catch (const std::exception &e) {
        out_.error = QStringLiteral("计算出错：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    out_.elapsedMs = total.elapsed();
    return out_;
}

void Run::runMethods()
{
    if (!out_.truth.isEmpty())
        log(QStringLiteral("INS 原始航迹的均方根误差 %1").arg(rmsError(out_.ins, out_.truth), 0, 'f', 4));

    const Method m = job_.method;
    const IccpMatcher iccp(out_.grid);

    // TERCOM with heading search: TERCOM, SITAN and All
    TercomResult tercom;
    if (m == Method::Tercom || m == Method::Sitan || m == Method::All) {
        tercom = runTercom(true);
        if (!tercom.ok()) {
            fail(QStringLiteral("TERCOM "), tercom.error);
            return;
        }
        addTrack(QStringLiteral("tercom"), QStringLiteral("TERCOM"), tercom.positions, stepTimer_.elapsed());
    }

    if (m == Method::Sitan || m == Method::All) {
        log(QStringLiteral("SITAN 滤波（以 TERCOM 结果为初值）…"));
        stepTimer_.start();
        const SitanResult r = SitanMatcher(out_.grid).match(withPositions(ins_, tercom.positions), SitanOptions(), cancel_);
        if (!r.ok()) {
            fail(QStringLiteral("SITAN "), r.error);
            return;
        }
        if (r.skippedUpdates > 0)
            log(QStringLiteral("    %1 个点附近背景场数据不足，未做修正").arg(r.skippedUpdates));
        addTrack(QStringLiteral("sitan"), QStringLiteral("SITAN"), r.positions, stepTimer_.elapsed());
    }

    if (m == Method::Iccp || m == Method::All) {
        log(QStringLiteral("ICCP 匹配…"));
        stepTimer_.start();
        const IccpResult r = iccp.match(ins_, IccpOptions(), cancel_);
        if (!r.ok()) {
            fail(QStringLiteral("ICCP "), r.error);
            return;
        }
        log(QStringLiteral("    迭代 %1 次%2").arg(r.iterations).arg(r.converged ? QStringLiteral("，已收敛") : QStringLiteral("，达到迭代上限")));
        if (r.pointsWithoutContour > 0)
            log(QStringLiteral("    %1 个点的实测值超出背景场取值范围，保持原位").arg(r.pointsWithoutContour));
        addTrack(QStringLiteral("iccp"), QStringLiteral("ICCP"), r.positions, stepTimer_.elapsed());
    }

    if (m == Method::TercomIccp || m == Method::All) {
        Path start = tercom.positions;
        if (m == Method::TercomIccp) {
            const TercomResult coarse = runTercom(false);
            if (!coarse.ok()) {
                fail(QStringLiteral("TERCOM "), coarse.error);
                return;
            }
            addTrack(QStringLiteral("tercom_plain"), QStringLiteral("TERCOM（仅平移）"), coarse.positions, stepTimer_.elapsed());
            start = coarse.positions;
        }
        log(QStringLiteral("ICCP 精匹配（以 TERCOM 结果为初值）…"));
        stepTimer_.start();
        IccpOptions options;
        if (m == Method::TercomIccp)
            options.tolerance = 1e-3;   // the start is already close
        const IccpResult r = iccp.match(withPositions(ins_, start), options, cancel_);
        if (!r.ok()) {
            fail(QStringLiteral("ICCP "), r.error);
            return;
        }
        log(QStringLiteral("    迭代 %1 次").arg(r.iterations));
        addTrack(QStringLiteral("tercom_iccp"), QStringLiteral("TERCOM + ICCP"), r.positions, stepTimer_.elapsed());
    }

    writeAccuracy();
    if (!out_.tracks.isEmpty())
        log(QStringLiteral("结果已保存到 %1").arg(QDir::toNativeSeparators(job_.outputDir)));
}

} // namespace

Outcome runJob(const Job &job, const std::atomic_bool *cancel, const ProgressFn &progress)
{
    return Run(job, cancel, progress).execute();
}

} // namespace Nav
