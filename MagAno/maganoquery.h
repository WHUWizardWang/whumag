#ifndef MAGANOQUERY_H
#define MAGANOQUERY_H

#include <QVector>
#include "OmgValidator.h"
#include "database/databasemanager.h"
#include <QtMath>
#include <QDebug>

class MagAnoQuery
{
public:
    MagAnoQuery();

    static int omg_emag2(QVector<AnoPoint> &ano_pnts)
    {
        double lon_step = 360.0/10800.0;
        double lat_step = 180.0/5400.0;
        if (DatabaseManager::instance().initConnection())
        {
            qDebug() << "Database connection successful!";
            QSqlDatabase db = DatabaseManager::instance().getDatabase();
            QSqlQuery query(db);
            for (int i = 0; i < ano_pnts.size(); ++i)
            {
                int ix; // 输入为-180-180，转到0-360
                if (ano_pnts[i].y<0)
                    ix = round((ano_pnts[i].y+360.0)/lon_step);
                else
                    ix = round(ano_pnts[i].y/lon_step);
                int jy = round((90-ano_pnts[i].x)/lat_step);
                QString str = QString("select * from emag2_v3_20170530 where index_ij = %1;").arg(ix*5400+jy);
                query.prepare(str);
                query.exec();
                if(query.next())
                {
                    ano_pnts[i].y = query.value(2).toDouble();
                    ano_pnts[i].x = query.value(3).toDouble();
                    ano_pnts[i].z = query.value(4).toDouble();
                }
            }
        }
        return 0;
    }

    static int omg_mamea(QVector<AnoPoint> &ano_pnts)
    {
        double lon_step = (160.0-93.0)/2011.0;
        double lat_step = (46.0+12.0)/1741.0;
        if (DatabaseManager::instance().initConnection())
        {
            qDebug() << "Database connection successful!";
            QSqlDatabase db = DatabaseManager::instance().getDatabase();
            QSqlQuery query(db);
            for (int i = 0; i < ano_pnts.size(); ++i)
            {
                int ix = round((ano_pnts[i].y-93.0)/lon_step);
                int jy = round((ano_pnts[i].x+12.0)/lat_step);
                QString str = QString("select * from mamea where index_ij = %1 ;").arg(ix*1741+jy);
                query.prepare(str);
                query.exec();
                if(query.next())
                {
                    // x-lat, y-lon
                    ano_pnts[i].y = query.value(0).toDouble();
                    ano_pnts[i].x = query.value(1).toDouble();
                    ano_pnts[i].z = query.value(2).toDouble();
                }
            }
        }

        return 0;
    }
};

#endif // MAGANOQUERY_H
