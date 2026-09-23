#ifndef NAV_ICCP_H
#define NAV_ICCP_H

#include "navdata.h"

#include <QLineF>

#include <atomic>

namespace Nav {

struct IccpOptions
{
    int maxIterations = 200;
    // Stops when the mean squared movement of the points in one iteration, in grid cells squared,
    // drops below this (so it does not depend on the coordinate unit).
    double toleranceCells2 = 4e-6;
};

struct IccpResult
{
    QString error;          // empty on success
    Path positions;         // matched track
    int iterations = 0;
    bool converged = false;
    int pointsWithoutContour = 0;   // measured value outside the background range (point kept as is)
    bool ok() const { return error.isEmpty(); }
};

// ICCP (iterative closest contour point): each INS point belongs on the background contour line of
// its measured value.  Repeatedly finds the closest point on that contour for every INS point and
// applies the rigid transform (rotation + translation) that best moves the track onto them.
class IccpMatcher
{
public:
    explicit IccpMatcher(const GridField &grid);

    IccpResult match(const Track &track, const IccpOptions &options, const std::atomic_bool *cancel = nullptr) const;

    // Contour line of |level| as segments in map coordinates (marching squares; cells with a
    // missing corner are skipped).
    QVector<QLineF> contour(double level) const;

private:
    GridField grid_;
};

} // namespace Nav

#endif // NAV_ICCP_H
