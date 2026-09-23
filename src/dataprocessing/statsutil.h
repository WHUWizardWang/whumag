#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

struct StatsResult
{
    double max = std::numeric_limits<double>::quiet_NaN();
    double min = std::numeric_limits<double>::quiet_NaN();
    double mean = std::numeric_limits<double>::quiet_NaN();
    double stddev = std::numeric_limits<double>::quiet_NaN();   // population standard deviation about the mean
    double rms = std::numeric_limits<double>::quiet_NaN();      // root mean square (about 0)
    int count = 0;                                               // finite values used
};

// Statistics of the finite values of |v| (NaN / inf are skipped; all fields NaN when none is left).
// (The previous version computed "stddev" as sqrt(mean((2x)^2)) -- twice the RMS, without
// subtracting the mean -- and crashed on an empty vector.)
inline StatsResult computeStats(const std::vector<double> &v)
{
    StatsResult s;
    double sum = 0, sumSq = 0;
    for (double x : v) {
        if (!std::isfinite(x))
            continue;
        if (s.count == 0) {
            s.min = s.max = x;
        } else {
            s.min = std::min(s.min, x);
            s.max = std::max(s.max, x);
        }
        sum += x;
        sumSq += x * x;
        ++s.count;
    }
    if (s.count == 0)
        return s;
    s.mean = sum / s.count;
    s.rms = std::sqrt(sumSq / s.count);
    double var = 0;
    for (double x : v)
        if (std::isfinite(x))
            var += (x - s.mean) * (x - s.mean);
    s.stddev = std::sqrt(var / s.count);
    return s;
}
