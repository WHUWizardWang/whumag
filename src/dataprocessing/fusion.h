#ifndef FUSION_H
#define FUSION_H

#include "procgrid.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <vector>

// Fusion of several scattered data sets (survey lines of different platforms / accuracies) onto one
// grid: inverse-distance weighting with Shepard's window weights, each source weighted by 1 / sigma^2,
// after an optional per-source level (bias) adjustment and gross-error rejection.  No GUI dependency.
namespace Proc {

struct FusionSource
{
    QString name;
    std::vector<Sample> samples;
    double sigma = 1;   // standard error of the source (中误差)
};

struct FusionOptions
{
    // Interpolation window (full width along x / y, coordinate units).  Samples farther than half of
    // it (in the normalised elliptical distance) do not contribute.
    double windowX = 0.1;
    double windowY = 0.1;
    // Remove a constant offset of each source against the most accurate one (median over overlapping points).
    bool removeBias = true;
    // Reject samples whose leave-one-out residual is more than |outlierK| robust sigmas from the median.
    bool rejectOutliers = true;
    double outlierK = 3.5;
};

struct GridSpec
{
    double xMin = 0, xMax = 0, dx = 0;
    double yMin = 0, yMax = 0, dy = 0;
};

struct SourceReport
{
    QString name;
    double sigma = 1;
    int samples = 0;
    int rejected = 0;
    bool reference = false;       // the source the others were levelled to
    double bias = std::numeric_limits<double>::quiet_NaN();   // subtracted offset (NaN: not estimated)
    int biasPairs = 0;
    double crossValidationRms = std::numeric_limits<double>::quiet_NaN();   // leave-one-out residual RMS
};

struct FusionResult
{
    QString error;
    Grid grid;   // NaN where no sample is within the window
    std::vector<SourceReport> sources;
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
};

FusionResult fuse(std::vector<FusionSource> sources, const GridSpec &spec, const FusionOptions &options);

// ---------------------------------------------------------------- file-level job (used by the GUI)
struct FusionJob
{
    QStringList files;        // x y value per line
    QVector<double> sigmas;   // one per file
    GridSpec grid;
    FusionOptions options;
    QString outputFile;
};

struct FusionOutcome
{
    QString error;
    QStringList log;
    int writtenNodes = 0;
    bool ok() const { return error.isEmpty(); }
};

FusionOutcome runFusion(const FusionJob &job);

} // namespace Proc

#endif // FUSION_H
