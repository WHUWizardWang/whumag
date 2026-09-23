#ifndef NAVDATA_H
#define NAVDATA_H

#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>

#include <cmath>
#include <limits>

// Data shared by the matching-navigation algorithms (TERCOM, ICCP, SITAN): the background field,
// tracks, a regular grid built from the background, text-file I/O and the accuracy measure.
// Nothing in here depends on the GUI, so the algorithms can run in a worker thread.
namespace Nav {

// One sample of the background (reference) field.
struct MapPoint
{
    double x = 0;
    double y = 0;
    double value = 0;
};

// One sample of an INS track: position and the magnetic value measured there.
struct TrackPoint
{
    double x = 0;
    double y = 0;
    double magnetic = 0;
};

using Path = QVector<QPointF>;
using Track = QVector<TrackPoint>;

Path positionsOf(const Track &track);

// Replaces the positions of |track| with |positions| (same length), keeping the measured values.
Track withPositions(const Track &track, const Path &positions);

// Regular grid built from scattered background samples.  Cells no sample fell into are NaN.
class GridField
{
public:
    GridField() = default;

    // Bins |points| into cells of dx * dy starting at the smallest x / y.  Several samples in one
    // cell are averaged.  |warnings| receives notes about suspicious input (spacing mismatch, holes).
    static GridField fromPoints(const QVector<MapPoint> &points, double dx, double dy, QStringList *warnings = nullptr);

    bool isEmpty() const { return values_.isEmpty(); }
    int columns() const { return nx_; }   // along x
    int rows() const { return ny_; }      // along y
    double xMin() const { return x0_; }
    double yMin() const { return y0_; }
    double xMax() const { return x0_ + (nx_ - 1) * dx_; }
    double yMax() const { return y0_ + (ny_ - 1) * dy_; }
    double dx() const { return dx_; }
    double dy() const { return dy_; }

    // Value of node (ix, iy); NaN for a hole or an index outside the grid.
    double at(int ix, int iy) const
    {
        if (ix < 0 || iy < 0 || ix >= nx_ || iy >= ny_)
            return std::numeric_limits<double>::quiet_NaN();
        return values_[iy * nx_ + ix];
    }
    double nodeX(int ix) const { return x0_ + ix * dx_; }
    double nodeY(int iy) const { return y0_ + iy * dy_; }

    // Smallest / largest valid value (NaN when the grid has no valid cell).
    double minValue() const { return minValue_; }
    double maxValue() const { return maxValue_; }
    int validCells() const { return validCells_; }

private:
    int nx_ = 0, ny_ = 0;
    double x0_ = 0, y0_ = 0, dx_ = 1, dy_ = 1;
    double minValue_ = std::numeric_limits<double>::quiet_NaN();
    double maxValue_ = std::numeric_limits<double>::quiet_NaN();
    int validCells_ = 0;
    QVector<double> values_;   // row-major: values_[iy * nx_ + ix]
};

// Result of reading a text file: how many lines were used / skipped and why.
struct LoadReport
{
    QString error;             // non-empty when the file could not be used at all
    int usedLines = 0;
    int skippedLines = 0;
    QStringList skippedSamples;   // the first few skipped lines, "第 N 行: ..."
    bool ok() const { return error.isEmpty(); }
    QString summary() const;   // one line for the log
};

// Text formats: one record per line, fields separated by spaces, tabs, commas or semicolons.
// Empty lines, lines starting with '#' or "//" and header lines are skipped.
//   background field  x y value
//   INS track         x y magnetic   (further columns are ignored)
//   true path         x y
QVector<MapPoint> readMap(const QString &path, LoadReport *report);
Track readTrack(const QString &path, LoadReport *report);
Path readPath(const QString &path, LoadReport *report);

// Writes "x,y" per line, or "x,y,value" when |values| has one value per point.
bool writePath(const QString &path, const Path &points, const QVector<double> &values = {}, QString *error = nullptr);

// Root-mean-square distance between point i of |a| and point i of |b| over the common length.
// NaN when there is no common point.
double rmsError(const Path &a, const Path &b, int *comparedPoints = nullptr);

} // namespace Nav

#endif // NAVDATA_H
