#include "project.h"

Project::Project(QObject *parent) : QObject(parent)
{

}


QString Project::Name() {return name_; }
void Project::SetName(QString name) {name_ = name; }

QString Project::Path() {return path_; }
void Project::SetPath(QString path) { path_ = path; }

QString Project::Author() { return author_; }
void Project::SetAuthor(QString author) { author_ = author; }

QDateTime Project::BuildTime() { return build_time_; }
void Project::SetBuildTime(QDateTime build_time) { build_time_ = build_time; }

QDateTime Project::LastUpdateTime() { return last_update_time_; }
void Project::SetLastUpdateTime(QDateTime last_update_time) { last_update_time_ = last_update_time; }

int Project::Status() { return status_; }
void Project::SetStatus(int status) { status_ = status; }

QString Project::ToString() {
    QString proj_str = "";
    proj_str += "Name: " + name_ + "\r\n";
    proj_str += "Path: " + path_ + "\r\n";
    proj_str += "Author: " + author_ + "\r\n";
    proj_str += "Build Time: " + build_time_.toString("yyyy-MM-dd hh:mm:ss") + "\r\n";
    proj_str += "Last Update Time: " + last_update_time_.toString("yyyy-MM-dd hh:mm:ss") + "\r\n";
    proj_str += "Status: " + QString::number(status_) + "\r\n";
    return proj_str;
}

bool Project::LoadFromFile(const QString& path){
    QFile proj_file(path);
    if (proj_file.open(QIODevice::ReadOnly)){
        QTextStream stream(&proj_file);
        while(!stream.atEnd()){
            QString line_str = stream.readLine();
            QStringList str_list = line_str.split(" ");
            if(str_list.size()<2){
                return false;
            }
            if (str_list[0] == "Name:") name_ = str_list[1];
            else if (str_list[0] == "Path:") path_ = str_list[1];
            else if (str_list[0] == "Author:") author_ = str_list[1];
            else if (str_list[0] == "Build") build_time_ = QDateTime::fromString(str_list[2] + " " + str_list[3], "yyyy-MM-dd hh:mm:ss");
            else if (str_list[0] == "Last") last_update_time_ = QDateTime::fromString(str_list[3] + " " + str_list[4], "yyyy-MM-dd hh:mm:ss");
            else if (str_list[0] == "Status:") status_ = str_list[1].toInt();
        }
    } else {
        return false;
    }
    return true;
}
