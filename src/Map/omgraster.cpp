#include "omgraster.h"



//OmgRaster::OmgRaster(QObject *parent) : QObject(parent)
//{
//    getColorMap("qrc:/jet.rgb");
//}

OmgRaster::OmgRaster()
{
    getColorMap(":/shaders/jet.rgb");
}

OmgRaster::OmgRaster(const OmgRaster &other)
{
    m_left_top_lat = other.m_left_top_lat;
    m_left_top_lon = other.m_left_top_lon;
    m_lat_resolution = other.m_lat_resolution;
    m_lon_resolution = other.m_lon_resolution;
    m_height = other.m_height;
    m_width = other.m_width;
    m_maxValue = other.m_maxValue;
    m_minValue = other.m_minValue;
    m_data = other.m_data;
    m_colorMap = other.m_colorMap;
}

void OmgRaster::setLeftTopLatLon(float lat, float lon)
{
    m_left_top_lat = lat;
    m_left_top_lon = lon;
}

void OmgRaster::setLatLonResolution(float lat_rsl, float lon_rsl)
{
    m_lat_resolution = lat_rsl;
    m_lon_resolution = lon_rsl;
}

bool OmgRaster::getColorMap(const QString &colorMapPath)
{
    m_colorMap.clear();
    QFile file(colorMapPath);
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    stream.readLine();
    stream.readLine();
    while (stream.atEnd() == false)
    {
        QString line = stream.readLine();
        QStringList strArr = line.split(' ', QString::SkipEmptyParts);
        float red = strArr[0].toFloat();
        float green = strArr[1].toFloat();
        float blue = strArr[2].toFloat();
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
    return true;
}


bool OmgRaster::loadFromImage(const QString &filePath)
{
    QImageReader reader(filePath);
    QImage image = reader.read();
    if (image.isNull())
    {
        return false;
    }

    m_height = image.height();
    m_width = image.width();
    m_data.resize(m_height * m_width);

    for (int i = 0; i < m_height; ++i)
    {
        for (int j = 0; j < m_width; ++j)
        {
            QRgb pixel = image.pixel(j, i);
            float z1 = qGray(pixel) / 255.0f;
            m_data[i * m_width + j] = z1;
        }
    }

    m_minValue = *std::min_element(m_data.cbegin(), m_data.cend());
    m_maxValue = *std::max_element(m_data.cbegin(), m_data.cend());

    setLeftTopLatLon(90.0f, 0.0f);
    //    setLatLonResolution(2.0f / 60.0f, 2.0f / 60.0f);
    setLatLonResolution(180.0f / m_height, 360.0f / m_width);
    return true;
}

bool OmgRaster::loadFromPoints(const QString &filePath)
{
    QFile file(filePath);
    bool isOpened = file.open(QIODevice::ReadOnly);
    if (!isOpened)
    {
        return false;
    }
    QTextStream stream(&file);

    QVector<QVector3D> pnts;

    while (stream.atEnd() == false)
    {
        QString line = stream.readLine();
        QStringList strArr = line.split(' ', QString::SkipEmptyParts);
        float lon = strArr[0].toFloat();
        float lat = strArr[1].toFloat();
        float value = strArr[2].toFloat();
        pnts.push_back(QVector3D(lon, lat, value));
    }

    float minLon = 999.9;
    float maxLon = -999.0;
    float minLat = 999.9;
    float maxLat = -999.9;
    float minV = 999.9;
    float maxV = -999.9;

    for (int i = 0; i < pnts.size(); ++i)
    {
        minLon = qMin(minLon, pnts[i].x());
        maxLon = qMax(maxLat, pnts[i].x());
        minLat = qMin(minLat, pnts[i].y());
        maxLat = qMax(maxLat, pnts[i].y());
        minV = qMin(minV, pnts[i].z());
        maxV = qMax(maxV, pnts[i].z());
    }

    m_minValue = minV;
    m_maxValue = maxV;

    setLeftTopLatLon(maxLat, minLon);
    setLatLonResolution(2.0f / 60.0f, 2.0f / 60.0f);
    m_height = qRound((maxLat - minLat) / m_lat_resolution) + 1;
    m_width = qRound((maxLon - minLon) / m_lon_resolution) + 1;

    m_data.clear();
    m_data = QVector<float>(m_height * m_width, -99999.9999);

    for (auto iter = pnts.cbegin(); iter != pnts.cend(); ++iter)
    {
        int i = qRound((maxLat - iter->y()) / m_lat_resolution);
        int j = qRound((iter->x() - minLon) / m_lon_resolution);
        m_data[i * m_width + j] = iter->z();
    }
    return true;
}

QVector<OmgGeoPoint> OmgRaster::getInternalPoints(QVector<Vec2d> &POL, float radio)
{
    double minX = 9999.9999;
    double maxX = -9999.9999;
    double minY = 9999.9999;
    double maxY = -9999.9999;
    //    for (auto iter = POL.begin(); iter != POL.end(); ++iter)
    //    {
    //        if (iter->x < 0.0f)
    //        {
    //            iter->x = iter->x + 360.0f;
    //        }
    //    }



    for (auto iter = POL.cbegin(); iter != POL.cend(); ++iter)
    {
        minX = qMin(minX, iter->x);
        maxX = qMax(maxX, iter->x);
        minY = qMin(minY, iter->y);
        maxY = qMax(maxY, iter->y);
    }

    // x is lon, y is lat.
    int minCol = qRound((minX - m_left_top_lon) / m_lon_resolution);
    int maxCol = qRound((maxX - m_left_top_lon) / m_lon_resolution);
    int minRow = qRound((m_left_top_lat - maxY) / m_lat_resolution);
    int maxRow = qRound((m_left_top_lat - minY) / m_lat_resolution);
    //    int minCol = qRound(minX / m_lon_resolution);
    //    int maxCol = qRound(maxX / m_lon_resolution);
    //    int minRow = qRound((90.0f - maxY) / m_lat_resolution);
    //    int maxRow = qRound((90.0f - minY) / m_lat_resolution);

    //
    int subWidth = maxCol - minCol + 1;
    int subHeight = maxRow - minRow + 1;
    QImage image(subWidth, subHeight, QImage::Format_ARGB32);
    QColor backgroundColor(0, 0, 0, 0);
    image.fill(backgroundColor);


    QVector<OmgGeoPoint> geoPoints;
    float transparent = 1.0;
    for (int i = minRow; i <= maxRow; ++i)
    {
        for (int j = minCol; j <= maxCol; ++j)
        {
            transparent = 1.0;
            int index = i * m_width + j;
            if (j < 0)  // If the longitude is minus
            {
                index += m_width;
            }
            if (index < 0 || index >= m_data.size() || invalid(m_data[index]))
            {
                continue;
            }
            float lat = m_left_top_lat - i * m_lat_resolution;
            float lon = m_left_top_lon + j * m_lon_resolution;
            bool isInternal = OmgPolygon::Point_In_Polygon_2D(lon, lat, POL);
            if (isInternal)
            {
                int clrIdx = getColorIndex(m_data[index]);
                OmgGeoPoint pt(lat, lon, m_data[index],
                               m_colorMap[clrIdx].x(),
                               m_colorMap[clrIdx].y(),
                               m_colorMap[clrIdx].z());

                // test. fill black in boundary and white in invalid region.
                if (clrIdx == 0)
                {
                    pt = OmgGeoPoint(lat, lon, m_data[index], 0.0, 0.0, 0.0);
                }
                else if (clrIdx == m_colorMap.size() - 1)
                {
                    pt = OmgGeoPoint(lat, lon, m_data[index], 1.0, 1.0, 1.0);
                    transparent = 0.0;
                }
                //

                geoPoints.push_back(pt);

                // fill pixels
                int irow = i - minRow;
                int icol = j - minCol;
                // QColor pixelColor(m_colorMap[clrIdx].x() * 255, m_colorMap[clrIdx].y() * 255, m_colorMap[clrIdx].z() * 255, 255);
                QColor pixelColor(pt.red() * 255, pt.green() * 255, pt.blue() * 255, transparent * 255);
                image.setPixel(icol, irow, pixelColor.rgba());
            }
        }
    }

    //    radio = subWidth * radio / subHeight;
    image = image.scaled(subWidth, subWidth * radio);
    image.save(m_img_path, "PNG");
    return geoPoints;
}

int OmgRaster::getColorIndex(float value)
{
    if (m_maxValue - m_minValue < 0.0001)
    {
        return m_colorMap.size() / 2;
    }
    return (value - m_minValue) / (m_maxValue - m_minValue) * (m_colorMap.size() - 1);
}

bool OmgRaster::valid(float value)
{
    if (value < -99999.9f)
    {
        return false;
    }
    return true;
}

bool OmgRaster::invalid(float value)
{
    return !valid(value);
}

float OmgRaster::latResolution() const
{
    return m_lat_resolution;
}

float OmgRaster::lonResolution() const
{
    return m_lon_resolution;
}


void OmgRaster::setResImgPath(const QString &path)
{
    m_img_path = path;
}

