#include "geomagnetismproject.h"

GeoMagnetismProject::GeoMagnetismProject(Project *parent) : Project(parent)
{

}

GeoMagnetismProject::~GeoMagnetismProject(){
}

bool copyFile(const QString &sourceFile, const QString &destFile)
{
    if (!QFile::exists(sourceFile))
    {
        qDebug() << "源文件不存在：" << sourceFile;
        return false;
    }
    if (!QFile::copy(sourceFile, destFile))
    {
        qDebug() << "复制文件失败：" << sourceFile << " 到 " << destFile;
        return false;
    }
    qDebug() << "文件复制成功：" << sourceFile << " 到 " << destFile;
    return true;

}
