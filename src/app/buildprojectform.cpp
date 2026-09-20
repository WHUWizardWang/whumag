#include "buildprojectform.h"
#include "ui_buildprojectform.h"

#include "formkit.h"
#include "uiscale.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

BuildProjectForm::BuildProjectForm(QWidget *parent, QString path) :
    QWidget(parent),
    ui(new Ui::BuildProjectForm),
    geomag_proj_(nullptr)
{
    ui->setupUi(this);
    ui->lineEdit_path->setText(path);
    folder_ = path;
    buildLayout();
}

BuildProjectForm::BuildProjectForm(GeoMagnetismProject *gmproj, QString path) :
    ui(new Ui::BuildProjectForm)
{
    ui->setupUi(this);
    ui->lineEdit_path->setText(path);
    folder_ = path;
    geomag_proj_ = gmproj;
    buildLayout();
}

BuildProjectForm::~BuildProjectForm()
{
    delete ui;
}

// Replaces the fixed-geometry layout of the .ui with a resizable one.
void BuildProjectForm::buildLayout()
{
    setWindowTitle(tr("新建工程"));
    resize(UiScale::windowSize(560, 340));
    setMinimumWidth(UiScale::dp(480));

    auto *root = new QVBoxLayout;
    root->setContentsMargins(26, 22, 26, 18);
    root->setSpacing(16);
    root->addWidget(FormKit::header(QStringLiteral("folder"), tr("新建工程"), tr("工程文件夹会创建在所选位置下")));

    ui->lineEdit_name->setPlaceholderText(tr("例如 南海北部_2025"));
    root->addLayout(FormKit::field(tr("工程名称"), ui->lineEdit_name));

    ui->btn_browser->setText(tr("更改位置…"));
    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(ui->lineEdit_path, 1);
    pathRow->addWidget(ui->btn_browser);
    root->addLayout(FormKit::field(tr("工程路径"), pathRow, tr("完整路径 = 所选位置 / 工程名称。")));
    root->addStretch(1);

    ui->btn_ok->setText(tr("创建"));
    ui->btn_ok->setDefault(true);
    ui->btn_ok->setMinimumWidth(96);
    FormKit::setRole(ui->btn_ok, "primary");
    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addStretch(1);
    footer->addWidget(ui->btn_cancel);
    footer->addWidget(ui->btn_ok);
    root->addLayout(footer);

    // the .ui put everything in one absolutely positioned container; adopt the widgets, drop it
    setLayout(root);
    delete ui->verticalLayoutWidget;
}

void BuildProjectForm::on_btn_browser_clicked()
{
    const QString start = folder_.isEmpty() ? QStringLiteral("./") : folder_;
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("选择工程路径"), start, QFileDialog::ShowDirsOnly);
    if (chosen.isEmpty())
        return;   // cancelled: keep what was there
    folder_ = chosen;
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
    // (The old version deleted |ui| here and again in the destructor.)
    ui->lineEdit_name->clear();
    this->close();
}

void BuildProjectForm::on_btn_ok_clicked()
{
    if(ui->lineEdit_name->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, tr("新建工程"), tr("请输入工程名称。"));
        ui->lineEdit_name->setFocus();
        return;
    }
    GeoMagnetismProject *geomag_proj = new GeoMagnetismProject();
    geomag_proj->SetName(ui->lineEdit_name->text());
    geomag_proj->SetPath(ui->lineEdit_path->text());
    geomag_proj->SetBuildTime(QDateTime::currentDateTime());
    geomag_proj->SetLastUpdateTime(geomag_proj->BuildTime());

    // Create project folder
    ProjectManager::Create(*geomag_proj);
    this->close();
    emit Created(geomag_proj);
}
