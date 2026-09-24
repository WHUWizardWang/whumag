#ifndef BINARYTABLES_H
#define BINARYTABLES_H

#include <QString>
#include <QStringList>

#include <vector>

// Readers that turn self-describing binary data files into a table of numeric columns, without any
// external library:
//   - NASA CDF 3.x (.cdf; Swarm / CHAMP satellite data, ImagCDF observatory data, CDAWeb), with
//     GZIP- or RLE-compressed variables or files.  Every record-varying numeric variable becomes a
//     column (a variable with several values per record, e.g. B_NEC[3], becomes several columns);
//   - netCDF classic (CDF-1, CDF-2 64-bit offset and CDF-5; .nc / .cdf / .grd, e.g. GMT grids).  A
//     2-D grid z(y, x) becomes x, y, z columns; otherwise the 1-D variables along the longest
//     dimension become the columns.
// netCDF-4 files are HDF5 files and are not supported (they are reported as such).
namespace DataIO {

struct NumericColumn
{
    enum Kind { Number, EpochMs, Tt2000Ns };   // EpochMs: CDF EPOCH (ms since 0000-01-01); Tt2000Ns: CDF TT2000
    QString name;
    QString unit;
    Kind kind = Number;
    std::vector<double> values;   // NaN for fill values / missing records
};

struct NumericTable
{
    QString error;
    QString format;                       // "NASA CDF 3.9", "netCDF classic (CDF-2)", ...
    std::vector<NumericColumn> columns;   // all of the same length
    QStringList notes;
    size_t rowCount() const { return columns.empty() ? 0 : columns.front().values.size(); }
    bool ok() const { return error.isEmpty(); }
};

enum class BinaryKind { None, NasaCdf, NetCdf, Hdf5 };

// Looks at the first bytes of the file.
BinaryKind binaryKind(const QString &path);

NumericTable readNasaCdf(const QString &path);
NumericTable readNetCdf(const QString &path);

// Formats one value of a column for display (dates for the time kinds).
QString formatValue(const NumericColumn &column, double value);

} // namespace DataIO

#endif // BINARYTABLES_H
