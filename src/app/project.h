#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QBitArray>
#include <QFile>
#include <QTextStream>

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject *parent = nullptr);

    /*
     * Declare of GET and SET method of properties.
     */
    QString Name();
    void SetName(QString name);

    QString Path();
    void SetPath(QString path);

    QString Author();
    void SetAuthor(QString author);

    QDateTime BuildTime();
    void SetBuildTime(QDateTime build_time);

    QDateTime LastUpdateTime();
    void SetLastUpdateTime(QDateTime last_update_time);

    int Status();
    void SetStatus(int status);

    /*
     * Other public functions.
     */
    QString ToString();
    bool LoadFromFile(const QString& path);

    /*
     * Define constants of project status
     */
    const static int EMPTY = 1;

protected:
    QString name_;                  // Project name.
    QString path_;                  // The path of project directory.
    QString author_;                // The author name.
    QDateTime build_time_;          // The build time of this project.
    QDateTime last_update_time_;    // The last update time of this project.
    int status_;              // Indicating the status of this project.

signals:

};

#endif // PROJECT_H
