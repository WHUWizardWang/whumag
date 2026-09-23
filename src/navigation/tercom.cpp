#include "tercom.h"

#include "nanoflann.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Nav {

namespace {

constexpr double kDegToRad = M_PI / 180.0;
constexpr double kInf = std::numeric_limits<double>::infinity();

struct MapCloud
{
    std::vector<MapPoint> points;
    size_t kdtree_get_point_count() const { return points.size(); }
    double kdtree_get_pt(size_t i, size_t dim) const { return dim == 0 ? points[i].x : points[i].y; }
    template <class Box> bool kdtree_get_bbox(Box &) const { return false; }
};

using MapTree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, MapCloud>, MapCloud, 2, size_t>;

bool cancelled(const std::atomic_bool *flag)
{
    return flag && flag->load(std::memory_order_relaxed);
}

} // namespace

struct TercomMatcher::Index
{
    MapCloud cloud;
    std::unique_ptr<MapTree> tree;
};

TercomMatcher::TercomMatcher(const QVector<MapPoint> &map) : index_(new Index)
{
    index_->cloud.points.assign(map.begin(), map.end());
    if (!index_->cloud.points.empty()) {
        index_->tree.reset(new MapTree(2, index_->cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10)));
        index_->tree->buildIndex();
    }
}

TercomMatcher::~TercomMatcher() = default;

bool TercomMatcher::isEmpty() const
{
    return !index_->tree;
}

double TercomMatcher::interpolate(double x, double y, const TercomOptions &options) const
{
    const size_t k = size_t(std::max(1, options.idwNeighbours));
    size_t idx[64];
    double d2[64];
    const double query[2] = {x, y};
    const size_t found = index_->tree->knnSearch(query, std::min<size_t>(k, 64), idx, d2);   // sorted, nearest first
    if (found == 0)
        return 0.0;

    const double r2 = options.idwRadius * options.idwRadius;
    const std::vector<MapPoint> &pts = index_->cloud.points;
    if (!(d2[0] < r2))
        return pts[idx[0]].value;   // nothing within the radius: nearest sample

    double sumW = 0, sumV = 0;
    for (size_t i = 0; i < found && d2[i] < r2; ++i) {
        const double d = std::sqrt(d2[i]);
        if (d < 1e-6)
            return pts[idx[i]].value;   // on a sample
        const double w = 1.0 / d;
        sumW += w;
        sumV += w * pts[idx[i]].value;
    }
    return sumV / sumW;
}

TercomResult TercomMatcher::match(const Track &track, const TercomOptions &options, const std::atomic_bool *cancel) const
{
    TercomResult result;
    if (isEmpty()) {
        result.error = QStringLiteral("背景场为空");
        return result;
    }
    if (track.size() < 2) {
        result.error = QStringLiteral("INS 航迹少于 2 个点，无法匹配");
        return result;
    }

    const int n = track.size();
    const double startX = track[0].x, startY = track[0].y;
    // track relative to its start; a candidate puts the start on a background sample
    std::vector<double> relX(n), relY(n);
    for (int i = 0; i < n; ++i) {
        relX[i] = track[i].x - startX;
        relY[i] = track[i].y - startY;
    }

    // sum of squared differences for start (sx, sy) and heading change |angle|; stops early once
    // it exceeds |limit| (the caller only needs to know it is not better)
    auto cost = [&](double sx, double sy, double angle, double limit) {
        const double c = std::cos(angle), s = std::sin(angle);
        double ssd = 0;
        for (int i = 0; i < n; ++i) {
            const double x = sx + c * relX[i] - s * relY[i];
            const double y = sy + s * relX[i] + c * relY[i];
            const double d = interpolate(x, y, options) - track[i].magnetic;
            ssd += d * d;
            if (ssd > limit)
                return kInf;
        }
        return ssd;
    };

    // ---- candidate start positions
    std::vector<nanoflann::ResultItem<size_t, double>> near;
    const double query[2] = {startX, startY};
    index_->tree->radiusSearch(query, options.searchRadius * options.searchRadius, near, nanoflann::SearchParameters(0, false));
    std::vector<size_t> starts;
    starts.reserve(near.size());
    for (const auto &hit : near)
        starts.push_back(hit.first);
    std::sort(starts.begin(), starts.end());   // file order: deterministic results
    if (starts.empty()) {
        result.error = QStringLiteral("INS 起点 %1 范围内没有背景场数据，请检查航迹与背景场是否在同一坐标系")
                           .arg(options.searchRadius);
        return result;
    }
    result.candidates = int(starts.size());
    const std::vector<MapPoint> &pts = index_->cloud.points;

    // ---- coarse search: every start, headings -max .. +max in coarse steps
    const int coarseSteps = options.searchRotation && options.coarseStepDeg > 0
                                ? int(std::lround(options.maxRotationDeg / options.coarseStepDeg))
                                : 0;
    const double coarseStep = options.coarseStepDeg * kDegToRad;
    const int m = int(starts.size());
    std::vector<double> bestCost(m, kInf), bestAngle(m, 0.0);

#pragma omp parallel for schedule(dynamic, 4)
    for (int k = 0; k < m; ++k) {
        if (cancelled(cancel))
            continue;
        const MapPoint &start = pts[starts[k]];
        for (int a = -coarseSteps; a <= coarseSteps; ++a) {
            const double angle = a * coarseStep;
            const double v = cost(start.x, start.y, angle, bestCost[k]);
            if (v < bestCost[k]) {
                bestCost[k] = v;
                bestAngle[k] = angle;
            }
        }
    }
    if (cancelled(cancel)) {
        result.error = QStringLiteral("已取消");
        return result;
    }

    // ---- fine search around the heading of the best few candidates
    std::vector<int> order(m);
    for (int k = 0; k < m; ++k)
        order[k] = k;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return bestCost[a] < bestCost[b] || (bestCost[a] == bestCost[b] && a < b);
    });
    if (coarseSteps > 0 && options.fineStepDeg > 0) {
        const int refine = std::min(m, std::max(1, options.refinedCandidates));
        const int fineSteps = std::max(1, int(std::lround(options.coarseStepDeg / options.fineStepDeg)));
        const double fineStep = options.fineStepDeg * kDegToRad;
#pragma omp parallel for schedule(dynamic, 1)
        for (int r = 0; r < refine; ++r) {
            const int k = order[r];
            const MapPoint &start = pts[starts[k]];
            for (int round = 0; round < options.maxRefineRounds && !cancelled(cancel); ++round) {
                const double centre = bestAngle[k];
                bool improved = false;
                for (int f = -fineSteps; f <= fineSteps; ++f) {
                    if (f == 0)
                        continue;
                    const double angle = centre + f * fineStep;
                    const double v = cost(start.x, start.y, angle, bestCost[k]);
                    if (v < bestCost[k]) {
                        bestCost[k] = v;
                        bestAngle[k] = angle;
                        improved = true;
                    }
                }
                if (!improved)
                    break;
            }
        }
        std::sort(order.begin(), order.begin() + refine, [&](int a, int b) {
            return bestCost[a] < bestCost[b] || (bestCost[a] == bestCost[b] && a < b);
        });
    }
    if (cancelled(cancel)) {
        result.error = QStringLiteral("已取消");
        return result;
    }

    // ---- the winner, with its full track
    const int best = order[0];
    const MapPoint &start = pts[starts[best]];
    const double angle = bestAngle[best];
    const double c = std::cos(angle), s = std::sin(angle);
    result.positions.reserve(n);
    result.mapValues.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double x = start.x + c * relX[i] - s * relY[i];
        const double y = start.y + s * relX[i] + c * relY[i];
        result.positions.append(QPointF(x, y));
        result.mapValues.append(interpolate(x, y, options));
    }
    result.msd = bestCost[best] / n;
    result.rotationDeg = angle / kDegToRad;
    result.shift = QPointF(start.x - startX, start.y - startY);
    return result;
}

} // namespace Nav
