#ifndef TIMECORRECTION_H
#define TIMECORRECTION_H

#include "statsutil.h"

#include <QDate>
#include <QString>
#include <QStringList>

// 数据通化: reduces magnetic survey values (total field, nT) observed on one date to another date by
// adding the change of the IGRF main field between the two dates at every point,
//     T(to) = T(from) + F_IGRF(to) - F_IGRF(from).
// No GUI dependency; the IGRF coefficients are read from ./COF/IGRF14_Windows.COF.
namespace Proc {

struct TimeCorrectionJob
{
    QString inputFile;        // longitude latitude value (degrees, nT) per line
    QString outputFile;
    QDate fromDate;           // survey date
    QDate toDate;             // reduction (target) date
    bool aboveGeoid = false;  // the height is above the geoid (M) rather than the ellipsoid (E)
    double heightKm = 0;      // observation height, km
};

struct TimeCorrectionOutcome
{
    QString error;
    QStringList log;
    int writtenPoints = 0;
    StatsResult correction;   // F_IGRF(to) - F_IGRF(from) over the points
    bool ok() const { return error.isEmpty(); }
};

// Decimal year of a date; 1 January is year + 0.
double decimalYear(const QDate &date);

TimeCorrectionOutcome runTimeCorrection(const TimeCorrectionJob &job);

} // namespace Proc

#endif // TIMECORRECTION_H
