#ifndef MAGANOQUERY_H
#define MAGANOQUERY_H

#include <QVector>
#include <QHash>
#include <QList>
#include "OmgValidator.h"
#include "database/databasemanager.h"
#include <QtMath>
#include <QDebug>
#include <QSqlRecord>
#include <functional>

class MagAnoQuery
{
private:
    // Batch size for "index_ij IN (?,...)" queries -- turns what used to be
    // one round trip per point (thousands, for a large grid query) into one
    // round trip per this many points.
    static constexpr int kBatchSize = 500;

    // Retries a failed exec() once, after attempting a single reconnect --
    // covers a connection that was fine at initConnection() time but was
    // silently dropped by the server mid-query (isOpen() alone doesn't
    // detect that). reconnectAttempted bounds this to one retry per call.
    static bool execWithReconnect(QSqlQuery &query, bool &reconnectAttempted)
    {
        if (query.exec())
            return true;
        qWarning() << "MagAnoQuery: query failed:" << query.lastError().text();
        if (reconnectAttempted)
            return false;
        reconnectAttempted = true;
        if (!DatabaseManager::instance().ensureConnected())
            return false;
        qWarning() << "MagAnoQuery: reconnected to database, retrying batch once";
        return query.exec();
    }

    // Runs indexCalc/assign for each point using a small number of batched
    // queries instead of one query per point. Return value: 0 = every batch
    // executed (a point with no matching row just keeps z=0, that's normal);
    // -1 = could not connect to the database at all, nothing was queried;
    // -2 = connected fine but at least one batch failed mid-query (e.g. the
    // connection dropped) -- some points may still be filled, but the
    // result should be treated as incomplete, not "no data here".
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

        QSqlDatabase db = DatabaseManager::instance().getDatabase();

        // Several query points can round to the same DB grid cell, so map
        // each index back to every point that needs it rather than assuming
        // a 1:1 correspondence.
        QHash<int, QVector<int>> idxToPoints;
        idxToPoints.reserve(ano_pnts.size());
        for (int i = 0; i < ano_pnts.size(); ++i)
        {
            ano_pnts[i].z = 0.0;
            idxToPoints[indexCalc(ano_pnts[i].x, ano_pnts[i].y)].append(i);
        }
        QList<int> allIdx = idxToPoints.keys();
        if (allIdx.isEmpty())
            return 0;

        bool anyBatchFailed = false;
        bool reconnectAttempted = false;
        bool inTransaction = db.transaction();
        QSqlQuery query(db);
        for (int offset = 0; offset < allIdx.size(); offset += kBatchSize)
        {
            QList<int> batch = allIdx.mid(offset, kBatchSize);
            QString placeholders = QString("?,").repeated(batch.size());
            placeholders.chop(1);
            // "*, index_ij AS ..." keeps every original column (and its
            // position) exactly as the assign() lambdas below expect, while
            // adding one extra trailing column we use to map each returned
            // row back to the point(s) that asked for it.
            query.prepare(QString("select *, index_ij as __batch_index_ij from %1 where index_ij in (%2)")
                              .arg(tableName, placeholders));
            for (int idx : batch)
                query.addBindValue(idx);

            if (!execWithReconnect(query, reconnectAttempted))
            {
                anyBatchFailed = true;
                continue;
            }

            while (query.next())
            {
                int idxColumn = query.record().count() - 1;
                int returnedIdx = query.value(idxColumn).toInt();
                for (int pointIndex : idxToPoints.value(returnedIdx))
                    assign(ano_pnts[pointIndex], query);
            }
        }
        if (inTransaction)
            db.commit();

        return anyBatchFailed ? -2 : 0;
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
