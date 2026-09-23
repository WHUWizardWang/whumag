#include "navdata.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>

namespace Nav {

Path positionsOf(const Track &track)
{
    Path path;
    path.reserve(track.size());
    for (const TrackPoint &p : track)
        path.append(QPointF(p.x, p.y));
    return path;
}

Track withPositions(const Track &track, const Path &positions)
{
    Track out = track;
    const int n = qMin(out.size(), positions.size());
    out.resize(n);
    for (int i = 0; i < n; ++i) {
        out[i].x = positions[i].x();
        out[i].y = positions[i].y();
    }
    return out;
}

// ---------------------------------------------------------------- GridField
namespace {

// Median spacing between the distinct coordinates of |coords| (0 when it cannot be told).
double medianSpacing(QVector<double> coords)
{
    if (coords.size() < 2)
        return 0;
    std::sort(coords.begin(), coords.end());
    const double span = coords.last() - coords.first();
    const double eps = span > 0 ? span * 1e-9 : 1e-12;
    QVector<double> gaps;
    for (int i = 1; i < coords.size(); ++i) {
        const double d = coords[i] - coords[i - 1];
        if (d > eps)
            gaps.append(d);
    }
    if (gaps.isEmpty())
        return 0;
    std::nth_element(gaps.begin(), gaps.begin() + gaps.size() / 2, gaps.end());
    return gaps[gaps.size() / 2];
}

} // namespace

GridField GridField::fromPoints(const QVector<MapPoint> &points, double dx, double dy, QStringList *warnings)
{
    GridField g;
    if (points.isEmpty() || !(dx > 0) || !(dy > 0))
        return g;

    double x0 = points[0].x, x1 = x0, y0 = points[0].y, y1 = y0;
    QVector<double> xs, ys;
    xs.reserve(points.size());
    ys.reserve(points.size());
    for (const MapPoint &p : points) {
        x0 = std::min(x0, p.x);
        x1 = std::max(x1, p.x);
        y0 = std::min(y0, p.y);
        y1 = std::max(y1, p.y);
        xs.append(p.x);
        ys.append(p.y);
    }

    // a resolution that does not match the data leaves holes or merges samples: say so
    if (warnings) {
        const double sx = medianSpacing(xs), sy = medianSpacing(ys);
        if (sx > 0 && std::fabs(sx - dx) > 0.05 * dx)
            warnings->append(QStringLiteral("背景场 X 方向的实际间距约为 %1，与设置的分辨率 %2 不一致").arg(sx, 0, 'g', 6).arg(dx));
        if (sy > 0 && std::fabs(sy - dy) > 0.05 * dy)
            warnings->append(QStringLiteral("背景场 Y 方向的实际间距约为 %1，与设置的分辨率 %2 不一致").arg(sy, 0, 'g', 6).arg(dy));
    }

    const double cols = std::round((x1 - x0) / dx) + 1;
    const double rows = std::round((y1 - y0) / dy) + 1;
    constexpr double kMaxCells = 40e6;   // ~320 MB of doubles
    if (cols * rows > kMaxCells) {
        if (warnings)
            warnings->append(QStringLiteral("按当前分辨率背景场格网为 %1 × %2，过大，请检查分辨率设置").arg(cols).arg(rows));
        return g;
    }

    g.nx_ = int(cols);
    g.ny_ = int(rows);
    g.x0_ = x0;
    g.y0_ = y0;
    g.dx_ = dx;
    g.dy_ = dy;

    QVector<double> sum(g.nx_ * g.ny_, 0.0);
    QVector<int> count(g.nx_ * g.ny_, 0);
    for (const MapPoint &p : points) {
        if (!std::isfinite(p.value))
            continue;
        const int ix = qBound(0, int(std::lround((p.x - x0) / dx)), g.nx_ - 1);
        const int iy = qBound(0, int(std::lround((p.y - y0) / dy)), g.ny_ - 1);
        sum[iy * g.nx_ + ix] += p.value;
        ++count[iy * g.nx_ + ix];
    }

    g.values_.resize(sum.size());
    for (int i = 0; i < sum.size(); ++i) {
        if (count[i] == 0) {
            g.values_[i] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        const double v = sum[i] / count[i];
        g.values_[i] = v;
        ++g.validCells_;
        if (g.validCells_ == 1 || v < g.minValue_)
            g.minValue_ = v;
        if (g.validCells_ == 1 || v > g.maxValue_)
            g.maxValue_ = v;
    }

    const int holes = g.values_.size() - g.validCells_;
    if (warnings && holes > 0)
        warnings->append(QStringLiteral("背景场格网 %1 × %2 中有 %3 个格点没有数据，按空白处理").arg(g.nx_).arg(g.ny_).arg(holes));
    return g;
}

// ---------------------------------------------------------------- reading
QString LoadReport::summary() const
{
    if (!ok())
        return error;
    QString s = QStringLiteral("读入 %1 行").arg(usedLines);
    if (skippedLines > 0)
        s += QStringLiteral("，跳过 %1 行格式不正确的数据").arg(skippedLines);
    return s;
}

namespace {

// Splits |line| at spaces, tabs, commas and semicolons and parses the first |need| fields.
bool parseFields(const QByteArray &line, int need, double *out)
{
    const char *p = line.constData();
    const char *end = p + line.size();
    int found = 0;
    while (p < end && found < need) {
        while (p < end && (*p == ' ' || *p == '\t' || *p == ',' || *p == ';' || *p == '\r' || *p == '\n'))
            ++p;
        const char *start = p;
        while (p < end && !(*p == ' ' || *p == '\t' || *p == ',' || *p == ';' || *p == '\r' || *p == '\n'))
            ++p;
        if (p == start)
            break;
        bool ok = false;
        const double v = QByteArray::fromRawData(start, int(p - start)).toDouble(&ok);
        if (!ok || !std::isfinite(v))
            return false;
        out[found++] = v;
    }
    return found == need;
}

// Calls |take| with the first |need| numbers of every usable line.
template <typename Take>
void readRecords(const QString &path, int need, LoadReport *report, Take take)
{
    LoadReport local;
    LoadReport &r = report ? *report : local;
    r = LoadReport();

    if (path.trimmed().isEmpty()) {
        r.error = QStringLiteral("未指定文件");
        return;
    }
    QFile file(path);
    if (!file.exists()) {
        r.error = QStringLiteral("文件不存在：%1").arg(path);
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        r.error = QStringLiteral("无法打开 %1：%2").arg(path, file.errorString());
        return;
    }

    double values[3];
    int lineNo = 0;
    bool sawData = false;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        ++lineNo;
        if (line.isEmpty() || line.startsWith('#') || line.startsWith("//"))
            continue;
        if (!parseFields(line, need, values)) {
            if (!sawData && r.skippedLines == 0 && lineNo <= 3)
                continue;   // a header line before the data
            ++r.skippedLines;
            if (r.skippedSamples.size() < 5)
                r.skippedSamples.append(QStringLiteral("第 %1 行: %2").arg(lineNo).arg(QString::fromLocal8Bit(line.left(80))));
            continue;
        }
        sawData = true;
        ++r.usedLines;
        take(values);
    }
    if (r.usedLines == 0)
        r.error = QStringLiteral("%1 中没有可用的数据行（每行至少需要 %2 列数字）").arg(QFileInfo(path).fileName()).arg(need);
}

} // namespace

QVector<MapPoint> readMap(const QString &path, LoadReport *report)
{
    QVector<MapPoint> points;
    readRecords(path, 3, report, [&points](const double *v) { points.append(MapPoint{v[0], v[1], v[2]}); });
    return points;
}

Track readTrack(const QString &path, LoadReport *report)
{
    Track track;
    readRecords(path, 3, report, [&track](const double *v) { track.append(TrackPoint{v[0], v[1], v[2]}); });
    return track;
}

Path readPath(const QString &path, LoadReport *report)
{
    Path points;
    readRecords(path, 2, report, [&points](const double *v) { points.append(QPointF(v[0], v[1])); });
    return points;
}

// ---------------------------------------------------------------- writing / accuracy
bool writePath(const QString &path, const Path &points, const QVector<double> &values, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("无法写入 %1：%2").arg(path, file.errorString());
        return false;
    }
    const bool withValues = values.size() == points.size();
    QTextStream out(&file);
    out.setRealNumberPrecision(10);
    for (int i = 0; i < points.size(); ++i) {
        out << points[i].x() << ',' << points[i].y();
        if (withValues)
            out << ',' << values[i];
        out << '\n';
    }
    out.flush();
    if (!file.commit()) {
        if (error)
            *error = QStringLiteral("无法写入 %1：%2").arg(path, file.errorString());
        return false;
    }
    return true;
}

double rmsError(const Path &a, const Path &b, int *comparedPoints)
{
    const int n = qMin(a.size(), b.size());
    if (comparedPoints)
        *comparedPoints = n;
    if (n == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double sum = 0;
    for (int i = 0; i < n; ++i) {
        const double ex = a[i].x() - b[i].x();
        const double ey = a[i].y() - b[i].y();
        sum += ex * ex + ey * ey;
    }
    return std::sqrt(sum / n);
}

} // namespace Nav
