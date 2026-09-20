#ifndef BUILDPROJECTFORM_H
#define BUILDPROJECTFORM_H

#include <QWidget>
#include <QString>
#include <QFileDialog>
#include <QDir>
#include "geomagnetismproject.h"
#include "projectmanager.h"

namespace Ui {
class BuildProjectForm;
}

class BuildProjectForm : public QWidget
{
    Q_OBJECT

public:
    explicit BuildProjectForm(QWidget *parent = nullptr, QString path = QDir::currentPath());
    BuildProjectForm(GeoMagnetismProject *gmproj, QString path = QDir::currentPath());
    ~BuildProjectForm();

signals:
    void Created(GeoMagnetismProject *);

private slots:
    void on_btn_browser_clicked();

    void on_lineEdit_name_textChanged(const QString &arg1);

    void on_btn_cancel_clicked();

    void on_btn_ok_clicked();

private:
    void buildLayout();

    Ui::BuildProjectForm *ui;

    GeoMagnetismProject *geomag_proj_;
    QString project_path_;
    QString folder_;
};

#endif // BUILDPROJECTFORM_H
