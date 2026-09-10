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
    static int omg_query_impl(QVector<AnoPoint> &ano_pnts,
                               const QString &tableName,
                               const std::function<int(double x, double y)> &indexCalc,
                               const std::function<void(AnoPoint&, QSqlQuery&)> &assign)
    {
        if (DatabaseManager::instance().initConnection())
        {
            qDebug() << "Database connection successful!";
            QSqlDatabase db = DatabaseManager::instance().getDatabase();
            QSqlQuery query(db);
            for (int i = 0; i < ano_pnts.size(); ++i)
            {
                // Ensure z (and, for a failed lookup, x/y) never surface uninitialized
                // stack memory to the caller if this point has no matching DB row.
                ano_pnts[i].z = 0.0;
                int idx = indexCalc(ano_pnts[i].x, ano_pnts[i].y);
                QString str = QString("select * from %1 where index_ij = %2;").arg(tableName).arg(idx);
                query.prepare(str);
                query.exec();
                if (query.next())
                    assign(ano_pnts[i], query);
            }
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
