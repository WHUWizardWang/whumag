#include "iccp.h"

#include <QLineF>

#include <cmath>
#include <vector>

namespace Nav {

namespace {

struct Segment
{
    double ax, ay, bx, by;
};

// Marching squares, per cell (i, j) with corners (i,j) (i+1,j) (i+1,j+1) (i,j+1).
// Edges: 0 = (i,j)-(i+1,j), 1 = (i+1,j)-(i+1,j+1), 2 = (i+1,j+1)-(i,j+1), 3 = (i,j+1)-(i,j).
// Case bits: 8 = (i,j), 4 = (i,j+1), 2 = (i+1,j+1), 1 = (i+1,j) at or above the level.
const int kEdgePairs[16][4] = {
    {-1, -1, -1, -1}, {0, 1, -1, -1}, {1, 2, -1, -1}, {0, 2, -1, -1},
    {3, 2, -1, -1},   {0, 3, 1, 2},   {3, 1, -1, -1}, {0, 3, -1, -1},
    {0, 3, -1, -1},   {3, 1, -1, -1}, {0, 1, 3, 2},   {3, 2, -1, -1},
    {0, 2, -1, -1},   {1, 2, -1, -1}, {0, 1, -1, -1}, {-1, -1, -1, -1},
};

std::vector<Segment> contourSegments(const GridField &g, double level)
{
    std::vector<Segment> out;
    const int nx = g.columns(), ny = g.rows();
    for (int i = 0; i + 1 < nx; ++i) {
        for (int j = 0; j + 1 < ny; ++j) {
            const double v00 = g.at(i, j), v10 = g.at(i + 1, j), v11 = g.at(i + 1, j + 1), v01 = g.at(i, j + 1);
            if (std::isnan(v00) || std::isnan(v10) || std::isnan(v11) || std::isnan(v01))
                continue;   // hole: no contour through this cell
            const int code = (v00 >= level ? 8 : 0) | (v01 >= level ? 4 : 0) | (v11 >= level ? 2 : 0) | (v10 >= level ? 1 : 0);
            if (code == 0 || code == 15)
                continue;

            auto crossing = [&](int edge, double &x, double &y) {
                int ia, ja, ib, jb;
                double va, vb;
                switch (edge) {
                case 0: ia = i; ja = j; ib = i + 1; jb = j; va = v00; vb = v10; break;
                case 1: ia = i + 1; ja = j; ib = i + 1; jb = j + 1; va = v10; vb = v11; break;
                case 2: ia = i + 1; ja = j + 1; ib = i; jb = j + 1; va = v11; vb = v01; break;
                default: ia = i; ja = j + 1; ib = i; jb = j; va = v01; vb = v00; break;
                }
                double t = (vb == va) ? 0.5 : (level - va) / (vb - va);
                t = std::min(1.0, std::max(0.0, t));
                x = g.nodeX(ia) + t * (g.nodeX(ib) - g.nodeX(ia));
                y = g.nodeY(ja) + t * (g.nodeY(jb) - g.nodeY(ja));
            };

            const int *pairs = kEdgePairs[code];
            for (int p = 0; p < 4 && pairs[p] >= 0; p += 2) {
                Segment s;
                crossing(pairs[p], s.ax, s.ay);
                crossing(pairs[p + 1], s.bx, s.by);
                out.push_back(s);
            }
        }
    }
    return out;
}

QPointF closestPointOn(const std::vector<Segment> &segments, const QPointF &p)
{
    double best = std::numeric_limits<double>::infinity();
    QPointF bestPoint = p;
    for (const Segment &s : segments) {
        const double ux = s.bx - s.ax, uy = s.by - s.ay;
        const double len2 = ux * ux + uy * uy;
        double t = len2 > 0 ? ((p.x() - s.ax) * ux + (p.y() - s.ay) * uy) / len2 : 0.0;
        t = std::min(1.0, std::max(0.0, t));
        const double cx = s.ax + t * ux, cy = s.ay + t * uy;
        const double d2 = (cx - p.x()) * (cx - p.x()) + (cy - p.y()) * (cy - p.y());
        if (d2 < best) {
            best = d2;
            bestPoint = QPointF(cx, cy);
        }
    }
    return bestPoint;
}

// Rigid transform (rotation about the centroid + translation) that best maps |from| onto |to|
// in the least-squares sense, applied to |from|.
Path fitAndApplyRigidTransform(const Path &from, const Path &to)
{
    const int n = from.size();
    double fx = 0, fy = 0, tx = 0, ty = 0;
    for (int i = 0; i < n; ++i) {
        fx += from[i].x();
        fy += from[i].y();
        tx += to[i].x();
        ty += to[i].y();
    }
    fx /= n; fy /= n; tx /= n; ty /= n;

    double sumDot = 0, sumCross = 0;
    for (int i = 0; i < n; ++i) {
        const double x1 = from[i].x() - fx, y1 = from[i].y() - fy;
        const double x2 = to[i].x() - tx, y2 = to[i].y() - ty;
        sumDot += x1 * x2 + y1 * y2;
        sumCross += x1 * y2 - y1 * x2;
    }
    const double angle = std::atan2(sumCross, sumDot);
    const double c = std::cos(angle), s = std::sin(angle);

    Path out(n);
    for (int i = 0; i < n; ++i) {
        const double x1 = from[i].x() - fx, y1 = from[i].y() - fy;
        out[i] = QPointF(c * x1 - s * y1 + tx, s * x1 + c * y1 + ty);
    }
    return out;
}

} // namespace

IccpMatcher::IccpMatcher(const GridField &grid) : grid_(grid)
{
}

QVector<QLineF> IccpMatcher::contour(double level) const
{
    QVector<QLineF> lines;
    for (const Segment &s : contourSegments(grid_, level))
        lines.append(QLineF(s.ax, s.ay, s.bx, s.by));
    return lines;
}

IccpResult IccpMatcher::match(const Track &track, const IccpOptions &options, const std::atomic_bool *cancel) const
{
    IccpResult result;
    if (grid_.isEmpty() || grid_.columns() < 2 || grid_.rows() < 2) {
        result.error = QStringLiteral("背景场格网为空或过小");
        return result;
    }
    if (track.size() < 2) {
        result.error = QStringLiteral("INS 航迹少于 2 个点，无法匹配");
        return result;
    }
    auto stop = [cancel]() { return cancel && cancel->load(std::memory_order_relaxed); };

    // contour of each point's measured value (the expensive part: one pass over the grid each)
    const int n = track.size();
    std::vector<std::vector<Segment>> contours(n);
#pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < n; ++i) {
        if (!stop())
            contours[i] = contourSegments(grid_, track[i].magnetic);
    }
    if (stop()) {
        result.error = QStringLiteral("已取消");
        return result;
    }
    for (const auto &c : contours)
        if (c.empty())
            ++result.pointsWithoutContour;
    if (result.pointsWithoutContour == n) {
        result.error = QStringLiteral("所有测量值都超出背景场的取值范围，无法匹配");
        return result;
    }

    Path current = positionsOf(track);
    Path target(n);
    for (int iter = 0; iter < options.maxIterations; ++iter) {
        if (stop()) {
            result.error = QStringLiteral("已取消");
            return result;
        }
        // points whose value has no contour stay where they are
#pragma omp parallel for schedule(static) if (n > 64)
        for (int i = 0; i < n; ++i)
            target[i] = contours[i].empty() ? current[i] : closestPointOn(contours[i], current[i]);

        const Path next = fitAndApplyRigidTransform(current, target);
        double moved = 0;
        for (int i = 0; i < n; ++i) {
            const double dx = next[i].x() - current[i].x(), dy = next[i].y() - current[i].y();
            moved += dx * dx + dy * dy;
        }
        moved /= n;
        current = next;
        result.iterations = iter + 1;
        if (moved < options.tolerance) {
            result.converged = true;
            break;
        }
    }
    result.positions = current;
    return result;
}

} // namespace Nav
