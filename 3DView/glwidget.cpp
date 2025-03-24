#include "glwidget.h"
#include <QMouseEvent>
#include <QtMath>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_program(0),
      m_terrain_ptr(nullptr),
      m_isPanning(false),
      m_closeShade(false),
      m_isInverse(false),
      m_closeTexture(0)
{
    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
    m_scale = 1.0;

    initColorMaps();
    m_colorMap = m_colorMapMap["jet"];
    m_colorNum = m_colorMap.size();
    if (m_isInverse)
    {
        reverseVertor(m_colorMap);
    }

    m_magColorMap = m_colorMapMap["jet"];
    if (m_isMagInverse)
    {
        reverseVertor(m_magColorMap);
    }
}

GLWidget::~GLWidget()
{
    cleanup();
}

void GLWidget::setTerrainPointer(Terrain *terrain_ptr)
{
    m_terrain_ptr = terrain_ptr;
}

void GLWidget::cleanup()
{
    if (m_program == nullptr)
    {
        return;
    }
    makeCurrent();
    m_terrainVbo.destroy();
    delete  m_program;
    m_program = 0;
    doneCurrent();
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
    {
        angle += 360 * 16;
    }
    while (angle > 360 * 16)
    {
        angle -= 360 * 16;
    }
}

void GLWidget::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_xRot)
    {
        m_xRot = angle;
        emit xRotationChanged(angle);
        update();
    }
}

void GLWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_yRot)
    {
        m_yRot = angle;
        emit yRotationChanged(angle);
        update();
    }
}

void GLWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_zRot)
    {
        m_zRot = angle;
        emit zRotationChanged(angle);
        update();
    }
}

void GLWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);
    initializeOpenGLFunctions();
    glClearColor(1.0, 1.0, 1.0, 1);
    m_program = new QOpenGLShaderProgram;
    //    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_core ? vertexShaderSourceCore : vertexShaderSource);
    //    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_core ? fragmentShaderSourceCore : fragmentShaderSource);
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/vertex.vsh");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/fragment.fsh");
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("normal", 1);
    m_program->bindAttributeLocation("colors", 2);
    m_program->bindAttributeLocation("type", 3);
    m_program->link();
    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_normalMatrixLoc = m_program->uniformLocation("normalMatrix");
    m_lightPosLoc = m_program->uniformLocation("lightPos");
    m_colorMapLoc = m_program->uniformLocation("colorMap");
    m_scaleLoc = m_program->uniformLocation("scale");
    m_colorNumLoc = m_program->uniformLocation("colorNum");
    m_closeShadeLoc = m_program->uniformLocation("closeShade");
    m_closeTextureLoc = m_program->uniformLocation("closeTexture");

    // VAO vertex array object
    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    // VBO vertex buffer object
    m_terrainVbo.create();
    m_terrainVbo.bind();
    m_terrainVbo.allocate(m_terrain_ptr->constData(), m_terrain_ptr->count() * sizeof(GLfloat));
    //    m_terrainVbo.allocate(m_logo.constData(), m_logo.count() * sizeof(GLfloat));


    setupVertexAttribs();

    // Our camera never changes in this example.
    m_camera.setToIdentity();
    m_camera.translate(0, 0, -4);

    m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 0, 70));
    m_program->release();
}

void GLWidget::setupVertexAttribs()
{
    m_terrainVbo.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glEnableVertexAttribArray(2);
    f->glEnableVertexAttribArray(3);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), 0);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof (GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof (GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
    f->glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof (GLfloat), reinterpret_cast<void *>(9 * sizeof(GLfloat)));
    m_terrainVbo.release();
}

void GLWidget::load()
{
    m_hasTexture = false;
    m_closeTexture = true;
    emit hasTextureChanged(m_hasTexture);

    //    QVector<QVector3D> color(pnts.size(), QVector3D(0, 0, 0));
    //    m_scale = scale;
    //    m_terrain_ptr->load(pnts, width, height, color);
    m_terrain_ptr->setScale(m_scale);
    m_terrain_ptr->setTerrainColorMap(m_colorMap);
    m_terrain_ptr->setMaganoColorMap(m_colorMapMap["hot"]);
    m_terrain_ptr->load();
    m_terrainVbo.bind();
    m_terrainVbo.allocate(m_terrain_ptr->constData(), m_terrain_ptr->count() * sizeof(GLfloat));
    setupVertexAttribs();
    update();
    toDefaultView();
}

void GLWidget::loadNewTerrain(const QVector<double> &pnts, int width, int height, double scale)
{
    m_hasTexture = false;
    m_closeTexture = true;
    emit hasTextureChanged(m_hasTexture);

    QVector<QVector3D> color(pnts.size(), QVector3D(0, 0, 0));
    m_scale = scale;
    m_terrain_ptr->load(pnts, width, height, color);
    m_terrainVbo.bind();
    m_terrainVbo.allocate(m_terrain_ptr->constData(), m_terrain_ptr->count() * sizeof(GLfloat));
    setupVertexAttribs();
    update();
    toDefaultView();
}

void GLWidget::loadNewTerrain(const QVector<double> &pnts, int width, int height, double scale,
                              const QVector<QVector3D> &colors)
{
    m_hasTexture = true;
    m_closeTexture = false;
    emit hasTextureChanged(m_hasTexture);

    m_scale = scale;
    m_terrain_ptr->load(pnts, width, height, colors);
    m_terrainVbo.bind();
    m_terrainVbo.allocate(m_terrain_ptr->constData(), m_terrain_ptr->count() * sizeof(GLfloat));
    setupVertexAttribs();
    update();
    toDefaultView();
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_world.setToIdentity();
    m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate((270 * 16.0f + m_zRot) / 16.0f, 0, 0, 1);

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_proj);
    m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setUniformValue(m_normalMatrixLoc, normalMatrix);
    m_program->setUniformValue(m_scaleLoc, m_scale);
    m_program->setUniformValue(m_closeShadeLoc, m_closeShade);
    m_program->setUniformValue(m_colorNumLoc, m_colorNum);
    m_program->setUniformValue(m_closeTextureLoc, m_closeTexture);
    m_program->setUniformValueArray(m_colorMapLoc, m_colorMap.data(), m_colorMap.length());
    glDrawArrays(GL_TRIANGLES, 0, m_terrain_ptr->vertexCount());

    m_program->release();
}

void GLWidget::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(45.0f, GLfloat(w) / h, 0.1f, 100.0f);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();
    if (!m_isPanning && event->buttons()&Qt::LeftButton)
    {
        setXRotation(m_xRot + 8 * dy);
        setYRotation(m_yRot + 8 * dx);
    }
    else if (!m_isPanning && event->buttons()&Qt::RightButton)
    {
        setXRotation(m_xRot + 8 * dy);
        setZRotation(m_zRot + 8 * dx);
    }
    else if ((m_isPanning && event->buttons()&Qt::LeftButton) || event->buttons()&Qt::MidButton)
    {
        GLfloat viewHeight = 2 * qAbs(m_camera(2, 3) * qTan(qDegreesToRadians(45.0 / 2)));
        GLfloat viewWidth = width() * 1.0f / height() * viewHeight;
        GLfloat deltaX = dx * viewWidth / width();
        GLfloat deltaY = dy * viewHeight / height();
        m_camera.translate(deltaX, -deltaY, 0.0);
        update();
    }
    m_lastPos = event->pos();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    //    if ((m_camera(2, 3) < -2.0f && event->delta() > 0) || (m_camera(2, 3) > -10.0f && event->delta() < 0))
    //    {
    //        m_camera.translate(0, 0, event->delta() / (5 * 120.0f));
    //    }
    zoom(event->delta() / (3 * 120.f));
    update();
}

void GLWidget::toDefaultView()
{
    setXRotation(0);
    setYRotation(0);
    setZRotation(0);
    m_camera.setToIdentity();
    m_camera.translate(0, 0, -4);
    update();
}

void GLWidget::zoomIn()
{
    zoom(1.0);
    update();
}

void GLWidget::zoomOut()
{
    zoom(-1.0);
    update();
}

void GLWidget::zoom(GLfloat value)
{
    GLfloat z = m_camera(2, 3) + value;
    if (z > -10.1f && z < -0.9f)
    {
        m_camera.translate(0, 0, value);
    }
}

void GLWidget::panView(bool isPanning)
{
    m_isPanning = isPanning;
}

void GLWidget::setScale(int scale)
{
    double scale_z = scale * 0.1;
    //    m_terrain.setScale(scale_z);
    //    m_terrainVbo.bind();
    //    m_terrainVbo.allocate(m_terrain.constData(), m_terrain.count() * sizeof(GLfloat));
    //    setupVertexAttribs();
    m_scale = scale_z;
    update();
}

void GLWidget::loadColorMap()
{
    m_colorMap.clear();
    QFile file(":/resources/matlab_jet.rgb");
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    stream.readLine();
    stream.readLine();
    while (stream.atEnd() == false)
    {
        QString line = stream.readLine();
        QStringList strArr = line.split(' ', QString::SkipEmptyParts);
        GLfloat red = strArr[0].toFloat();
        GLfloat green = strArr[1].toFloat();
        GLfloat blue = strArr[2].toFloat();
        m_colorMap.push_back(QVector3D(red, green, blue));
    }
    file.close();

    // Map to 0.0-1.0
    float maxGray = 1.0;
    for (auto iter = m_colorMap.cbegin(); iter != m_colorMap.cend(); ++iter)
    {
        if (iter->x() > 1.1f || iter->y() > 1.1f || iter->z() > 1.1f)
        {
            maxGray = 255.0;
            break;
        }
    }
    for (auto iter = m_colorMap.begin(); iter != m_colorMap.end(); ++iter)
    {
        iter->setX(iter->x() / maxGray);
        iter->setY(iter->y() / maxGray);
        iter->setZ(iter->z() / maxGray);
    }
}

QVector<QVector3D> GLWidget::loadColorMap(const QString &colorMapPath)
{
    QVector<QVector3D> colorMap;
    QFile file(colorMapPath);
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    stream.readLine();
    stream.readLine();
    while (stream.atEnd() == false)
    {
        QString line = stream.readLine();
        QStringList strArr = line.split(' ', QString::SkipEmptyParts);
        GLfloat red = strArr[0].toFloat();
        GLfloat green = strArr[1].toFloat();
        GLfloat blue = strArr[2].toFloat();
        colorMap.push_back(QVector3D(red, green, blue));
    }
    file.close();

    // Map to 0.0-1.0
    float maxGray = 1.0;
    for (auto iter = colorMap.cbegin(); iter != colorMap.cend(); ++iter)
    {
        if (iter->x() > 1.1f || iter->y() > 1.1f || iter->z() > 1.1f)
        {
            maxGray = 255.0;
            break;
        }
    }
    for (auto iter = colorMap.begin(); iter != colorMap.end(); ++iter)
    {
        iter->setX(iter->x() / maxGray);
        iter->setY(iter->y() / maxGray);
        iter->setZ(iter->z() / maxGray);
    }
    return colorMap;
}

void GLWidget::initColorMaps()
{
    m_colorMapMap.clear();

    QStringList colorMapNames = {"jet", "hot", "deep", "drywet", "balance"};
    for (int i = 0; i < colorMapNames.size(); ++i)
    {
        QString colorMapPath = ":/resources/" + colorMapNames[i] + ".rgb";
        QVector<QVector3D> colorMap = loadColorMap(colorMapPath);
        m_colorMapMap.insert(colorMapNames[i], colorMap);
    }
}

void GLWidget::changeColorMap(const QString &colorMapName)
{
    m_colorMap = m_colorMapMap[colorMapName];
    m_colorNum = m_colorMap.size();
    if (m_isInverse)
    {
        reverseVertor(m_colorMap);
    }
    myUpdate();
}

void GLWidget::reverseVertor(QVector<QVector3D> &vec)
{
    QVector<QVector3D>::iterator begIter = vec.begin();
    QVector<QVector3D>::iterator endIter = vec.end();
    while (begIter != endIter && begIter != --endIter)
    {
        std::iter_swap(begIter, endIter);
        ++begIter;
    }
}

void GLWidget::inverseColorMap(bool isInverse)
{
    if (m_isInverse != isInverse)
    {
        reverseVertor(m_colorMap);
    }
    m_isInverse = isInverse;
    myUpdate();
}

void GLWidget::closeShade(bool isCloseShade)
{
    m_closeShade = isCloseShade;
    update();
}

void GLWidget::closeTexture(bool isCloseTexture)
{
    m_closeTexture = isCloseTexture;
    update();
}

void GLWidget::myUpdate()
{
    //    m_terrain_ptr->setTerrainColorMap(m_colorMap);
    //    m_terrain_ptr->setMaganoColorMap(m_colorMap);
    //    m_terrain_ptr->load();
    //    update();

    //    m_hasTexture = false;
    //    m_closeTexture = true;
    //    emit hasTextureChanged(m_hasTexture);

    //    m_terrain_ptr->setScale(m_scale);
    m_terrain_ptr->setMode(m_displayMode);
    m_terrain_ptr->setTerrainColorMap(m_colorMap);
    m_terrain_ptr->setMaganoColorMap(m_magColorMap);
    m_terrain_ptr->load();
    m_terrainVbo.bind();
    m_terrainVbo.allocate(m_terrain_ptr->constData(), m_terrain_ptr->count() * sizeof(GLfloat));
    setupVertexAttribs();
    update();
    //    toDefaultView();
}

void GLWidget::setColorMaps()
{
    m_terrain_ptr->setTerrainColorMap(m_colorMap);
    m_terrain_ptr->setMaganoColorMap(m_magColorMap);
}

void GLWidget::changeMagColorMap(const QString &colorMapName)
{
    m_magColorMap = m_colorMapMap[colorMapName];
    if (m_isMagInverse)
    {
        reverseVertor(m_magColorMap);
    }
    myUpdate();
}

void GLWidget::inverseMagColorMap(bool isInverse)
{
    if (m_isMagInverse != isInverse)
    {
        reverseVertor(m_magColorMap);
    }
    m_isMagInverse = isInverse;
    myUpdate();
}

void GLWidget::changeDisplayMode(int mode)
{
    m_displayMode = mode;
    myUpdate();
}
