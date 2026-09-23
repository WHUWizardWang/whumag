#include "continuation.h"

#include "fftw3.h"
#include "navigation/navdata.h"

#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <complex>
#include <exception>
#include <new>

namespace Proc {

QString downwardMethodName(DownwardMethod method)
{
    switch (method) {
    case DownwardMethod::Tikhonov: return QStringLiteral("Tikhonov 正则化法");
    case DownwardMethod::IntegralIteration: return QStringLiteral("积分迭代法");
    case DownwardMethod::Landweber: return QStringLiteral("Landweber 迭代法");
    case DownwardMethod::IteratedTikhonov: return QStringLiteral("迭代 Tikhonov 正则化法");
    }
    return QString();
}

bool parameterIsIterationCount(DownwardMethod method)
{
    return method == DownwardMethod::IntegralIteration || method == DownwardMethod::Landweber;
}

double defaultParameter(DownwardMethod method)
{
    return parameterIsIterationCount(method) ? 13.0 : 0.95;
}

namespace {

constexpr double kLandweberStep = 0.95;
constexpr int kIteratedTikhonovSteps = 13;

// FFTW plan creation / destruction is not thread-safe (execution is)
QMutex &fftwMutex()
{
    static QMutex mutex;
    return mutex;
}

int fftFriendlySize(int n)
{
    for (int m = std::max(n, 1);; ++m) {
        int k = m;
        for (int p : {2, 3, 5})
            while (k % p == 0)
                k /= p;
        if (k == 1)
            return m;
    }
}

// index |m| folded into [0, n) by even reflection (… 2 1 0 1 2 … n-2 n-1 n-2 …)
int reflect(int m, int n)
{
    if (n == 1)
        return 0;
    const int period = 2 * n - 2;
    m %= period;
    if (m < 0)
        m += period;
    return m < n ? m : period - m;
}

// Level added back after the FFT (only the mean is used, see forward()).
struct Plane
{
    double a = 0, b = 0, d = 0, cc = 0, rc = 0;
    double at(int r, int c) const { return a + b * (c - cc) + d * (r - rc); }
};

// Spectrum of the detrended, padded and tapered grid.
struct Spectrum
{
    int ny = 0, nx = 0, nxc = 0;   // padded size; nxc = nx / 2 + 1 (r2c layout)
    int offR = 0, offC = 0;        // position of the data inside the padded array
    Plane plane;
    std::vector<std::complex<double>> G;
    std::vector<double> k;         // |wavenumber| per bin
    std::vector<double> w;         // Parseval weight per bin (1 or 2 for the r2c half spectrum)
};

Spectrum forward(const Grid &grid)
{
    Spectrum s;
    const Grid filled = filledGrid(grid);
    // Only the mean is removed (and added back unchanged): it is the k = 0 part, which continuation
    // leaves unchanged anyway.  Removing a fitted plane instead would keep the part of the anomaly
    // that projects onto the plane from being continued (tested: 4x larger upward error).
    s.plane = Plane();
    s.plane.cc = 0.5 * (grid.cols - 1);
    s.plane.rc = 0.5 * (grid.rows - 1);
    {
        double sum = 0;
        int n = 0;
        for (double v : grid.v)
            if (!std::isnan(v)) {
                sum += v;
                ++n;
            }
        s.plane.a = n > 0 ? sum / n : 0;
    }

    const int padR = std::max(8, grid.rows / 4), padC = std::max(8, grid.cols / 4);
    s.ny = fftFriendlySize(grid.rows + 2 * padR);
    s.nx = fftFriendlySize(grid.cols + 2 * padC);
    s.nxc = s.nx / 2 + 1;
    s.offR = (s.ny - grid.rows) / 2;
    s.offC = (s.nx - grid.cols) / 2;
    const double widthR = std::max(1, std::max(s.offR, s.ny - s.offR - grid.rows));
    const double widthC = std::max(1, std::max(s.offC, s.nx - s.offC - grid.cols));

    // mirror the (detrended) data into the pad and taper it to zero towards the outer edge
    auto taper = [](double outside, double width) {
        return outside <= 0 ? 1.0 : 0.5 * (1.0 + std::cos(M_PI * std::min(outside / width, 1.0)));
    };
    double *in = fftw_alloc_real(size_t(s.ny) * s.nx);
    fftw_complex *out = fftw_alloc_complex(size_t(s.ny) * s.nxc);
    if (!in || !out) {
        fftw_free(in);
        fftw_free(out);
        throw std::bad_alloc();
    }
    for (int i = 0; i < s.ny; ++i) {
        const int r = reflect(i - s.offR, grid.rows);
        const double outR = i < s.offR ? s.offR - i : (i >= s.offR + grid.rows ? i - (s.offR + grid.rows - 1) : 0);
        const double tr = taper(outR, widthR);
        for (int j = 0; j < s.nx; ++j) {
            const int c = reflect(j - s.offC, grid.cols);
            const double outC = j < s.offC ? s.offC - j : (j >= s.offC + grid.cols ? j - (s.offC + grid.cols - 1) : 0);
            in[size_t(i) * s.nx + j] = (filled.at(r, c) - s.plane.at(r, c)) * tr * taper(outC, widthC);
        }
    }

    fftw_plan plan;
    {
        QMutexLocker lock(&fftwMutex());
        plan = fftw_plan_dft_r2c_2d(s.ny, s.nx, in, out, FFTW_ESTIMATE);
    }
    fftw_execute(plan);
    {
        QMutexLocker lock(&fftwMutex());
        fftw_destroy_plan(plan);
    }

    s.G.resize(size_t(s.ny) * s.nxc);
    s.k.resize(s.G.size());
    s.w.resize(s.G.size());
    for (int i = 0; i < s.ny; ++i) {
        const int fi = i <= s.ny / 2 ? i : i - s.ny;
        const double ky = 2.0 * M_PI * fi / (s.ny * grid.dy);   // rows run along y
        for (int j = 0; j < s.nxc; ++j) {
            const size_t b = size_t(i) * s.nxc + j;
            const double kx = 2.0 * M_PI * j / (s.nx * grid.dx);   // columns run along x
            s.G[b] = std::complex<double>(out[b][0], out[b][1]);
            s.k[b] = std::hypot(kx, ky);
            s.w[b] = (j == 0 || (s.nx % 2 == 0 && j == s.nx / 2)) ? 1.0 : 2.0;
        }
    }
    fftw_free(in);
    fftw_free(out);
    return s;
}

// Applies the real filter F (one value per bin) and returns the continued grid.
Grid inverse(const Spectrum &s, const std::vector<double> &F, const Grid &grid)
{
    fftw_complex *in = fftw_alloc_complex(size_t(s.ny) * s.nxc);
    double *out = fftw_alloc_real(size_t(s.ny) * s.nx);
    if (!in || !out) {
        fftw_free(in);
        fftw_free(out);
        throw std::bad_alloc();
    }
    for (size_t b = 0; b < s.G.size(); ++b) {
        in[b][0] = s.G[b].real() * F[b];
        in[b][1] = s.G[b].imag() * F[b];
    }
    fftw_plan plan;
    {
        QMutexLocker lock(&fftwMutex());
        plan = fftw_plan_dft_c2r_2d(s.ny, s.nx, in, out, FFTW_ESTIMATE);
    }
    fftw_execute(plan);
    {
        QMutexLocker lock(&fftwMutex());
        fftw_destroy_plan(plan);
    }

    Grid result = grid;
    const double norm = 1.0 / (double(s.ny) * s.nx);
    for (int r = 0; r < grid.rows; ++r)
        for (int c = 0; c < grid.cols; ++c) {
            if (std::isnan(grid.at(r, c)))
                continue;   // no data there: none in the result either
            result.at(r, c) = out[size_t(r + s.offR) * s.nx + (c + s.offC)] * norm + s.plane.at(r, c);
        }
    fftw_free(in);
    fftw_free(out);
    return result;
}

// Downward continuation filter for upward operator R = exp(-|k| h), R in [0, 1].
double downwardFilter(DownwardMethod method, double R, double p)
{
    switch (method) {
    case DownwardMethod::Tikhonov:
        return R / (R * R + p);
    case DownwardMethod::IntegralIteration:
        if (R <= 0)
            return p;   // limit R -> 0
        return -std::expm1(p * std::log1p(-std::min(R, 1.0))) / R;
    case DownwardMethod::Landweber:
        if (R <= 0)
            return 0;
        return -std::expm1(p * std::log1p(-kLandweberStep * R * R)) / R;
    case DownwardMethod::IteratedTikhonov:
        if (R <= 0)
            return 0;
        return -std::expm1(-kIteratedTikhonovSteps * std::log1p(R * R / p)) / R;
    }
    return 0;
}

std::vector<double> candidateParameters(DownwardMethod method)
{
    std::vector<double> ps;
    if (parameterIsIterationCount(method)) {
        for (double e = 0; e <= 4.0001; e += 0.05) {
            const double n = std::round(std::pow(10.0, e));
            if (ps.empty() || n != ps.back())
                ps.push_back(n);
        }
    } else {
        for (double e = -10; e <= 2.0001; e += 0.2)
            ps.push_back(std::pow(10.0, e));
    }
    return ps;
}

// Corner of the L-curve: the point of largest curvature (either direction) of
// (log residual, log norm), both axes scaled to [0, 1], ordered by residual.  Points whose residual
// exceeds half the largest one are left out: there the solution is certainly over-smoothed and the
// curve bends again as the residual saturates at the data norm.
// (Depending on h / grid spacing and the noise the curve is an "L" or a flipped one, so the corner can
// turn either way.  Tested against the exact answer on four synthetic sets: this rule was the only one
// of three that never failed; it lands within 1.2 - 4 times the best possible error, the old fixed
// parameters within 1.3 - 36 times.)
int lcurveCorner(const std::vector<LCurvePoint> &pts)
{
    struct P { double x, y; int index; };
    double maxResidual = 0;
    for (const LCurvePoint &p : pts)
        if (std::isfinite(p.residualNorm))
            maxResidual = std::max(maxResidual, p.residualNorm);
    std::vector<P> v;
    for (int i = 0; i < int(pts.size()); ++i)
        if (pts[i].residualNorm > 0 && pts[i].residualNorm < 0.5 * maxResidual && pts[i].solutionNorm > 0
            && std::isfinite(pts[i].solutionNorm))
            v.push_back({std::log10(pts[i].residualNorm), std::log10(pts[i].solutionNorm), i});
    if (v.size() < 3)
        return v.empty() ? -1 : v.front().index;
    std::sort(v.begin(), v.end(), [](const P &a, const P &b) { return a.x < b.x; });
    std::vector<P> u{v.front()};   // drop points that do not move
    for (size_t i = 1; i < v.size(); ++i)
        if (std::hypot(v[i].x - u.back().x, v[i].y - u.back().y) > 1e-9)
            u.push_back(v[i]);
    if (u.size() < 3)
        return u.front().index;

    double x0 = u.front().x, x1 = x0, y0 = u.front().y, y1 = y0;
    for (const P &p : u) {
        x0 = std::min(x0, p.x); x1 = std::max(x1, p.x);
        y0 = std::min(y0, p.y); y1 = std::max(y1, p.y);
    }
    const double sx = std::max(x1 - x0, 1e-12), sy = std::max(y1 - y0, 1e-12);
    for (P &p : u) {
        p.x = (p.x - x0) / sx;
        p.y = (p.y - y0) / sy;
    }

    int best = -1;
    double bestK = -1;
    for (size_t i = 1; i + 1 < u.size(); ++i) {
        const P &a = u[i - 1], &b = u[i], &c = u[i + 1];
        const double cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
        const double denom = std::hypot(b.x - a.x, b.y - a.y) * std::hypot(c.x - b.x, c.y - b.y) * std::hypot(c.x - a.x, c.y - a.y);
        if (denom <= 0)
            continue;
        const double kappa = std::fabs(2 * cross / denom);   // Menger curvature
        if (kappa > bestK) {
            bestK = kappa;
            best = int(i);
        }
    }
    return best < 0 ? u[u.size() / 2].index : u[best].index;
}

LCurve computeLCurve(const Spectrum &s, const std::vector<double> &R, DownwardMethod method)
{
    LCurve lc;
    lc.method = method;
    const std::vector<double> params = candidateParameters(method);
    lc.points.resize(params.size());
#pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < int(params.size()); ++i) {
        double rho = 0, eta = 0;
        for (size_t b = 0; b < s.G.size(); ++b) {
            const double g2 = std::norm(s.G[b]);
            const double F = downwardFilter(method, R[b], params[i]);
            const double misfit = R[b] * F - 1.0;
            rho += s.w[b] * misfit * misfit * g2;
            eta += s.w[b] * F * F * g2;
        }
        lc.points[i] = {params[i], std::sqrt(rho), std::sqrt(eta)};
    }
    lc.corner = lcurveCorner(lc.points);
    return lc;
}

QString formatParameter(DownwardMethod method, double p)
{
    return parameterIsIterationCount(method) ? QStringLiteral("迭代次数 n = %1").arg(qRound(p))
                                             : QStringLiteral("正则化参数 α = %1").arg(p, 0, 'g', 3);
}

} // namespace

ContinuationResult continueUpward(const Grid &grid, double height)
{
    ContinuationResult res;
    if (grid.empty() || grid.rows < 4 || grid.cols < 4 || grid.validCount() == 0) {
        res.error = QStringLiteral("格网过小或没有数据（至少需要 4 × 4）");
        return res;
    }
    if (!(height > 0)) {
        res.error = QStringLiteral("延拓高度必须大于 0");
        return res;
    }
    const Spectrum s = forward(grid);
    std::vector<double> F(s.k.size());
    for (size_t b = 0; b < F.size(); ++b)
        F[b] = std::exp(-s.k[b] * height);
    res.grid = inverse(s, F, grid);
    return res;
}

ContinuationResult continueDownward(const Grid &grid, double height, const DownwardOptions &options)
{
    ContinuationResult res;
    if (grid.empty() || grid.rows < 4 || grid.cols < 4 || grid.validCount() == 0) {
        res.error = QStringLiteral("格网过小或没有数据（至少需要 4 × 4）");
        return res;
    }
    if (!(height > 0)) {
        res.error = QStringLiteral("延拓高度必须大于 0");
        return res;
    }
    const double cell = std::max(grid.dx, grid.dy);
    if (height > 5 * cell)
        res.notes << QStringLiteral("向下延拓距离是格网间距的 %1 倍，结果对噪声非常敏感，请谨慎使用").arg(height / cell, 0, 'f', 1);

    const Spectrum s = forward(grid);
    std::vector<double> R(s.k.size());
    for (size_t b = 0; b < R.size(); ++b)
        R[b] = std::exp(-s.k[b] * height);

    double p = options.parameter;
    if (options.autoParameter) {
        res.lcurve = computeLCurve(s, R, options.method);
        if (res.lcurve.corner < 0) {
            p = defaultParameter(options.method);
            res.notes << QStringLiteral("L 曲线没有明显拐点，使用默认参数");
        } else {
            p = res.lcurve.points[res.lcurve.corner].parameter;
        }
        res.notes << QStringLiteral("L 曲线拐点：%1").arg(formatParameter(options.method, p));
    } else {
        if (!(p > 0)) {
            res.error = QStringLiteral("正则化参数必须大于 0");
            return res;
        }
        if (parameterIsIterationCount(options.method))
            p = std::max(1.0, std::round(p));
        res.notes << QStringLiteral("手动指定：%1").arg(formatParameter(options.method, p));
    }
    res.parameterUsed = p;

    std::vector<double> F(R.size());
    double maxGain = 0;
    for (size_t b = 0; b < F.size(); ++b) {
        F[b] = downwardFilter(options.method, R[b], p);
        maxGain = std::max(maxGain, std::fabs(F[b]));
    }
    res.notes << QStringLiteral("最大放大倍数 %1").arg(maxGain, 0, 'g', 4);
    res.grid = inverse(s, F, grid);
    return res;
}

// ---------------------------------------------------------------- file-level job
namespace {

StatsResult differenceStats(const Grid &a, const Grid &b, int border)
{
    std::vector<double> d;
    for (int r = border; r < a.rows - border; ++r)
        for (int c = border; c < a.cols - border; ++c) {
            const double x = a.at(r, c), y = b.at(r, c);
            if (!std::isnan(x) && !std::isnan(y))
                d.push_back(x - y);
        }
    return computeStats(d);
}

QString statsLine(const StatsResult &s)
{
    if (s.count == 0)
        return QStringLiteral("无可比较的节点");
    return QStringLiteral("均值 %1，标准差 %2，均方根 %3，最小 %4，最大 %5（%6 个节点）")
        .arg(s.mean, 0, 'f', 3).arg(s.stddev, 0, 'f', 3).arg(s.rms, 0, 'f', 3)
        .arg(s.min, 0, 'f', 3).arg(s.max, 0, 'f', 3).arg(s.count);
}

} // namespace

ContinuationOutcome runContinuation(const ContinuationJob &job)
{
    ContinuationOutcome out;
    try {
        Nav::LoadReport report;
        const QVector<Nav::MapPoint> points = Nav::readMap(job.inputFile, &report);
        if (!report.ok()) {
            out.error = report.error;
            return out;
        }
        out.log << QStringLiteral("读入 %1：%2").arg(QFileInfo(job.inputFile).fileName(), report.summary());
        if (!(job.dx > 0) || !(job.dy > 0)) {
            out.error = QStringLiteral("格网分辨率必须大于 0");
            return out;
        }
        if (!(job.height > 0)) {
            out.error = QStringLiteral("延拓高度必须大于 0");
            return out;
        }

        std::vector<Sample> samples;
        samples.reserve(points.size());
        for (const Nav::MapPoint &p : points)
            samples.push_back({p.x, p.y, p.value});

        // geographic data: work in a local plane in metres, height km -> m
        LocalPlane plane;
        double dx = job.dx, dy = job.dy, h = job.height;
        if (job.geographic) {
            for (const Sample &s : samples)
                if (s.x < -180 || s.x > 360 || s.y < -90 || s.y > 90) {
                    out.error = QStringLiteral("坐标超出经纬度范围，请取消“使用 BL 坐标”");
                    return out;
                }
            plane = LocalPlane::around(samples);
            for (Sample &s : samples)
                s = plane.toPlane(s);
            dx = job.dx * plane.metresPerDegLon;
            dy = job.dy * plane.metresPerDegLat;
            h = job.height * 1000.0;
            out.log << QStringLiteral("经纬度坐标：格网间距约 %1 m × %2 m，延拓高度 %3 m").arg(dx, 0, 'f', 1).arg(dy, 0, 'f', 1).arg(h, 0, 'f', 1);
        }

        QStringList notes;
        const Grid grid = gridSamples(samples, dx, dy, &notes);
        out.log << notes;
        if (grid.empty()) {
            out.error = QStringLiteral("无法建立格网，请检查分辨率");
            return out;
        }

        ContinuationResult result;
        switch (job.kind) {
        case ContinuationKind::Upward:
            result = continueUpward(grid, h);
            break;
        case ContinuationKind::Downward:
            out.log << QStringLiteral("向下延拓算子：%1").arg(downwardMethodName(job.downward.method));
            result = continueDownward(grid, h, job.downward);
            break;
        case ContinuationKind::RoundTrip: {
            const ContinuationResult up = continueUpward(grid, h);
            if (!up.ok()) {
                out.error = up.error;
                return out;
            }
            out.log << QStringLiteral("已向上延拓 %1，再用 %2 向下延拓回原高度").arg(job.height).arg(downwardMethodName(job.downward.method));
            result = continueDownward(up.grid, h, job.downward);
            if (result.ok()) {
                const int border = std::max(1, std::min(grid.rows, grid.cols) / 10);
                out.difference = differenceStats(result.grid, grid, 0);
                out.differenceInterior = differenceStats(result.grid, grid, border);
            }
            break;
        }
        }
        if (!result.ok()) {
            out.error = result.error;
            return out;
        }
        out.log << result.notes;
        out.lcurve = result.lcurve;
        out.parameterUsed = result.parameterUsed;
        if (job.kind == ContinuationKind::RoundTrip) {
            out.log << QStringLiteral("精度（延拓回来的值 − 原始值）：%1").arg(statsLine(out.difference));
            out.log << QStringLiteral("精度（去掉 10% 边缘后）：%1").arg(statsLine(out.differenceInterior));
        }

        // write valid nodes (in the input's coordinates)
        QSaveFile file(job.outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out.error = QStringLiteral("无法写入 %1：%2").arg(job.outputFile, file.errorString());
            return out;
        }
        QTextStream ts(&file);
        ts.setRealNumberNotation(QTextStream::FixedNotation);
        for (const Sample &s0 : gridToSamples(result.grid)) {
            const Sample s = job.geographic ? plane.toLonLat(s0) : s0;
            ts.setRealNumberPrecision(job.geographic ? 8 : 6);
            ts << s.x << ' ' << s.y << ' ';
            ts.setRealNumberPrecision(4);
            ts << s.v << '\n';
            ++out.writtenPoints;
        }
        ts.flush();
        if (!file.commit()) {
            out.error = QStringLiteral("无法写入 %1：%2").arg(job.outputFile, file.errorString());
            return out;
        }
        out.log << QStringLiteral("写出 %1 个节点到 %2").arg(out.writtenPoints).arg(QFileInfo(job.outputFile).fileName());
    } catch (const std::bad_alloc &) {
        out.error = QStringLiteral("内存不足，请增大格网间距");
    } catch (const std::exception &e) {
        out.error = QStringLiteral("计算出错：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    return out;
}

} // namespace Proc
