#ifndef NAV_NAVIGATIONRUNNER_H
#define NAV_NAVIGATIONRUNNER_H

#include "navdata.h"

#include <atomic>
#include <functional>

namespace Nav {

enum class Method
{
    Tercom = 0,   // TERCOM with heading search
    Iccp,         // ICCP on the raw INS track
    Sitan,        // TERCOM, then SITAN
    TercomIccp,   // translation-only TERCOM, then ICCP
    All,          // everything above, compared in one plot
};

QString methodTitle(Method method);         // "TERCOM", "TERCOM + ICCP", ...
QString methodDescription(Method method);   // one sentence for the form
QString methodKey(Method method);           // file-name friendly: "tercom", "all_methods", ...

struct Job
{
    Method method = Method::Tercom;
    QString mapFile;      // background field: x y value
    QString insFile;      // INS track: x y magnetic
    QString truthFile;    // true path: x y (optional, only used for the accuracy)
    QString outputDir;    // results are written here
    double dx = 0.5;      // background grid resolution
    double dy = 0.5;
    double searchRadius = 9.0;   // TERCOM: start positions within this distance of the INS start
};

struct MatchedTrack
{
    QString key;     // file / style key: "tercom", "iccp", "sitan", "tercom_plain", "tercom_iccp"
    QString label;   // shown in the legend and the log
    Path path;
    double rms = std::numeric_limits<double>::quiet_NaN();   // against the true path
    int comparedPoints = 0;
    QString file;    // where the track was written
};

struct Outcome
{
    QString error;          // set when nothing could be computed
    bool cancelled = false;
    GridField grid;
    Path ins;
    Path truth;
    QVector<MatchedTrack> tracks;
    QString accuracyFile;
    qint64 elapsedMs = 0;
    bool ok() const { return error.isEmpty() && !cancelled; }
};

// Called from the worker thread with one line for the log each time.
using ProgressFn = std::function<void(const QString &line)>;

// Loads the inputs, runs the chosen method(s), writes the matched tracks and an accuracy summary to
// |job.outputDir|.  No GUI calls: safe to run in a worker thread.  |cancel| is polled.
Outcome runJob(const Job &job, const std::atomic_bool *cancel = nullptr, const ProgressFn &progress = {});

} // namespace Nav

#endif // NAV_NAVIGATIONRUNNER_H
