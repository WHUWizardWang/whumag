#include "sitan.h"

#include <eigen-3.4.0/Eigen/Dense>

#include <cmath>

namespace Nav {

SitanMatcher::SitanMatcher(const GridField &grid) : grid_(grid)
{
}

bool SitanMatcher::linearise(double x, double y, int halfWidth, double *value, double *gradX, double *gradY) const
{
    const int cx = int(std::lround((x - grid_.xMin()) / grid_.dx()));
    const int cy = int(std::lround((y - grid_.yMin()) / grid_.dy()));
    if (cx < -halfWidth || cy < -halfWidth || cx >= grid_.columns() + halfWidth || cy >= grid_.rows() + halfWidth)
        return false;   // well outside the map

    // least squares plane v = a + b * u + c * w through the valid nodes around (x, y), with
    // u = (node x - x) / dx and w = (node y - y) / dy (in cells, so the fit does not depend on units)
    Eigen::Matrix3d normal = Eigen::Matrix3d::Zero();
    Eigen::Vector3d rhs = Eigen::Vector3d::Zero();
    int used = 0;
    for (int i = cx - halfWidth; i <= cx + halfWidth; ++i) {
        for (int j = cy - halfWidth; j <= cy + halfWidth; ++j) {
            const double v = grid_.at(i, j);
            if (std::isnan(v))
                continue;   // hole or outside the grid
            const Eigen::Vector3d row(1.0, (grid_.nodeX(i) - x) / grid_.dx(), (grid_.nodeY(j) - y) / grid_.dy());
            normal += row * row.transpose();
            rhs += row * v;
            ++used;
        }
    }
    if (used < 4 || std::fabs(normal.determinant()) < 1e-9 * used * used * used)
        return false;   // too few nodes, or all on a line: no gradient
    const Eigen::Vector3d plane = normal.ldlt().solve(rhs);
    *value = plane(0);
    *gradX = plane(1) / grid_.dx();
    *gradY = plane(2) / grid_.dy();
    return std::isfinite(*value) && std::isfinite(*gradX) && std::isfinite(*gradY);
}

SitanResult SitanMatcher::match(const Track &track, const SitanOptions &options, const std::atomic_bool *cancel) const
{
    SitanResult result;
    if (grid_.isEmpty() || grid_.columns() < 2 || grid_.rows() < 2) {
        result.error = QStringLiteral("背景场格网为空或过小");
        return result;
    }
    if (track.isEmpty()) {
        result.error = QStringLiteral("输入航迹为空");
        return result;
    }

    // state: offset (dx, dy) to add to the input track
    Eigen::Vector2d offset = Eigen::Vector2d::Zero();
    Eigen::Matrix2d P = Eigen::Matrix2d::Identity() * (options.initialPositionSigma * options.initialPositionSigma);
    const Eigen::Matrix2d Q = Eigen::Matrix2d::Identity() * (options.processNoise * options.processNoise);
    const double R = options.measurementSigma * options.measurementSigma;

    result.positions.reserve(track.size());
    for (int k = 0; k < track.size(); ++k) {
        if (cancel && cancel->load(std::memory_order_relaxed)) {
            result.error = QStringLiteral("已取消");
            return result;
        }
        if (k > 0)
            P += Q;   // predict: the offset is a slowly varying random walk

        const double ex = track[k].x + offset(0), ey = track[k].y + offset(1);
        double predicted, gx, gy;
        if (linearise(ex, ey, options.fitHalfWidth, &predicted, &gx, &gy)) {
            const Eigen::RowVector2d H(gx, gy);
            const double S = (H * P * H.transpose())(0, 0) + R;
            double innovation = track[k].magnetic - predicted;
            const double limit = options.innovationClamp * std::sqrt(S);
            if (options.innovationClamp > 0 && std::fabs(innovation) > limit)
                innovation = std::copysign(limit, innovation);   // robust against outliers
            const Eigen::Vector2d K = P * H.transpose() / S;
            offset += K * innovation;
            P = (Eigen::Matrix2d::Identity() - K * H) * P;
        } else {
            ++result.skippedUpdates;
        }
        result.positions.append(QPointF(track[k].x + offset(0), track[k].y + offset(1)));
    }
    return result;
}

} // namespace Nav
