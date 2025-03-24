#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#pragma once
#include <QSqlDatabase>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>

class DatabaseManager {
public:
    static DatabaseManager& instance() {
        static DatabaseManager instance;
        return instance;
    }

    bool initConnection() {
        QMutexLocker locker(&mutex);

        // 防止多次连接
        if (db.isOpen()) {
            return true;
        }

        QStringList drivers = QSqlDatabase::drivers();
//        foreach(QString str,drivers)
//            qDebug()<<str;
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
        db.setHostName("localhost");
        db.setDatabaseName("whumag");
        db.setUserName("postgres");
        db.setPassword("010915");
        db.setPort(5432);

        bool ok = db.open();
        if (!ok) {
            qDebug() << "Failed to connect to the database:" << db.lastError().text();
            return false;
        }

        this->db = db; // 保存连接实例
        return true;
    }

    QSqlDatabase& getDatabase() {
        QMutexLocker locker(&mutex);
        return db;
    }

private:
    DatabaseManager() {}
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase db;
    QMutex mutex;
};

#endif // DATABASEMANAGER_H
