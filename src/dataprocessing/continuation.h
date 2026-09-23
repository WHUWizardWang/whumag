#ifndef CONTINUATION_H
#define CONTINUATION_H

#include "procgrid.h"
#include "statsutil.h"

#include <QString>
#include <QStringList>

#include <vector>

// Upward / downward continuation of a gridded potential field in the wavenumber domain.
// Downward continuation amplifies short wavelengths exponentially, so it is regularised; the
// regularisation parameter can be chosen with the L-curve.  No GUI dependency.
namespace Proc {

enum class DownwardMethod
{
    Tikhonov = 0,        // F = R / (R^2 + alpha)                         parameter: alpha
    IntegralIteration,   // F = (1 - (1 - R)^n) / R                        parameter: n
    Landweber,           // F = (1 - (1 - lambda R^2)^n) / R, lambda 0.95  parameter: n
    IteratedTikhonov,    // F = (1 - (alpha / (R^2 + alpha))^n) / R, n 13  parameter: alpha
};

QString downwardMethodName(DownwardMethod method);
bool parameterIsIterationCount(DownwardMethod method);   // n (integer) rather than alpha
double defaultParameter(DownwardMethod method);           // what the old code used

// One point of the L-curve: the parameter, the data misfit ||R f - g|| and the solution norm ||f||.
struct LCurvePoint
{
    double parameter = 0;
    double residualNorm = 0;
    double solutionNorm = 0;
};

struct LCurve
{
    DownwardMethod method = DownwardMethod::Tikhonov;
    std::vector<LCurvePoint> points;   // in the order the parameters were tried
    int corner = -1;                   // index of the chosen point (maximum curvature)
    bool isEmpty() const { return points.empty(); }
};

struct DownwardOptions
{
    DownwardMethod method = DownwardMethod::Tikhonov;
    bool autoParameter = true;   // choose the parameter at the corner of the L-curve
    double parameter = 0;        // used when autoParameter is false (alpha, or n)
};

struct ContinuationResult
{
    QString error;          // empty on success
    Grid grid;              // continued field; NaN where the input had no data
    LCurve lcurve;          // downward continuation only
    double parameterUsed = 0;
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
};

// |height| > 0 in the unit of the grid spacing.
ContinuationResult continueUpward(const Grid &grid, double height);
ContinuationResult continueDownward(const Grid &grid, double height, const DownwardOptions &options);

// ---------------------------------------------------------------- file-level job (used by the GUI)
enum class ContinuationKind
{
    Upward,
    Downward,
    RoundTrip,   // accuracy check: continue up by h, back down by h, compare with the input
};

struct ContinuationJob
{
    ContinuationKind kind = ContinuationKind::Upward;
    QString inputFile;       // x y value per line
    QString outputFile;
    bool geographic = false; // x / y are longitude / latitude in degrees; height in km
    double dx = 0.5, dy = 0.5;   // grid spacing (degrees when geographic)
    double height = 0.5;         // km when geographic, otherwise the unit of x / y
    DownwardOptions downward;
};

struct ContinuationOutcome
{
    QString error;
    QStringList log;
    LCurve lcurve;
    double parameterUsed = 0;
    int writtenPoints = 0;
    // RoundTrip only: continued-back field minus input, over all valid nodes / the interior
    // (a border of 10 % of the grid size left out, where FFT edge effects are largest)
    StatsResult difference;
    StatsResult differenceInterior;
    bool ok() const { return error.isEmpty(); }
};

ContinuationOutcome runContinuation(const ContinuationJob &job);

} // namespace Proc

#endif // CONTINUATION_H
