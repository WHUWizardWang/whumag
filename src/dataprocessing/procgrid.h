#ifndef PROCGRID_H
#define PROCGRID_H

#include <QStringList>

#include <cmath>
#include <limits>
#include <vector>

// Regular grids for the data-processing steps (continuation, fusion).  No GUI dependency.
namespace Proc {

struct Sample
{
    double x = 0;
    double y = 0;
    double v = 0;
};

// rows along y, columns along x; v[r * cols + c]; NaN = no data at that node
struct Grid
{
    int rows = 0;
    int cols = 0;
    double x0 = 0, y0 = 0;   // position of node (0, 0)
    double dx = 1, dy = 1;
    std::vector<double> v;

    bool empty() const { return rows <= 0 || cols <= 0; }
    double &at(int r, int c) { return v[size_t(r) * cols + c]; }
    double at(int r, int c) const { return v[size_t(r) * cols + c]; }
    double nodeX(int c) const { return x0 + c * dx; }
    double nodeY(int r) const { return y0 + r * dy; }
    int validCount() const;
};

// Grid over the extent of |samples| with spacing dx * dy.
//  - samples that already lie on such a grid are copied to their nodes (averaged if repeated);
//  - scattered samples are interpolated by inverse distance weighting of the 8 nearest samples
//    within two cells.
// Nodes with no sample nearby stay NaN (they are never made up).  |notes| receives remarks for
// the log.  Returns an empty grid when the spacing is invalid or the grid would be too large.
Grid gridSamples(const std::vector<Sample> &samples, double dx, double dy, QStringList *notes = nullptr);

// Valid nodes of |grid| as samples.
std::vector<Sample> gridToSamples(const Grid &grid);

// Copy of |grid| with the NaN nodes filled smoothly (Laplace interpolation from the valid nodes),
// e.g. before an FFT.  Valid nodes are unchanged.
Grid filledGrid(const Grid &grid);

// Geographic <-> local plane (metres) conversion around a reference point (equirectangular,
// spherical earth).  Good for survey areas of a few hundred kilometres.
struct LocalPlane
{
    double lon0 = 0, lat0 = 0;
    double metresPerDegLon = 1, metresPerDegLat = 1;
    static LocalPlane around(const std::vector<Sample> &lonLat);
    Sample toPlane(const Sample &s) const { return {(s.x - lon0) * metresPerDegLon, (s.y - lat0) * metresPerDegLat, s.v}; }
    Sample toLonLat(const Sample &s) const { return {s.x / metresPerDegLon + lon0, s.y / metresPerDegLat + lat0, s.v}; }
};

} // namespace Proc

#endif // PROCGRID_H
