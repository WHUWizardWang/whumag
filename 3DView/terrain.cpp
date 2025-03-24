#include "terrain.h"
Terrain::Terrain(): m_count(0)
{
    m_data.resize(0);
    m_count = 0;
    m_mode = 0;
}
Terrain::Terrain(const QString &imagePath)
    : m_count(0)
{
    QImage image(imagePath);
    if (image.isNull())
    {
        return;
    }

    int ncols = image.width();
    int nrows = image.height();
    m_data.resize((ncols - 1) * (nrows - 1) * 6 * 6);

    for (int i = 0; i < nrows - 1; ++i)
    {
        for (int j = 0; j < ncols - 1; ++j)
        {
            double z1 = qGray(image.pixel(j, i)) * 1.0 / 256;
            double z2 = qGray(image.pixel(j + 1, i)) * 1.0 / 256;
            double z3 = qGray(image.pixel(j + 1, i + 1)) * 1.0 / 256;
            double z4 = qGray(image.pixel(j, i + 1)) * 1.0 / 256;
            QVector3D p1(i * 1.0 / nrows, j * 1.0 / ncols, z1);
            QVector3D p2(i * 1.0 / nrows, (j + 1) * 1.0 / ncols, z2);
            QVector3D p3((i + 1) * 1.0 / nrows, (j + 1) * 1.0 / ncols, z3);
            QVector3D p4((i + 1) * 1.0 / nrows, j * 1.0 / ncols, z4);
            normalize(p1);
            normalize(p2);
            normalize(p3);
            normalize(p4);
            quad(p1, p2, p3, p4);
        }
    }
}

void Terrain::load()
{
    calcuColorIndex();
    addTerrainData();
    addMaganoData();
}

void Terrain::load(const QVector<double> &pnts, int width, int height)
{
    double minZ = *(std::min_element(pnts.begin(), pnts.end()));
    double maxZ = *(std::max_element(pnts.begin(), pnts.end()));
    m_data.clear();
    m_count = 0;
    m_data.resize((width - 1) * (height - 1) * 6 * 6);

    //    m_scale = scale;
    double offset = 0.5;// * scale;
    for (int i = 0; i < height - 1; ++i)
    {
        for (int j = 0; j < width - 1; ++j)
        {
            double z1 = normalize(pnts[i * width + j], minZ, maxZ) - offset;
            double z2 = normalize(pnts[i * width + j + 1], minZ, maxZ) - offset;
            double z3 = normalize(pnts[(i + 1) * width + j + 1], minZ, maxZ) - offset;
            double z4 = normalize(pnts[(i + 1) * width + j], minZ, maxZ) - offset;
            QVector3D p1(normalize(i, 0, height - 1), normalize(j, 0, width - 1), z1);
            QVector3D p2(normalize(i, 0, height - 1), normalize(j + 1, 0, width - 1), z2);
            QVector3D p3(normalize(i + 1, 0, height - 1), normalize(j + 1, 0, width - 1), z3);
            QVector3D p4(normalize(i + 1, 0, height - 1), normalize(j, 0, width - 1), z4);
            normalize(p1);
            normalize(p2);
            normalize(p3);
            normalize(p4);
            quad(p1, p2, p3, p4);
        }
    }
}

void Terrain::load(const QVector<double> &pnts, int width, int height, const QVector<QVector3D> &colors)
{
    double minZ = *(std::min_element(pnts.begin(), pnts.end()));
    double maxZ = *(std::max_element(pnts.begin(), pnts.end()));
    m_data.clear();
    m_count = 0;
    m_data.resize((width - 1) * (height - 1) * 6 * 9);

    //    m_scale = scale;
    double offset = 0.5;// * scale;
    for (int i = 0; i < height - 1; ++i)
    {
        for (int j = 0; j < width - 1; ++j)
        {
            double z1 = normalize(pnts[i * width + j], minZ, maxZ) - offset;
            double z2 = normalize(pnts[i * width + j + 1], minZ, maxZ) - offset;
            double z3 = normalize(pnts[(i + 1) * width + j + 1], minZ, maxZ) - offset;
            double z4 = normalize(pnts[(i + 1) * width + j], minZ, maxZ) - offset;
            QVector3D p1(normalize(i, 0, height - 1), normalize(j, 0, width - 1), z1);
            QVector3D p2(normalize(i, 0, height - 1), normalize(j + 1, 0, width - 1), z2);
            QVector3D p3(normalize(i + 1, 0, height - 1), normalize(j + 1, 0, width - 1), z3);
            QVector3D p4(normalize(i + 1, 0, height - 1), normalize(j, 0, width - 1), z4);
            normalize(p1);
            normalize(p2);
            normalize(p3);
            normalize(p4);
            QVector3D c1 = colors[i * width + j];
            QVector3D c2 = colors[i * width + j + 1];
            QVector3D c3 = colors[(i + 1) * width + j + 1];
            QVector3D c4 = colors[(i + 1) * width + j];
            quad(p1, p2, p3, p4, c1, c2, c3, c4);
        }
    }
}

void Terrain::load(const QVector<double> &pnts, int width, int height, double scale)
{
    double minZ = *(std::min_element(pnts.begin(), pnts.end()));
    double maxZ = *(std::max_element(pnts.begin(), pnts.end()));
    m_data.clear();
    m_count = 0;
    m_data.resize((width - 1) * (height - 1) * 6 * 6);

    m_scale = scale;
    double offset = 0.5 * scale;
    for (int i = 0; i < height - 1; ++i)
    {
        for (int j = 0; j < width - 1; ++j)
        {
            double z1 = scale * normalize(pnts[i * width + j], minZ, maxZ) - offset;
            double z2 = scale * normalize(pnts[i * width + j + 1], minZ, maxZ) - offset;
            double z3 = scale * normalize(pnts[(i + 1) * width + j + 1], minZ, maxZ) - offset;
            double z4 = scale * normalize(pnts[(i + 1) * width + j], minZ, maxZ) - offset;
            QVector3D p1(normalize(i, 0, height - 1), normalize(j, 0, width - 1), z1);
            QVector3D p2(normalize(i, 0, height - 1), normalize(j + 1, 0, width - 1), z2);
            QVector3D p3(normalize(i + 1, 0, height - 1), normalize(j + 1, 0, width - 1), z3);
            QVector3D p4(normalize(i + 1, 0, height - 1), normalize(j, 0, width - 1), z4);
            normalize(p1);
            normalize(p2);
            normalize(p3);
            normalize(p4);
            quad(p1, p2, p3, p4);
        }
    }
}


void Terrain::add(const QVector3D &v, const QVector3D &n)
{
    int idx = m_count;
    m_data[idx++] = v.x();
    m_data[idx++] = v.y();
    m_data[idx++] = v.z();
    m_data[idx++] = n.x();
    m_data[idx++] = n.y();
    m_data[idx++] = n.z();
    m_count += 6;
}

void Terrain::add(const QVector3D &v, const QVector3D &n, const QVector3D &c)
{
    add(v, n);
    int idx = m_count;
    m_data[idx++] = c.x();
    m_data[idx++] = c.y();
    m_data[idx++] = c.z();
    m_count += 3;

    m_data[idx++] = m_dataType;
    m_count++;
}


void Terrain::quad(const QVector3D &p1, const QVector3D &p2, const QVector3D &p3, const QVector3D &p4)
{
    QVector3D n = QVector3D::normal(p4 - p1, p2 - p1);
    add(p2, n);
    add(p1, n);
    add(p4, n);

    n = QVector3D::normal(p4 - p2, p3 - p2);
    add(p2, n);
    add(p4, n);
    add(p3, n);
}

void Terrain::quad(const QVector3D &p1, const QVector3D &p2, const QVector3D &p3, const QVector3D &p4,
                   const QVector3D &c1, const QVector3D &c2, const QVector3D &c3, const QVector3D &c4)
{
    if (c2.x() < 0.0f || c4.x() < 0.0f)
    {
        return;
    }
    QVector3D n;
    if (c1.x() >= 0.0f)
    {
        n = QVector3D::normal(p4 - p1, p2 - p1);
        add(p2, n, c2);
        add(p1, n, c1);
        add(p4, n, c4);
    }

    if (c3.x() >= 0.0f)
    {
        n = QVector3D::normal(p4 - p2, p3 - p2);
        add(p2, n, c2);
        add(p4, n, c4);
        add(p3, n, c3);
    }
}

void Terrain::normalize(QVector3D &p)
{
    p.setX(p.x() * 2.0 - 1.0);
    p.setY(p.y() * 2.0 - 1.0);
    //    p.setZ(p.z() * 2.0 - 1.0);
}

double Terrain::normalize(double v, double minV, double maxV)
{
    if (minV >= maxV)
    {
        qDebug() << "minV is not less than maxV!";
        return 0;
    }
    if (v < minV)
    {
        return 0;
    }
    if (v > maxV)
    {
        return 1;
    }
    return (v - minV) / (maxV - minV);
}

void Terrain::setScale(double scale_z)
{
    double cof = scale_z / m_scale;
    for (int i = 2; i < m_count; i += 6)
    {
        m_data[i] *= cof;
    }
    m_scale = scale_z;
}

void Terrain::setHeight(int height)
{
    m_height = height;
}

void Terrain::setWidth(int width)
{
    m_width = width;
}

void Terrain::setMask(const VVb &mask)
{
    m_mask = mask;
}

void Terrain::setTerrainData(const VVf &terrain)
{
    m_terrain = terrain;
}

void Terrain::setMaganoData(const VVf &magano)
{
    m_magano = magano;
}

void Terrain::setDisplayMode(int mode)
{
    if (mode != 0 && mode != 1 & mode != 2)
    {
        mode = 0;
    }
    m_mode = mode;
}

void Terrain::setTerrainColorMap(const QVector<QVector3D> &colorMap)
{
    m_t_colorMap = colorMap;
}

void Terrain::setMaganoColorMap(const QVector<QVector3D> &colorMap)
{
    m_m_colorMap = colorMap;
}

bool Terrain::verifyDataSize(const VVf &data)
{
    int width = 0, height = 0;
    if (data.size() > 0)
    {
        height = data.size();
        width = data[0].size();
    }
    if (height != m_mask.size())
    {
        return false;
    }
    else if (height > 0 && width != m_mask[0].size())
    {
        return false;
    }
    return width == m_width && height == m_height;
}

void Terrain::calcuColorIndex()
{
    if (!(verifyDataSize(m_terrain) && verifyDataSize(m_magano)))
    {
        qDebug() << "The sizes of variables are not match!";
        return;
    }

    GLfloat minZ, maxZ, minV, maxV;
    findMinMax(m_terrain, minZ, maxZ);
    findMinMax(m_magano, minV, maxV);

    m_t_colorIndex.resize(m_height);
    m_m_colorIndex.resize(m_height);
    for (int i = 0; i < m_height; ++i)
    {
        m_t_colorIndex[i].resize(m_width);
        m_m_colorIndex[i].resize(m_width);
        for (int j = 0; j < m_width; ++j)
        {
            if (m_mask[i][j])
            {
                m_t_colorIndex[i][j] = static_cast<int>((m_terrain[i][j] - minZ) / (maxZ - minZ) * (m_t_colorMap.size() - 1));
                m_m_colorIndex[i][j] = static_cast<int>((m_magano[i][j] - minV) / (maxV - minV) * (m_m_colorMap.size() - 1));
            }
        }
    }
}

void Terrain::addTerrainData()
{
    // Map terrain to -.5*scale to .5*scale
    GLfloat minZ, maxZ;     // min and max value of Terrain
    findMinMax(m_terrain, minZ, maxZ);
    m_data.clear();
    m_count = 0;
    m_data.resize((m_width - 1) * (m_height - 1) * 6 * 10 * 2);
    m_dataType = 0.0f;

    GLfloat minV, maxV;     // min and max value of mapped Terrain
    minV = -0.5 * m_scale;
    maxV = 0.5 * m_scale;
    for (int i = 0; i < m_height - 1; ++i)
    {
        for (int j = 0; j < m_width - 1; ++j)
        {

            if (m_mask[i][j] && m_mask[i][j + 1] && m_mask[i + 1][j + 1] && m_mask[i + 1][j])
            {
                QVector3D p1, p2, p3, p4;
                QVector3D c1, c2, c3, c4;
                getQuadPoint(0, i, j, minV, maxV, minZ, maxZ, p1, c1);
                getQuadPoint(0, i, j + 1, minV, maxV, minZ, maxZ, p2, c2);
                getQuadPoint(0, i + 1, j + 1, minV, maxV, minZ, maxZ, p3, c3);
                getQuadPoint(0, i + 1, j, minV, maxV, minZ, maxZ, p4, c4);
                quad(p1, p2, p3, p4, c1, c2, c3, c4);
            }
        }
    }
}

void Terrain::addMaganoData()
{
    if (m_mode == 0 || m_mode == 1)
    {
        return;
    }
    m_dataType = 1.0f;

    GLfloat minZ, maxZ;
    findMinMax(m_magano, minZ, maxZ);

    for (int i = 0; i < m_height - 1; ++i)
    {
        for (int j = 0; j < m_width - 1; ++j)
        {
            if (m_mask[i][j] && m_mask[i][j + 1] && m_mask[i + 1][j + 1] && m_mask[i + 1][j])
            {
                QVector3D p1, p2, p3, p4;
                QVector3D c1, c2, c3, c4;
                getQuadPoint(1, i, j, 0, 0, 0, 0, p1, c1);
                getQuadPoint(1, i, j + 1, 0, 0, 0, 0, p2, c2);
                getQuadPoint(1, i + 1, j + 1, 0, 0, 0, 0, p3, c3);
                getQuadPoint(1, i + 1, j, 0, 0, 0, 0, p4, c4);
                quad(p1, p2, p3, p4, c1, c2, c3, c4);
            }
        }
    }

}

void Terrain::findMinMax(const VVf &data, GLfloat &minV, GLfloat &maxV)
{
    minV = 99999.9999;
    maxV = -99999.9999;
    for (int i = 0; i < m_height; ++i)
    {
        for (int j = 0; j < m_width; ++j)
        {
            if (m_mask[i][j])
            {
                minV = qMin(minV, data[i][j]);
                maxV = qMax(maxV, data[i][j]);
            }
        }
    }
}

GLfloat Terrain::mapMinMax(GLfloat value, GLfloat minV, GLfloat maxV, GLfloat minZ, GLfloat maxZ)
{
    return minV + (maxV - minV) * normalize(value, minZ, maxZ);
}

void Terrain::getQuadPoint(int dataType, int i, int j,
                           GLfloat minV, GLfloat maxV, GLfloat minZ, GLfloat maxZ,
                           QVector3D &point, QVector3D &color)
{
    GLfloat z;
    if (dataType == 0)  // Terrain data
    {
        z = mapMinMax(m_terrain[i][j], minV, maxV, minZ, maxZ);
        if (m_mode == 0 || m_mode == 2)
        {
            color = m_t_colorMap[m_t_colorIndex[i][j]];
        }
        else if (m_mode == 1)
        {
            color = m_m_colorMap[m_m_colorIndex[i][j]];
        }
    }
    else                // Magano data
    {
        z = 0.7f;
        color = m_m_colorMap[m_m_colorIndex[i][j]];
    }

    point = QVector3D(normalize(i, 0, m_height - 1), normalize(j, 0, m_width - 1), z);
    normalize(point);
}

void Terrain::setMode(int mode)
{
    m_mode = mode;
}
