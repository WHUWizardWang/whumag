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

    //    m_gl_ctrl = new OmgGLCtrl(this);
    //    ui->horizontalLayout->addWidget(m_gl_ctrl);
}

OmgGLWidget::~OmgGLWidget()
{
    //    if (m_gl_ctrl != nullptr)
    //    {
    //        delete m_gl_ctrl;
    //        m_gl_ctrl = nullptr;
    //    }
    delete m_terrain_ptr;
    delete ui;
}

void OmgGLWidget::load(const QVector<double> &griddate, int width, int height)
{
    if (griddate.size() != width * height)
    {
        qDebug() << "griddate.size() != width * height!";
        return;
    }

    ui->openGLWidget->loadNewTerrain(griddate, width, height, 0.1 * ui->widget->scale());
}

void OmgGLWidget::load(const QVector<double> &griddate, int width, int height,
                       const QVector<QVector3D> &colors)
{
    if (griddate.size() != width * height || colors.size() != width * height)
    {
        qDebug() << "griddate.size() != width * height!";
        return;
    }
    ui->openGLWidget->loadNewTerrain(griddate, width, height, 0.1 * ui->widget->scale(), colors);
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

//void OmgGLWidget::initColorMaps()
//{
//    m_colorMapMap.clear();

//    QStringList colorMapNames = {"jet", "hot", "deep", "drywet", "balance"};
//    for (int i = 0; i < colorMapNames.size(); ++i)
//    {
//        QString colorMapPath = ":/resources/" + colorMapNames[i] + ".rgb";
//        QVector<QVector3D> colorMap = loadColorMap(colorMapPath);
//        m_colorMapMap.insert(colorMapNames[i], colorMap);
//    }
//}

//void OmgGLWidget::changeColorMap(const QString &colorMapName)
//{
//    m_colorMap = m_colorMapMap[colorMapName];
//    m_colorNum = m_colorMap.size();
//    if (m_isInverse)
//    {
//        reverseVertor(m_colorMap);
//    }
//    update();
//}

//void OmgGLWidget::reverseVertor(QVector<QVector3D> &vec)
//{
//    QVector<QVector3D>::iterator begIter = vec.begin();
//    QVector<QVector3D>::iterator endIter = vec.end();
//    while (begIter != endIter && begIter != --endIter)
//    {
//        std::iter_swap(begIter, endIter);
//        ++begIter;
//    }
//}

//void OmgGLWidget::inverseColorMap(bool isInverse)
//{
//    if (m_isInverse != isInverse)
//    {
//        reverseVertor(m_colorMap);
//    }
//    m_isInverse = isInverse;
//    update();
//}
