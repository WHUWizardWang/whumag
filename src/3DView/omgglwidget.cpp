#include "omgglwidget.h"
#include "ui_omgglwidget.h"

OmgGLWidget::OmgGLWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OmgGLWidget)
{
    ui->setupUi(this);
    m_terrain_ptr = new Terrain();
    ui->openGLWidget->setTerrainPointer(m_terrain_ptr);
    ui->openGLWidget->setColorMaps();

    connect(ui->openGLWidget, &GLWidget::xRotationChanged, ui->widget, &OmgGLCtrl::moveSliderX);
    connect(ui->openGLWidget, &GLWidget::yRotationChanged, ui->widget, &OmgGLCtrl::moveSliderY);
    connect(ui->openGLWidget, &GLWidget::zRotationChanged, ui->widget, &OmgGLCtrl::moveSliderZ);
    connect(ui->openGLWidget, &GLWidget::hasTextureChanged, ui->widget, &OmgGLCtrl::changeCheckboxTexture);

    connect(ui->widget, &OmgGLCtrl::verticalSliderXMoved, ui->openGLWidget, &GLWidget::setXRotation);
    connect(ui->widget, &OmgGLCtrl::verticalSliderYMoved, ui->openGLWidget, &GLWidget::setYRotation);
    connect(ui->widget, &OmgGLCtrl::verticalSliderZMoved, ui->openGLWidget, &GLWidget::setZRotation);
    connect(ui->widget, &OmgGLCtrl::verticalSliderScaleMoved, ui->openGLWidget, &GLWidget::setScale);

    connect(ui->widget, &OmgGLCtrl::pushButtonRawViewClicked, ui->openGLWidget, &GLWidget::toDefaultView);
    connect(ui->widget, &OmgGLCtrl::pushButtonZoomInClicked, ui->openGLWidget, &GLWidget::zoomIn);
    connect(ui->widget, &OmgGLCtrl::pushButtonZoomOutClicked, ui->openGLWidget, &GLWidget::zoomOut);
    connect(ui->widget, &OmgGLCtrl::pushButtonMoveViewClicked, ui->openGLWidget, &GLWidget::panView);

    connect(ui->widget, &OmgGLCtrl::colorMapChanged, ui->openGLWidget, &GLWidget::changeColorMap);
    connect(ui->widget, &OmgGLCtrl::colorMapInversed, ui->openGLWidget, &GLWidget::inverseColorMap);
    connect(ui->widget, &OmgGLCtrl::shadeClosed, ui->openGLWidget, &GLWidget::closeShade);
    connect(ui->widget, &OmgGLCtrl::textureClosed, ui->openGLWidget, &GLWidget::closeTexture);

    connect(ui->widget, &OmgGLCtrl::magColorMapChanged, ui->openGLWidget, &GLWidget::changeMagColorMap);
    connect(ui->widget, &OmgGLCtrl::magColorMapInversed, ui->openGLWidget, &GLWidget::inverseMagColorMap);
    connect(ui->widget, &OmgGLCtrl::displayModeChanged, ui->openGLWidget, &GLWidget::changeDisplayMode);
}

OmgGLWidget::~OmgGLWidget()
{
    delete m_terrain_ptr;
    delete ui;
}

void OmgGLWidget::load()
{
    ui->openGLWidget->load();
}

void OmgGLWidget::setHeight(int height)
{
    m_terrain_ptr->setHeight(height);
}

void OmgGLWidget::setWidth(int width)
{
    m_terrain_ptr->setWidth(width);
}

void OmgGLWidget::setMask(VVb &mask)
{
    m_terrain_ptr->setMask(mask);
}

void OmgGLWidget::setTerrain(VVf &terrain)
{
    m_terrain_ptr->setTerrainData(terrain);
}

void OmgGLWidget::setMagano(VVf &magano)
{
    m_terrain_ptr->setMaganoData(magano);
}
