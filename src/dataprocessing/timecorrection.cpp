#include "timecorrection.h"

#include "navigation/navdata.h"

#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTextStream>

#include <exception>
#include <new>
#include <vector>

// last: GeomagnetismHeader.h defines _POSIX_C_SOURCE empty, which breaks <time.h> if included before it
#include "wmm/igrf_point.h"

namespace Proc {

namespace {

// Range covered by IGRF-14: definitive / main-field models from 1900, secular variation predicted to 2030.
constexpr double kFirstYear = 1900.0;
constexpr double kLastYear = 2030.0;
constexpr double kLastDefinitive = 2025.0;

// omg_igrf keeps the coefficients in global arrays, so calls must not overlap.
QMutex &igrfMutex()
{
    static QMutex m;
    return m;
}

} // namespace

double decimalYear(const QDate &date)
{
    return date.year() + (date.dayOfYear() - 1) / double(date.daysInYear());
}

TimeCorrectionOutcome runTimeCorrection(const TimeCorrectionJob &job)
{
    TimeCorrectionOutcome out;
    try {
        if (!job.fromDate.isValid() || !job.toDate.isValid()) {
            out.error = QStringLiteral("日期无效");
            return out;
        }
        const double y0 = decimalYear(job.fromDate), y1 = decimalYear(job.toDate);
        for (double y : {y0, y1}) {
            if (y < kFirstYear || y > kLastYear) {
                out.error = QStringLiteral("日期 %1 超出 IGRF-14 的适用范围（%2—%3 年）")
                                .arg(y, 0, 'f', 2).arg(kFirstYear).arg(kLastYear);
                return out;
            }
        }
        if (y0 > kLastDefinitive || y1 > kLastDefinitive)
            out.log << QStringLiteral("注意：%1 年以后的 IGRF 为预测的长期变化，精度较低").arg(kLastDefinitive);
        if (job.heightKm < -10 || job.heightKm > 10000) {
            out.error = QStringLiteral("观测高度 %1 km 不合理（单位为千米）").arg(job.heightKm);
            return out;
        }

        Nav::LoadReport report;
        const QVector<Nav::MapPoint> pts = Nav::readMap(job.inputFile, &report);
        if (!report.ok()) {
            out.error = report.error;
            return out;
        }
        out.log << QStringLiteral("读入 %1：%2").arg(QFileInfo(job.inputFile).fileName(), report.summary());
        for (const Nav::MapPoint &p : pts) {
            if (p.x < -180 || p.x > 360 || p.y < -90 || p.y > 90) {
                out.error = QStringLiteral("坐标 (%1, %2) 不是经纬度：通化需要以度为单位的经度、纬度")
                                .arg(p.x).arg(p.y);
                return out;
            }
        }

        const int n = pts.size();
        std::vector<MAGtype_CoordGeodetic> coords(n);
        std::vector<MAGtype_Date> date0(n), date1(n);
        std::vector<MAGtype_GeoMagneticElements> f0(n), f1(n), err0(n), err1(n);
        for (int i = 0; i < n; ++i) {
            MAGtype_CoordGeodetic &c = coords[i];
            c.lambda = pts[i].x > 180 ? pts[i].x - 360 : pts[i].x;
            c.phi = pts[i].y;
            // omg_igrf only reads HeightAboveEllipsoid.  The geoid undulation (< 110 m) changes the
            // difference of the two epochs by far less than 0.01 nT, so a height above the geoid is used as is.
            c.HeightAboveEllipsoid = job.heightKm;
            c.HeightAboveGeoid = job.heightKm;
            c.UseGeoid = job.aboveGeoid ? 1 : 0;
            date0[i] = {job.fromDate.year(), job.fromDate.month(), job.fromDate.day(), y0};
            date1[i] = {job.toDate.year(), job.toDate.month(), job.toDate.day(), y1};
        }
        {
            QMutexLocker lock(&igrfMutex());
            if (omg_igrf(coords.data(), date0.data(), f0.data(), err0.data(), n) != 0
                || omg_igrf(coords.data(), date1.data(), f1.data(), err1.data(), n) != 0) {
                out.error = QStringLiteral("无法读取 IGRF 系数文件（程序目录下的 COF/IGRF14_Windows.COF）");
                return out;
            }
        }

        std::vector<double> corr(n);
        for (int i = 0; i < n; ++i)
            corr[i] = f1[i].F - f0[i].F;
        out.correction = computeStats(corr);
        if (out.correction.count != n) {
            out.error = QStringLiteral("IGRF 计算结果无效（%1 个点）").arg(n - out.correction.count);
            return out;
        }

        QSaveFile file(job.outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out.error = QStringLiteral("无法写入 %1：%2").arg(job.outputFile, file.errorString());
            return out;
        }
        QTextStream ts(&file);
        ts.setRealNumberNotation(QTextStream::FixedNotation);
        for (int i = 0; i < n; ++i) {
            ts.setRealNumberPrecision(8);
            ts << pts[i].x << ' ' << pts[i].y << ' ';
            ts.setRealNumberPrecision(4);
            ts << pts[i].value + corr[i] << '\n';
        }
        ts.flush();
        if (!file.commit()) {
            out.error = QStringLiteral("无法写入 %1：%2").arg(job.outputFile, file.errorString());
            return out;
        }
        out.writtenPoints = n;
        out.log << QStringLiteral("%1 → %2（高度 %3 km，%4）")
                       .arg(job.fromDate.toString(Qt::ISODate), job.toDate.toString(Qt::ISODate))
                       .arg(job.heightKm)
                       .arg(job.aboveGeoid ? QStringLiteral("海拔高") : QStringLiteral("椭球高"));
        out.log << QStringLiteral("通化改正量 F_IGRF(目标) − F_IGRF(测量)：平均 %1 nT，最小 %2，最大 %3")
                       .arg(out.correction.mean, 0, 'f', 3)
                       .arg(out.correction.min, 0, 'f', 3)
                       .arg(out.correction.max, 0, 'f', 3);
        out.log << QStringLiteral("写出 %1 个点到 %2").arg(n).arg(QFileInfo(job.outputFile).fileName());
    } catch (const std::bad_alloc &) {
        out.error = QStringLiteral("内存不足");
    } catch (const std::exception &e) {
        out.error = QStringLiteral("计算出错：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    return out;
}

} // namespace Proc
