#ifndef NAV_TERCOM_H
#define NAV_TERCOM_H

#include "navdata.h"

#include <atomic>
#include <memory>

namespace Nav {

struct TercomOptions
{
    // Start positions tried: every background sample within this distance of the INS start.
    double searchRadius = 9.0;
    // Map value at an arbitrary position: inverse-distance weighting of the nearest
    // |idwNeighbours| samples within |idwRadius| (the nearest sample when none is that close).
    double idwRadius = 9.0;
    int idwNeighbours = 10;

    // Heading search (on top of the translation search).  Off: translation only.
    bool searchRotation = true;
    double maxRotationDeg = 10.0;
    double coarseStepDeg = 0.5;
    double fineStepDeg = 0.05;
    int refinedCandidates = 8;    // the best coarse candidates that get the fine search
    int maxRefineRounds = 50;
};

struct TercomResult
{
    QString error;             // empty on success
    Path positions;            // matched track
    QVector<double> mapValues; // background value along the matched track
    double msd = std::numeric_limits<double>::quiet_NaN();   // mean squared magnetic difference
    double rotationDeg = 0;    // heading correction of the best match
    QPointF shift;             // translation of the track start
    int candidates = 0;        // start positions tried
    bool ok() const { return error.isEmpty(); }
};

// TERCOM (terrain contour matching) on a magnetic background: moves - and optionally rotates -
// the whole INS track so that the background values along it best agree with the measured values
// (least mean squared difference).
class TercomMatcher
{
public:
    explicit TercomMatcher(const QVector<MapPoint> &map);
    ~TercomMatcher();
    TercomMatcher(const TercomMatcher &) = delete;
    TercomMatcher &operator=(const TercomMatcher &) = delete;

    bool isEmpty() const;

    // Background value at (x, y) by inverse-distance weighting.
    double interpolate(double x, double y, const TercomOptions &options) const;

    // |cancel| (optional) is polled; a cancelled run returns an error result.
    TercomResult match(const Track &track, const TercomOptions &options, const std::atomic_bool *cancel = nullptr) const;

private:
    struct Index;
    std::unique_ptr<Index> index_;
};

} // namespace Nav

#endif // NAV_TERCOM_H
