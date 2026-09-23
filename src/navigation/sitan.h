#ifndef NAV_SITAN_H
#define NAV_SITAN_H

#include "navdata.h"

#include <atomic>

namespace Nav {

struct SitanOptions
{
    // Defaults chosen on the sample data and two synthetic sets (SITAN refines a TERCOM result).
    // Position noise is given in grid cells, so it does not depend on the coordinate unit.
    double initialPositionSigmaCells = 1.0;   // uncertainty of the input track at its start
    double processNoiseCells = 0.1;           // growth of the position error per step
    double measurementSigma = 4.0;       // magnetic measurement + map / interpolation error (field units)
    double innovationClamp = 3.0;        // innovations larger than this many sigma are clipped (outliers)
    int fitHalfWidth = 2;                // local plane fit over (2 * half + 1)^2 grid nodes
};

struct SitanResult
{
    QString error;          // empty on success
    Path positions;         // corrected track, one point per input point
    int skippedUpdates = 0; // steps without a usable local gradient (near holes / outside the map)
    bool ok() const { return error.isEmpty(); }
};

// SITAN (Sandia inertial terrain-aided navigation) on a magnetic background: an extended Kalman
// filter estimates the position offset of the input track.  At each point the background is
// linearised by a local plane fit; the difference between the measured value and the plane value
// at the current estimate corrects the offset along the gradient.
class SitanMatcher
{
public:
    explicit SitanMatcher(const GridField &grid);

    SitanResult match(const Track &track, const SitanOptions &options, const std::atomic_bool *cancel = nullptr) const;

private:
    // Plane through the nodes around (x, y): value and gradient at (x, y).  False when there are
    // not enough valid nodes nearby.
    bool linearise(double x, double y, int halfWidth, double *value, double *gradX, double *gradY) const;

    GridField grid_;
};

} // namespace Nav

#endif // NAV_SITAN_H
