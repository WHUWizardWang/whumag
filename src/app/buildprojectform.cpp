#include "buildprojectform.h"
#include "ui_buildprojectform.h"

BuildProjectForm::BuildProjectForm(QWidget *parent, QString path) :
    QWidget(parent),
    ui(new Ui::BuildProjectForm),
    geomag_proj_(nullptr)
{
    ui->setupUi(this);
    ui->lineEdit_path->setText(path);
    folder_ = path;
}

BuildProjectForm::BuildProjectForm(GeoMagnetismProject *gmproj, QString path) :
    ui(new Ui::BuildProjectForm)
{
    ui->setupUi(this);
    ui->lineEdit_path->setText(path);
    folder_ = path;
    geomag_proj_ = gmproj;
}

BuildProjectForm::~BuildProjectForm()
{
    delete ui;
}


void BuildProjectForm::on_btn_browser_clicked()
{
    folder_ = QFileDialog::getExistingDirectory(this, tr("选择工程路径"), "./", QFileDialog::ShowDirsOnly);
    project_path_ = folder_ + "/" + ui->lineEdit_name->text();
    ui->lineEdit_path->setText(project_path_);
}


void BuildProjectForm::on_lineEdit_name_textChanged(const QString &arg1)
{
    project_path_ = folder_ + "/" + arg1;
    ui->lineEdit_path->setText(project_path_);
}

void BuildProjectForm::on_btn_cancel_clicked()
{
    folder_ = "";
    project_path_ = "";
    delete ui;
    this->close();
}

void BuildProjectForm::on_btn_ok_clicked()
{
    GeoMagnetismProject *geomag_proj = new GeoMagnetismProject();
    if(ui->lineEdit_name->text().isEmpty())
    {
        return;
    }
    geomag_proj->SetName(ui->lineEdit_name->text());
    geomag_proj->SetPath(ui->lineEdit_path->text());
    geomag_proj->SetBuildTime(QDateTime::currentDateTime());
    geomag_proj->SetLastUpdateTime(geomag_proj->BuildTime());

    // Create project folder
    ProjectManager::Create(*geomag_proj);
    this->close();
    emit Created(geomag_proj);
}
