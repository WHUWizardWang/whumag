#include "geomagnetismproject.h"

GeoMagnetismProject::GeoMagnetismProject(Project *parent) : Project(parent)
{

}

GeoMagnetismProject::~GeoMagnetismProject(){
    DeletePointerInVector(vec_global_geomag_model_);
    DeletePointerInVector(vec_magnetic_anomaly_);
    DeletePointerInVector(vec_survey_data_);
    DeletePointerInVector(vec_background_map_);
}

template<typename T>
void GeoMagnetismProject::DeletePointerInVector(QVector<T*>& vec_ptr_t){
    foreach(auto ptr_v, vec_ptr_t){
        if(ptr_v!=nullptr){
            delete  ptr_v;
            ptr_v = nullptr;
        }
    }
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
