#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

struct StatsResult
{
    double max;
    double min;
    double mean;
    double stddev;
};

inline StatsResult computeStats(const std::vector<double>& v)
{
    StatsResult s;
    s.max = *std::max_element(v.begin(), v.end());
    s.min = *std::min_element(v.begin(), v.end());
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    s.mean = sum / v.size();
    double sumSqDiff = std::inner_product(v.begin(), v.end(), v.begin(), 0.0,
        [](double acc, double diff) { return acc + diff * diff; },
        [](double a, double b) { return a + b; });
    s.stddev = std::sqrt(sumSqDiff / v.size());
    return s;
}
