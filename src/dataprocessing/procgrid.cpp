#include "procgrid.h"

#include "nanoflann.hpp"

#include <algorithm>

namespace Proc {

namespace {

struct Cloud
{
    const std::vector<Sample> *pts;
    size_t kdtree_get_point_count() const { return pts->size(); }
    double kdtree_get_pt(size_t i, size_t d) const { return d == 0 ? (*pts)[i].x : (*pts)[i].y; }
    template <class B> bool kdtree_get_bbox(B &) const { return false; }
};
using Tree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, Cloud>, Cloud, 2, size_t>;

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

} // namespace

int Grid::validCount() const
{
    int n = 0;
    for (double x : v)
        if (!std::isnan(x))
            ++n;
    return n;
}

Grid gridSamples(const std::vector<Sample> &samples, double dx, double dy, QStringList *notes)
{
    Grid g;
    if (samples.empty() || !(dx > 0) || !(dy > 0) || !std::isfinite(dx) || !std::isfinite(dy))
        return g;

    double x0 = samples[0].x, x1 = x0, y0 = samples[0].y, y1 = y0;
    for (const Sample &s : samples) {
        x0 = std::min(x0, s.x);
        x1 = std::max(x1, s.x);
        y0 = std::min(y0, s.y);
        y1 = std::max(y1, s.y);
    }
    const double cols = std::floor((x1 - x0) / dx + 1e-6) + 1;
    const double rows = std::floor((y1 - y0) / dy + 1e-6) + 1;
    if (cols * rows > 40e6) {
        if (notes)
            notes->append(QStringLiteral("按该分辨率格网为 %1 × %2，过大，请检查分辨率").arg(cols).arg(rows));
        return g;
    }
    g.cols = int(cols);
    g.rows = int(rows);
    g.x0 = x0;
    g.y0 = y0;
    g.dx = dx;
    g.dy = dy;
    g.v.assign(size_t(g.rows) * g.cols, kNaN);

    // samples sitting on nodes
    std::vector<double> sum(g.v.size(), 0.0);
    std::vector<int> count(g.v.size(), 0);
    int onNode = 0;
    for (const Sample &s : samples) {
        const double fx = (s.x - x0) / dx, fy = (s.y - y0) / dy;
        const double rx = std::round(fx), ry = std::round(fy);
        if (std::fabs(fx - rx) < 0.05 && std::fabs(fy - ry) < 0.05 && rx < g.cols && ry < g.rows) {
            const size_t i = size_t(ry) * g.cols + size_t(rx);
            sum[i] += s.v;
            ++count[i];
            ++onNode;
        }
    }

    if (onNode >= 0.9 * samples.size()) {
        for (size_t i = 0; i < g.v.size(); ++i)
            if (count[i] > 0)
                g.v[i] = sum[i] / count[i];
        if (notes && onNode < int(samples.size()))
            notes->append(QStringLiteral("%1 个数据点不在格网节点上，已忽略").arg(int(samples.size()) - onNode));
    } else {
        // scattered data: IDW of the nearest samples within two cells
        Cloud cloud{&samples};
        Tree tree(2, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        tree.buildIndex();
        const double r2 = std::pow(2.0 * std::max(dx, dy), 2);
#pragma omp parallel for schedule(dynamic, 16)
        for (int r = 0; r < g.rows; ++r) {
            size_t idx[8];
            double d2[8];
            for (int c = 0; c < g.cols; ++c) {
                const double q[2] = {g.nodeX(c), g.nodeY(r)};
                const size_t found = tree.knnSearch(q, 8, idx, d2);
                double sw = 0, sv = 0;
                for (size_t k = 0; k < found && d2[k] <= r2; ++k) {
                    if (d2[k] < 1e-18) {
                        sw = 1;
                        sv = samples[idx[k]].v;
                        break;
                    }
                    const double w = 1.0 / d2[k];
                    sw += w;
                    sv += w * samples[idx[k]].v;
                }
                if (sw > 0)
                    g.at(r, c) = sv / sw;
            }
        }
        if (notes)
            notes->append(QStringLiteral("数据不在规则格网上，已按反距离加权插值到 %1 × %2 格网").arg(g.cols).arg(g.rows));
    }

    const int empty = int(g.v.size()) - g.validCount();
    if (notes && empty > 0)
        notes->append(QStringLiteral("格网 %1 × %2 中有 %3 个节点附近没有数据（结果中不输出这些节点）")
                          .arg(g.cols).arg(g.rows).arg(empty));
    return g;
}

std::vector<Sample> gridToSamples(const Grid &grid)
{
    std::vector<Sample> out;
    out.reserve(grid.v.size());
    for (int r = 0; r < grid.rows; ++r)
        for (int c = 0; c < grid.cols; ++c)
            if (!std::isnan(grid.at(r, c)))
                out.push_back({grid.nodeX(c), grid.nodeY(r), grid.at(r, c)});
    return out;
}

Grid filledGrid(const Grid &grid)
{
    Grid g = grid;
    const int valid = g.validCount();
    if (valid == 0 || valid == int(g.v.size()))
        return g;

    double mean = 0, lo = 1e300, hi = -1e300;
    for (double x : g.v)
        if (!std::isnan(x)) {
            mean += x;
            lo = std::min(lo, x);
            hi = std::max(hi, x);
        }
    mean /= valid;

    std::vector<char> fixed(g.v.size());
    for (size_t i = 0; i < g.v.size(); ++i) {
        fixed[i] = !std::isnan(g.v[i]);
        if (!fixed[i])
            g.v[i] = mean;
    }

    // successive over-relaxation of Laplace's equation over the holes
    const double tol = 1e-6 * std::max(hi - lo, 1e-12);
    const double omega = 1.8;
    for (int iter = 0; iter < 2000; ++iter) {
        double change = 0;
        for (int r = 0; r < g.rows; ++r) {
            for (int c = 0; c < g.cols; ++c) {
                const size_t i = size_t(r) * g.cols + c;
                if (fixed[i])
                    continue;
                double s = 0;
                int n = 0;
                if (r > 0) { s += g.v[i - g.cols]; ++n; }
                if (r + 1 < g.rows) { s += g.v[i + g.cols]; ++n; }
                if (c > 0) { s += g.v[i - 1]; ++n; }
                if (c + 1 < g.cols) { s += g.v[i + 1]; ++n; }
                const double next = g.v[i] + omega * (s / n - g.v[i]);
                change = std::max(change, std::fabs(next - g.v[i]));
                g.v[i] = next;
            }
        }
        if (change < tol)
            break;
    }
    return g;
}

LocalPlane LocalPlane::around(const std::vector<Sample> &lonLat)
{
    LocalPlane p;
    if (lonLat.empty())
        return p;
    double lo0 = lonLat[0].x, lo1 = lo0, la0 = lonLat[0].y, la1 = la0;
    for (const Sample &s : lonLat) {
        lo0 = std::min(lo0, s.x);
        lo1 = std::max(lo1, s.x);
        la0 = std::min(la0, s.y);
        la1 = std::max(la1, s.y);
    }
    p.lon0 = 0.5 * (lo0 + lo1);
    p.lat0 = 0.5 * (la0 + la1);
    constexpr double kEarthRadius = 6371008.8;
    p.metresPerDegLat = kEarthRadius * M_PI / 180.0;
    p.metresPerDegLon = p.metresPerDegLat * std::cos(p.lat0 * M_PI / 180.0);
    return p;
}

} // namespace Proc
