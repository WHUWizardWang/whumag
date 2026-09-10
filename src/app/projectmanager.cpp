#include "projectmanager.h"

ProjectManager::ProjectManager(QObject *parent) : QObject(parent)
{

}

ProjectManager::~ProjectManager()
{

}

bool ProjectManager::Load(GMP &gmproj, const QString &path)
{
    gmproj.nameList_real.clear();
    gmproj.nameList_processed.clear();
    QDir dir_measured(path.left(path.lastIndexOf('/'))+"/Measured");
    QDir dir_processed(path.left(path.lastIndexOf('/'))+"/Processed");
    if (!dir_measured.exists())
    {
        qDebug() << "目录不存在：" << dir_measured.path();
        return 0;
    }
    if (!dir_processed.exists())
    {
        qDebug() << "目录不存在：" << dir_processed.path();
        return 0;
    }
    // 设置文件过滤器，只匹配.txt和.dat文件
    QStringList filters;
    filters << "*.txt" << "*.dat";
    dir_measured.setNameFilters(filters);
    dir_processed.setNameFilters(filters);
    // 获取所有匹配的文件信息列表
    QFileInfoList list_measured = dir_measured.entryInfoList(QDir::Files);
    QFileInfoList list_processed = dir_processed.entryInfoList(QDir::Files);
    // 遍历文件信息列表
    foreach (const QFileInfo &fileInfo, list_measured)
    {
        gmproj.nameList_real.append(fileInfo.fileName());
    }
    foreach (const QFileInfo &fileInfo, list_processed)
    {
        gmproj.nameList_processed.append(fileInfo.fileName());
    }
    return gmproj.LoadFromFile(path);
}

void ProjectManager::Create(GMP &gmproj)
{
    // Verify the directory path.
    QDir proj_dir;
    if (!proj_dir.exists(gmproj.Path()))
    {
        proj_dir.mkdir(gmproj.Path());
    }
    else
    {

    }

    // Create project file
    QString proj_full_path = gmproj.Path() + "/" + gmproj.Name() + ".proj";
    QFile proj_file(proj_full_path);
    proj_file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&proj_file);
    stream << gmproj.ToString();
    proj_file.close();

    // Create folders
    QString subdir;

    // Global Geomagnetism Model Folder
    subdir = gmproj.Path() + "/GlobalMag";
    if (!proj_dir.exists(subdir))
    {
        proj_dir.mkdir(subdir);
    }

    // Magnetic Anomily Folder
    subdir = gmproj.Path() + "/MagAno";
    if (!proj_dir.exists(subdir))
    {
        proj_dir.mkdir(subdir);
    }

    // Survey line Folder
    subdir = gmproj.Path() + "/Measured";
    if (!proj_dir.exists(subdir))
    {
        proj_dir.mkdir(subdir);
    }

    // Process Folder
    subdir = gmproj.Path() + "/Processed";
    if (!proj_dir.exists(subdir))
    {
        proj_dir.mkdir(subdir);
    }
}

// Save current project to hard disk
void ProjectManager::Save(GMP &gmproj)
{
    QString proj_full_path = gmproj.Path() + "/" + gmproj.Name() + ".proj";
    QFile proj_file(proj_full_path);
    if (!proj_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qDebug() << "无法保存工程文件：" << proj_full_path;
        return;
    }
    QTextStream stream(&proj_file);
    stream << gmproj.ToString();
    proj_file.close();
}

