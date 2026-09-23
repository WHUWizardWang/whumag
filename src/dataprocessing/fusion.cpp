#include "fusion.h"

#include "navigation/navdata.h"
#include "nanoflann.hpp"

#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <exception>
#include <memory>
#include <new>

namespace Proc {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// A sample in normalised coordinates (the window becomes the unit circle) with its weight 1 / sigma^2.
struct Point
{
    double u, w, v, weight;
    int source;
};

struct Cloud
{
    const std::vector<Point> *pts;
    size_t kdtree_get_point_count() const { return pts->size(); }
    double kdtree_get_pt(size_t i, size_t d) const { return d == 0 ? (*pts)[i].u : (*pts)[i].w; }
    template <class B> bool kdtree_get_bbox(B &) const { return false; }
};
using Tree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, Cloud>, Cloud, 2, size_t>;

class Index
{
public:
    explicit Index(std::vector<Point> pts) : pts_(std::move(pts)), cloud_{&pts_}
    {
        if (!pts_.empty()) {
            tree_.reset(new Tree(2, cloud_, nanoflann::KDTreeSingleIndexAdaptorParams(16)));
            tree_->buildIndex();
        }
    }

    // Shepard-weighted estimate at (u, w) from the points within the unit window.  |skip| is left out
    // (leave-one-out).  Returns NaN when fewer than |minNeighbours| points contribute.
    double estimate(double u, double w, int minNeighbours, size_t skip = size_t(-1), int *used = nullptr) const
    {
        if (!tree_)
            return kNaN;
        const double q[2] = {u, w};
        std::vector<nanoflann::ResultItem<size_t, double>> hits;
        tree_->radiusSearch(q, 1.0, hits, nanoflann::SearchParameters(0, false));
        double sw = 0, sv = 0, exactW = 0, exactV = 0;
        int n = 0;
        for (const auto &h : hits) {
            if (h.first == skip)
                continue;
            const Point &p = pts_[h.first];
            const double rho = std::sqrt(h.second);
            ++n;
            if (rho < 1e-9) {   // on the node: those samples alone decide
                exactW += p.weight;
                exactV += p.weight * p.v;
                continue;
            }
            // Shepard (1968): s = 1 / rho (rho <= 1/3), 27/4 (rho - 1)^2 (1/3 < rho <= 1); weight s^2
            const double s = rho <= 1.0 / 3.0 ? 1.0 / rho : 27.0 / 4.0 * (rho - 1.0) * (rho - 1.0);
            const double wgt = s * s * p.weight;
            sw += wgt;
            sv += wgt * p.v;
        }
        if (used)
            *used = n;
        if (n < minNeighbours)
            return kNaN;
        if (exactW > 0)
            return exactV / exactW;
        return sw > 0 ? sv / sw : kNaN;
    }

    // Median of the values of the (up to 12) nearest other points inside the window and their robust
    // spread (1.4826 * MAD); false when fewer than 4.  Unlike a weighted mean the median is not pulled
    // away by a neighbouring spike, and the spread grows where the field itself changes quickly.
    bool neighbourMedian(size_t i, double &median, double &spread) const
    {
        constexpr size_t K = 13;
        const double q[2] = {pts_[i].u, pts_[i].w};
        size_t idx[K];
        double d2[K];
        const size_t found = tree_->knnSearch(q, K, idx, d2);
        double vals[K];
        int n = 0;
        for (size_t k = 0; k < found; ++k)
            if (idx[k] != i && d2[k] <= 1.0)
                vals[n++] = pts_[idx[k]].v;
        if (n < 4)
            return false;
        auto mid = [](double *v, int m) {
            std::sort(v, v + m);
            return m % 2 ? v[m / 2] : 0.5 * (v[m / 2 - 1] + v[m / 2]);
        };
        median = mid(vals, n);
        for (int k = 0; k < n; ++k)
            vals[k] = std::fabs(vals[k] - median);
        spread = 1.4826 * mid(vals, n);
        return true;
    }

    const std::vector<Point> &points() const { return pts_; }

private:
    std::vector<Point> pts_;
    Cloud cloud_;
    std::unique_ptr<Tree> tree_;
};

double median(std::vector<double> v)
{
    if (v.empty())
        return kNaN;
    const size_t m = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + m, v.end());
    double med = v[m];
    if (v.size() % 2 == 0) {
        std::nth_element(v.begin(), v.begin() + m - 1, v.end());
        med = 0.5 * (med + v[m - 1]);
    }
    return med;
}

} // namespace

FusionResult fuse(std::vector<FusionSource> sources, const GridSpec &spec, const FusionOptions &options)
{
    FusionResult res;
    if (sources.empty()) {
        res.error = QStringLiteral("没有输入数据");
        return res;
    }
    if (!(options.windowX > 0) || !(options.windowY > 0)) {
        res.error = QStringLiteral("窗口大小必须大于 0");
        return res;
    }
    if (!(spec.dx > 0) || !(spec.dy > 0) || spec.xMax < spec.xMin || spec.yMax < spec.yMin) {
        res.error = QStringLiteral("融合范围或间隔不正确（间隔须大于 0，且“到”不小于“从”）");
        return res;
    }
    for (const FusionSource &s : sources) {
        if (!(s.sigma > 0)) {
            res.error = QStringLiteral("%1 的中误差必须大于 0").arg(s.name);
            return res;
        }
    }
    const double cols = std::floor((spec.xMax - spec.xMin) / spec.dx + 1e-6) + 1;
    const double rows = std::floor((spec.yMax - spec.yMin) / spec.dy + 1e-6) + 1;
    if (cols * rows > 40e6) {
        res.error = QStringLiteral("格网 %1 × %2 过大，请增大间隔").arg(cols).arg(rows);
        return res;
    }

    const double hx = options.windowX / 2, hy = options.windowY / 2;   // half widths
    auto toPoints = [&](int si) {
        std::vector<Point> pts;
        pts.reserve(sources[si].samples.size());
        const double weight = 1.0 / (sources[si].sigma * sources[si].sigma);
        for (const Sample &s : sources[si].samples)
            pts.push_back({s.x / hx, s.y / hy, s.v, weight, si});
        return pts;
    };

    res.sources.resize(sources.size());
    for (size_t i = 0; i < sources.size(); ++i) {
        res.sources[i].name = sources[i].name;
        res.sources[i].sigma = sources[i].sigma;
        res.sources[i].samples = int(sources[i].samples.size());
    }

    // ---- 1. gross errors, per source (Hampel identifier): a sample is compared with the median of its
    //         nearest neighbours of the same source, so a level difference between sources cannot cause
    //         rejections.  The threshold is k times the larger of the source's noise level (robust scale
    //         of all residuals) and the local spread of the neighbours, so steep anomalies are kept.
    if (options.rejectOutliers) {
        for (int si = 0; si < int(sources.size()); ++si) {
            std::vector<Sample> &smp = sources[si].samples;
            if (smp.size() < 10)
                continue;
            const Index index(toPoints(si));
            std::vector<double> resid(smp.size(), kNaN), local(smp.size(), kNaN);
#pragma omp parallel for schedule(dynamic, 256)
            for (int k = 0; k < int(smp.size()); ++k) {
                double m, spread;
                if (index.neighbourMedian(size_t(k), m, spread)) {
                    resid[k] = smp[k].v - m;
                    local[k] = spread;
                }
            }
            std::vector<double> r;
            for (double x : resid)
                if (std::isfinite(x))
                    r.push_back(x);
            if (r.size() < 10)
                continue;
            const double med = median(r);
            for (double &x : r)
                x = std::fabs(x - med);
            const double noise = 1.4826 * median(r);
            if (!(noise > 0))
                continue;
            std::vector<Sample> kept;
            kept.reserve(smp.size());
            for (size_t k = 0; k < smp.size(); ++k) {
                if (std::isfinite(resid[k])
                    && std::fabs(resid[k] - med) > options.outlierK * std::max(noise, local[k]))
                    ++res.sources[si].rejected;
                else
                    kept.push_back(smp[k]);
            }
            smp.swap(kept);
        }
    }

    // ---- 2. level adjustment against the most accurate source (median of the differences, so the
    //         remaining gross errors and the interpolation error of single points do not matter)
    if (options.removeBias && sources.size() > 1) {
        int ref = 0;
        for (int i = 1; i < int(sources.size()); ++i)
            if (sources[i].sigma < sources[ref].sigma
                || (sources[i].sigma == sources[ref].sigma && sources[i].samples.size() > sources[ref].samples.size()))
                ref = i;
        res.sources[ref].reference = true;
        const Index refIndex(toPoints(ref));
        for (int si = 0; si < int(sources.size()); ++si) {
            if (si == ref)
                continue;
            std::vector<Sample> &smp = sources[si].samples;
            std::vector<double> diff(smp.size(), kNaN);
#pragma omp parallel for schedule(dynamic, 256)
            for (int k = 0; k < int(smp.size()); ++k) {
                const double e = refIndex.estimate(smp[k].x / hx, smp[k].y / hy, 3);
                if (std::isfinite(e))
                    diff[k] = smp[k].v - e;
            }
            std::vector<double> d;
            for (double x : diff)
                if (std::isfinite(x))
                    d.push_back(x);
            res.sources[si].biasPairs = int(d.size());
            if (d.size() >= 20) {
                const double b = median(d);
                res.sources[si].bias = b;
                for (Sample &s : smp)
                    s.v -= b;
            }
        }
    }

    // ---- 3. cross validation: leave-one-out residual of every sample against all the others
    std::vector<Point> all;
    for (int si = 0; si < int(sources.size()); ++si) {
        const std::vector<Point> p = toPoints(si);
        all.insert(all.end(), p.begin(), p.end());
    }
    const Index index(std::move(all));
    {
        const std::vector<Point> &pts = index.points();
        std::vector<double> resid(pts.size(), kNaN);
#pragma omp parallel for schedule(dynamic, 256)
        for (int i = 0; i < int(pts.size()); ++i) {
            const double e = index.estimate(pts[i].u, pts[i].w, 3, size_t(i));
            if (std::isfinite(e))
                resid[i] = pts[i].v - e;
        }
        std::vector<double> ss(sources.size(), 0.0);
        std::vector<int> n(sources.size(), 0);
        for (size_t i = 0; i < pts.size(); ++i)
            if (std::isfinite(resid[i])) {
                ss[pts[i].source] += resid[i] * resid[i];
                ++n[pts[i].source];
            }
        for (size_t si = 0; si < sources.size(); ++si)
            if (n[si] > 0)
                res.sources[si].crossValidationRms = std::sqrt(ss[si] / n[si]);
    }

    // ---- 4. the grid
    Grid &g = res.grid;
    g.cols = int(cols);
    g.rows = int(rows);
    g.x0 = spec.xMin;
    g.y0 = spec.yMin;
    g.dx = spec.dx;
    g.dy = spec.dy;
    g.v.assign(size_t(g.rows) * g.cols, kNaN);
#pragma omp parallel for schedule(dynamic, 8)
    for (int r = 0; r < g.rows; ++r)
        for (int c = 0; c < g.cols; ++c)
            g.at(r, c) = index.estimate(g.nodeX(c) / hx, g.nodeY(r) / hy, 1);

    const int empty = int(g.v.size()) - g.validCount();
    if (empty > 0)
        res.notes << QStringLiteral("%1 个格网节点的窗口内没有数据，不输出").arg(empty);
    return res;
}

FusionOutcome runFusion(const FusionJob &job)
{
    FusionOutcome out;
    try {
        if (job.files.size() < 1 || job.sigmas.size() != job.files.size()) {
            out.error = QStringLiteral("请至少选择一个数据文件，并为每个文件填写中误差");
            return out;
        }
        std::vector<FusionSource> sources;
        for (int i = 0; i < job.files.size(); ++i) {
            Nav::LoadReport report;
            const QVector<Nav::MapPoint> pts = Nav::readMap(job.files[i], &report);
            const QString name = QFileInfo(job.files[i]).fileName();
            if (!report.ok()) {
                out.error = QStringLiteral("%1：%2").arg(name, report.error);
                return out;
            }
            out.log << QStringLiteral("%1：%2").arg(name, report.summary());
            FusionSource s;
            s.name = name;
            s.sigma = job.sigmas[i];
            s.samples.reserve(pts.size());
            for (const Nav::MapPoint &p : pts)
                s.samples.push_back({p.x, p.y, p.value});
            sources.push_back(std::move(s));
        }

        const FusionResult r = fuse(std::move(sources), job.grid, job.options);
        if (!r.ok()) {
            out.error = r.error;
            return out;
        }
        for (const SourceReport &s : r.sources) {
            QString line = QStringLiteral("%1（中误差 %2）：").arg(s.name).arg(s.sigma);
            if (s.reference)
                line += QStringLiteral("作为参考数据；");
            else if (job.options.removeBias && std::isfinite(s.bias))
                line += QStringLiteral("系统偏差 %1（%2 个重叠点），已改正；").arg(s.bias, 0, 'f', 3).arg(s.biasPairs);
            else if (job.options.removeBias && r.sources.size() > 1)
                line += QStringLiteral("与参考数据重叠点不足（%1 个），未改正偏差；").arg(s.biasPairs);
            if (job.options.rejectOutliers)
                line += QStringLiteral("剔除粗差 %1 个；").arg(s.rejected);
            if (std::isfinite(s.crossValidationRms))
                line += QStringLiteral("交叉验证均方根 %1").arg(s.crossValidationRms, 0, 'f', 3);
            out.log << line;
        }
        out.log << r.notes;

        QSaveFile file(job.outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out.error = QStringLiteral("无法写入 %1：%2").arg(job.outputFile, file.errorString());
            return out;
        }
        QTextStream ts(&file);
        ts.setRealNumberNotation(QTextStream::FixedNotation);
        for (const Sample &s : gridToSamples(r.grid)) {
            ts.setRealNumberPrecision(8);
            ts << s.x << ' ' << s.y << ' ';
            ts.setRealNumberPrecision(4);
            ts << s.v << '\n';
            ++out.writtenNodes;
        }
        ts.flush();
        if (!file.commit()) {
            out.error = QStringLiteral("无法写入 %1：%2").arg(job.outputFile, file.errorString());
            return out;
        }
        out.log << QStringLiteral("融合格网 %1 × %2，写出 %3 个节点到 %4")
                       .arg(r.grid.cols).arg(r.grid.rows).arg(out.writtenNodes).arg(QFileInfo(job.outputFile).fileName());
    } catch (const std::bad_alloc &) {
        out.error = QStringLiteral("内存不足，请增大格网间隔或缩小范围");
    } catch (const std::exception &e) {
        out.error = QStringLiteral("计算出错：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    return out;
}

} // namespace Proc
