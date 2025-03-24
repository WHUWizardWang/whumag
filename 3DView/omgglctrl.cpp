#include "omgglctrl.h"
#include "ui_omgglctrl.h"

OmgGLCtrl::OmgGLCtrl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OmgGLCtrl)
{
    ui->setupUi(this);
    QStringList colorMapNames = {"jet", "hot", "deep", "drywet", "balance"};
    ui->comboBox_color->addItems(colorMapNames);
    ui->comboBox_color_2->addItems(colorMapNames);
    ui->checkBox_closeTexture->hide();
    ui->radioButton_1->setChecked(true);
    ui->verticalSlider_scale->setValue(10);
}

OmgGLCtrl::~OmgGLCtrl()
{
    delete ui;
}

int OmgGLCtrl::scale()
{
    return ui->verticalSlider_scale->value();
}

static void normalizeAngle(int &angle)
{
    while (angle < 0)
    {
        angle += 360 * 16;
    }
    while (angle >= 360 * 16)
    {
        angle -= 360 * 16;
    }
}

void OmgGLCtrl::moveSliderX(int value)
{
    if (value == ui->verticalSlider_x->value())
    {
        return;
    }
    normalizeAngle(value);
    ui->verticalSlider_x->setValue(value);
}

void OmgGLCtrl::moveSliderY(int value)
{
    if (value == ui->verticalSlider_y->value())
    {
        return;
    }
    normalizeAngle(value);
    ui->verticalSlider_y->setValue(value);
}

void OmgGLCtrl::moveSliderZ(int value)
{
    if (value == ui->verticalSlider_z->value())
    {
        return;
    }
    normalizeAngle(value);
    ui->verticalSlider_z->setValue(value);
}

void OmgGLCtrl::on_verticalSlider_x_sliderMoved(int position)
{
    emit verticalSliderXMoved(position);
}

void OmgGLCtrl::on_verticalSlider_y_sliderMoved(int position)
{
    emit verticalSliderYMoved(position);
}

void OmgGLCtrl::on_verticalSlider_z_sliderMoved(int position)
{
    emit verticalSliderZMoved(position);
}

void OmgGLCtrl::on_verticalSlider_scale_sliderMoved(int position)
{
    emit verticalSliderScaleMoved(position);
}


void OmgGLCtrl::on_pushButton_rawView_clicked()
{
    emit pushButtonRawViewClicked();
}

void OmgGLCtrl::on_pushButton_zoomIn_clicked()
{
    emit pushButtonZoomInClicked();
}

void OmgGLCtrl::on_pushButton_zoomOut_clicked()
{
    emit pushButtonZoomOutClicked();
}


void OmgGLCtrl::on_pushButton_moveView_clicked(bool checked)
{
    emit pushButtonMoveViewClicked(checked);
}



void OmgGLCtrl::on_comboBox_color_currentTextChanged(const QString &arg1)
{
    emit colorMapChanged(arg1);
}

void OmgGLCtrl::on_checkBox_inverse_toggled(bool checked)
{
    emit colorMapInversed(checked);
}

void OmgGLCtrl::on_checkBox_closeShade_toggled(bool checked)
{
    emit shadeClosed(checked);
}

void OmgGLCtrl::on_checkBox_closeTexture_toggled(bool checked)
{
    emit textureClosed(checked);
}

void OmgGLCtrl::changeCheckboxTexture(bool hasTexture)
{
    ui->checkBox_closeTexture->setEnabled(hasTexture);
}

void OmgGLCtrl::on_comboBox_color_2_currentIndexChanged(const QString &arg1)
{
    emit magColorMapChanged(arg1);
}

void OmgGLCtrl::on_checkBox_inverse_2_toggled(bool checked)
{
    emit magColorMapInversed(checked);
}

void OmgGLCtrl::on_radioButton_1_toggled(bool checked)
{
    if (checked)
    {
        emit displayModeChanged(0);
    }
}

void OmgGLCtrl::on_radioButton_2_toggled(bool checked)
{
    if (checked)
    {
        emit displayModeChanged(1);
    }
}

void OmgGLCtrl::on_radioButton_3_toggled(bool checked)
{
    if (checked)
    {
        emit displayModeChanged(2);
    }
}
