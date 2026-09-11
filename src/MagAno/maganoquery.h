#ifndef MAGANOQUERY_H
#define MAGANOQUERY_H

#include <QVector>
#include "OmgValidator.h"
#include "database/databasemanager.h"
#include <QtMath>
#include <QDebug>
#include <functional>

class MagAnoQuery
{
private:
    // Returns 0 on success (queries were actually run -- individual points
    // with no matching DB row are expected/normal and just keep z=0, that's
    // not a failure), or -1 if the database connection itself failed, in
    // which case NO point was queried at all.
    //
    // Previously this always `return 0`, even when initConnection() failed
    // -- the caller (AnoQueryForm::on_pushButton_3_clicked) treated that as
    // full success and drew a heatmap/contour from it regardless. Combined
    // with AnoPoint's x/y/z having no default member initializers at the
    // time, a failed connection (e.g. WHUMAG_DB_PASSWORD not set, or the DB
    // simply not running) meant every point kept whatever uninitialized
    // stack garbage it started with -- which, since freshly-committed OS
    // memory pages are often zero-filled, frequently rendered as a blank/
    // empty plot instead of a visible error. AnoPoint now default-
    // initializes to 0 regardless, but the caller still needs to know the
    // connection failed so it can tell the user instead of silently
    // "succeeding" with an empty result.
    static int omg_query_impl(QVector<AnoPoint> &ano_pnts,
                               const QString &tableName,
                               const std::function<int(double x, double y)> &indexCalc,
                               const std::function<void(AnoPoint&, QSqlQuery&)> &assign)
    {
        if (!DatabaseManager::instance().initConnection())
        {
            qWarning() << "MagAnoQuery: database connection failed, aborting query against" << tableName;
            return -1;
        }

        qDebug() << "Database connection successful!";
        QSqlDatabase db = DatabaseManager::instance().getDatabase();
        QSqlQuery query(db);
        for (int i = 0; i < ano_pnts.size(); ++i)
        {
            ano_pnts[i].z = 0.0;
            int idx = indexCalc(ano_pnts[i].x, ano_pnts[i].y);
            QString str = QString("select * from %1 where index_ij = %2;").arg(tableName).arg(idx);
            query.prepare(str);
            query.exec();
            if (query.next())
                assign(ano_pnts[i], query);
        }
        return 0;
    }

public:
    static int omg_emag2(QVector<AnoPoint> &ano_pnts)
    {
        double lon_step = 360.0/10800.0;
        double lat_step = 180.0/5400.0;
        return omg_query_impl(ano_pnts, "emag2_v3_20170530",
            [lon_step, lat_step](double x, double y) {
                int ix; // 输入为-180-180，转到0-360
                if (y<0)
                    ix = round((y+360.0)/lon_step);
                else
                    ix = round(y/lon_step);
                int jy = round((90-x)/lat_step);
                return ix*5400+jy;
            },
            [](AnoPoint &p, QSqlQuery &query) {
                p.y = query.value(2).toDouble();
                p.x = query.value(3).toDouble();
                p.z = query.value(4).toDouble();
            });
    }

    static int omg_mamea(QVector<AnoPoint> &ano_pnts)
    {
        double lon_step = (160.0-93.0)/2011.0;
        double lat_step = (46.0+12.0)/1741.0;
        return omg_query_impl(ano_pnts, "mamea",
            [lon_step, lat_step](double x, double y) {
                int ix = round((y-93.0)/lon_step);
                int jy = round((x+12.0)/lat_step);
                return ix*1741+jy;
            },
            [](AnoPoint &p, QSqlQuery &query) {
                // x-lat, y-lon
                p.y = query.value(0).toDouble();
                p.x = query.value(1).toDouble();
                p.z = query.value(2).toDouble();
            });
    }
};

#endif // MAGANOQUERY_H
