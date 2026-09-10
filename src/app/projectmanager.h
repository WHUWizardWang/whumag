#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QFile>
#include "geomagnetismproject.h"

using GMP = GeoMagnetismProject;

class ProjectManager: public QObject
{
    Q_OBJECT
public:
    explicit ProjectManager(QObject *parent = nullptr);
    ~ProjectManager();

    static bool Load(GMP& gmproj, const QString& path);
    static void Create(GMP& gmproj);
    static void Save(GMP& gmproj);
};

#endif // PROJECTMANAGER_H
