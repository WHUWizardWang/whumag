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

    // 离线模式：用户选择“离线工作”后，所有需要数据库的功能都直接报告不可用，不再尝试连接。
    void setOffline(bool value) { offline_ = value; }
    bool isOffline() const { return offline_; }

    bool initConnection() {
        QMutexLocker locker(&mutex);

        // 防止多次连接
        if (db.isOpen()) {
            return true;
        }
        if (offline_) {
            return false;
        }

        QStringList drivers = QSqlDatabase::drivers();
//        foreach(QString str,drivers)
//            qDebug()<<str;
        QSqlDatabase db = QSqlDatabase::contains() ? QSqlDatabase::database(QLatin1String(QSqlDatabase::defaultConnection), false)
                                                   : QSqlDatabase::addDatabase("QPSQL");
        db.setHostName("localhost");
        db.setDatabaseName("whumag");
        db.setUserName("postgres");
        QString pwd = qEnvironmentVariable("WHUMAG_DB_PASSWORD");
        if (pwd.isEmpty()) {
            qDebug() << "WHUMAG_DB_PASSWORD not set; falling back to parameterized initConnection() is required.";
        }
        db.setPassword(pwd);
        db.setPort(5432);

        bool ok = db.open();
        if (!ok) {
            qDebug() << "Failed to connect to the database:" << db.lastError().text();
            return false;
        }

        this->db = db; // 保存连接实例
        return true;
    }

    bool initConnection(const QString& host,
                        int port,
                        const QString& user,
                        const QString& password)
    {
        QMutexLocker locker(&mutex);

        // 防止多次连接
        if (db.isOpen()) {
            return true;
        }
        if (offline_) {
            return false;
        }

        QStringList drivers = QSqlDatabase::drivers();
        //        foreach(QString str,drivers)
        //            qDebug()<<str;
        QSqlDatabase db = QSqlDatabase::contains() ? QSqlDatabase::database(QLatin1String(QSqlDatabase::defaultConnection), false)
                                                   : QSqlDatabase::addDatabase("QPSQL");
        db.setHostName(host);
        db.setDatabaseName("whumag");
        db.setUserName(user);
        db.setPassword(password);
        db.setPort(port);

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

    // Re-opens the connection using its already-configured parameters if it
    // isn't currently open -- e.g. the server dropped an idle connection
    // mid-session. isOpen() alone won't detect that case (it only reflects
    // the client-side handle), so callers should call this after a query
    // fails, not just check isOpen() up front. Cannot establish a
    // connection from scratch; that still requires initConnection().
    bool ensureConnected() {
        QMutexLocker locker(&mutex);
        if (db.isOpen())
            return true;
        if (!db.isValid())
            return false;
        return db.open();
    }

private:
    DatabaseManager() {}
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase db;
    QMutex mutex;
    bool offline_ = false;
};

#endif // DATABASEMANAGER_H
