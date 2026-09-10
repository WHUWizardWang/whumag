#include "omgqmlpolygon.h"

OmgQmlPolygon::OmgQmlPolygon(QObject *parent)
{
    m_raster_emag2.setResImgPath(g_image_path);
    m_raster_mamea.setResImgPath(g_image_path);
    qDebug() << "=== 数据加载调试 ===";
    qDebug() << "EMAG2 数据路径:" << g_emag2_path;
    qDebug() << "MAMEA 数据路径:" << g_mamea_path;
    qDebug() << "图像输出路径:" << g_image_path;

    // 检查文件是否存在
    if (!QFile::exists(g_emag2_path)) {
        qWarning() << "❌ EMAG2 数据文件不存在!";
    } else {
        qDebug() << "✅ EMAG2 数据文件存在";
    }

    if (!QFile::exists(g_mamea_path)) {
        qWarning() << "❌ MAMEA 数据文件不存在!";
    } else {
        qDebug() << "✅ MAMEA 数据文件存在";
    }

    bool emag2_loaded = m_raster_emag2.loadFromImage(g_emag2_path);
    bool mamea_loaded = m_raster_mamea.loadFromPoints(g_mamea_path);

    qDebug() << "EMAG2 加载结果:" << (emag2_loaded ? "成功" : "失败");
    qDebug() << "MAMEA 加载结果:" << (mamea_loaded ? "成功" : "失败");

    m_lat_rsl = m_raster_emag2.latResolution();
    m_lon_rsl = m_raster_emag2.lonResolution();

    qDebug() << "纬度分辨率:" << m_lat_rsl;
    qDebug() << "经度分辨率:" << m_lon_rsl;
    qDebug() << "=== 数据加载调试结束 ===";
}

OmgQmlPolygon::OmgQmlPolygon(const OmgQmlPolygon &polygon)
{
    m_lat_rsl = polygon.m_lat_rsl;
    m_lon_rsl = polygon.m_lon_rsl;
    m_points = polygon.m_points;

    m_raster_emag2 = polygon.m_raster_emag2;
    m_raster_mamea = polygon.m_raster_mamea;
}

OmgQmlPolygon::~OmgQmlPolygon()
{

}

qreal OmgQmlPolygon::latRsl() const
{
    return m_lat_rsl;
}

qreal OmgQmlPolygon::lonRsl() const
{
    return m_lon_rsl;
}

qreal OmgQmlPolygon::maxLat() const
{
    return m_max_lat;
}

qreal OmgQmlPolygon::minLon() const
{
    return m_min_lon;
}

const QVariantList &OmgQmlPolygon::points() const
{
    return m_points;
}

void OmgQmlPolygon::setLatRsl(qreal lat_rsl)
{
    m_lat_rsl = lat_rsl;
}

void OmgQmlPolygon::setLonRsl(qreal lon_rsl)
{
    m_lon_rsl = lon_rsl;
}

void OmgQmlPolygon::setMaxLat(qreal max_lat)
{
    m_max_lat = max_lat;
}

void OmgQmlPolygon::setMinLon(qreal min_lon)
{
    m_min_lon = min_lon;
}

void OmgQmlPolygon::setPoints(const QVariantList &pnts)
{
    m_points = pnts;
}

void OmgQmlPolygon::addPoint(const OmgQmlPoint &pnt)
{
    m_points.append(QVariant::fromValue(pnt));
}

void OmgQmlPolygon::clear()
{
    m_points.clear();
}

OmgQmlPoint OmgQmlPolygon::pointConvertToQml(const OmgGeoPoint &geoPnt)
{
    OmgQmlPoint pt(geoPnt.lat(), geoPnt.lon(), geoPnt.red(), geoPnt.green(), geoPnt.blue());
    return pt;
}

void OmgQmlPolygon::addNode(qreal lat, qreal lon)
{
    Vec2d pt(lon, lat);
    m_polygon.append(pt);
}

void OmgQmlPolygon::clearNodes()
{
    m_polygon.clear();
}

int OmgQmlPolygon::pointCount()
{
    return m_points.size();
}

void OmgQmlPolygon::getInertnalPoints(int flag)
{

    qDebug() << "=== getInertnalPoints 开始调试 ===";
    qDebug() << "Flag:" << flag << (flag == 0 ? "(EMAG2)" : "(MAMEA)");
    qDebug() << "输入多边形点数:" << m_polygon.size();

    // 打印所有多边形顶点
    for (int i = 0; i < m_polygon.size(); ++i) {
        qDebug() << "点" << i << ": 经度=" << m_polygon[i].x << ", 纬度=" << m_polygon[i].y;
    }

    calibrateLon(m_polygon);

    qDebug() << "经度校准后的多边形:";
    for (int i = 0; i < m_polygon.size(); ++i) {
        qDebug() << "校准后点" << i << ": 经度=" << m_polygon[i].x << ", 纬度=" << m_polygon[i].y;
    }
    //
    double minX = 9999.9999;
    double maxX = -9999.9999;
    double minY = 9999.9999;
    double maxY = -9999.9999;
    for (auto iter = m_polygon.cbegin(); iter != m_polygon.cend(); ++iter)
    {
        minX = qMin(minX, iter->x);
        maxX = qMax(maxX, iter->x);
        minY = qMin(minY, iter->y);
        maxY = qMax(maxY, iter->y);
    }

    qDebug() << "计算的边界:";
    qDebug() << "经度范围:" << minX << "到" << maxX;
    qDebug() << "纬度范围:" << minY << "到" << maxY;
    qDebug() << "Radio:" << m_radio;

    // x is lon, y is lat.
    m_max_lat = maxY;
    m_min_lon = minX;

    if (m_polygon.size() < 3)
    {
        return;
    }
    QVector<OmgGeoPoint> geoPnts;
    switch (flag)
    {
        case 0: // emag2
            qDebug() << "开始获取 EMAG2 内部点...";
            geoPnts = m_raster_emag2.getInternalPoints(m_polygon, m_radio);
            qDebug() << "EMAG2 获取到" << geoPnts.size() << "个点";
            break;
        case 1: // mamea
            qDebug() << "开始获取 MAMEA 内部点...";
            geoPnts = m_raster_mamea.getInternalPoints(m_polygon, m_radio);
            qDebug() << "MAMEA 获取到" << geoPnts.size() << "个点";
            break;
        default:
            qWarning() << "❌ 不支持的 flag:" << flag;
            return;
    }


    if (geoPnts.isEmpty()) {
        qWarning() << "❌ 没有获取到任何有效数据点！";
        qWarning() << "可能原因：";
        qWarning() << "1. 选择区域超出数据范围";
        qWarning() << "2. 数据文件损坏或格式错误";
        qWarning() << "3. 多边形区域太小";
        return;
    }
    // 检查前几个点的数据
    qDebug() << "前5个数据点:";
    for (int i = 0; i < qMin(5, geoPnts.size()); ++i) {
        qDebug() << "点" << i << ": lat=" << geoPnts[i].lat()
                 << ", lon=" << geoPnts[i].lon()
                 << ", R=" << geoPnts[i].red()
                 << ", G=" << geoPnts[i].green()
                 << ", B=" << geoPnts[i].blue();
    }


    m_pnts = geoPnts;
    m_points.clear();
    for (auto iter = geoPnts.cbegin(); iter != geoPnts.cend(); ++iter)
    {
        addPoint(pointConvertToQml(*iter));
    }
}

qreal OmgQmlPolygon::pointLat(int index)
{
    return m_pnts[index].lat();
}

qreal OmgQmlPolygon::pointLon(int index)
{
    return m_pnts[index].lon();
}

qreal OmgQmlPolygon::pointRed(int index)
{
    return m_pnts[index].red();
}

qreal OmgQmlPolygon::pointGreen(int index)
{
    return m_pnts[index].green();
}

qreal OmgQmlPolygon::pointBlue(int index)
{
    return m_pnts[index].blue();
}

void OmgQmlPolygon::setRadio(qreal radio)
{
    m_radio = radio;
}

void OmgQmlPolygon::calibrateLon(QVector<Vec2d> &POL)
{
    bool isNeed = false;
    for (int i = 0; i < POL.size() - 1; ++i)
    {
        if (POL[i].x * POL[i + 1].x < 0.0f && qAbs(POL[i].x) + qAbs(POL[i + 1].x) > 180.0f)
        {
            isNeed = true;
            break;
        }
    }

    if (isNeed)
    {
        for (auto iter = POL.begin(); iter != POL.end(); ++iter)
        {
            if (iter->x < 0.0f)
            {
                iter->x = iter->x + 360.0f;
            }
        }
    }
}

QString OmgQmlPolygon::getResImgPath()
{
    QDir appDir(QCoreApplication::applicationDirPath());

    // 确保 images 目录存在
    if (!appDir.exists("images")) {
        appDir.mkpath("images");
    }

    QString absolutePath = appDir.absoluteFilePath("images/region.png");
    return "file:///" + absolutePath;
}

QString OmgQmlPolygon::loadChinaBorder(){
    QString file_path = ":/Map/China.json";
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << file.errorString();
        return "";
    }
    QTextStream in(&file);
    QString jsonString = in.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parsing error at offset" << parseError.offset << ":" << parseError.errorString();
        return "";
    }

    QJsonObject jsonObject = jsonDoc.object();
    QJsonValue jsonValue = jsonObject.value("features");
    jsonObject = jsonValue[0].toVariant().toJsonObject();
    jsonValue = jsonObject.value("geometry");
    jsonObject = jsonValue.toVariant().toJsonObject();
    jsonString = QString(QJsonDocument(jsonObject).toJson());
    jsonString = jsonString.remove('\n').remove('\r').remove('\t');
    return jsonString;
}

void OmgQmlPolygon::addVesselPath(const VesselPath &vessel_path){
    QString json_str =  vessel_path.toJsonString();
    emit sigAddVesselPath(json_str);
}

void OmgQmlPolygon::removeVesselPath(const QString &vessel_name){
    emit sigRemoveVesselPath(vessel_name);
}

QString VesselPath::toJsonString() const {
    QJsonObject js_obj;
    QJsonValue js_val = QJsonValue(this->name);
    js_obj.insert("name", js_val);
    js_val = QJsonValue(this->color);
    js_obj.insert("color",js_val);
    js_val = QJsonValue(this->width);
    js_obj.insert("width",js_val);
    QJsonArray js_arr;
    QJsonArray js_pos;
    for(int i=0;i<this->pos.size();++i){
        // Clear QJsonArray
        while(js_pos.size()>0){
            js_pos.pop_back();
        }
        js_val = QJsonValue(this->pos[i].latitude());
        js_pos.push_back(js_val);
        js_val = QJsonValue(this->pos[i].longitude());
        js_pos.push_back(js_val);

        js_arr.push_back(js_pos);
    }
    js_obj.insert("coordinates",js_arr);
    QJsonDocument js_doc(js_obj);
    QByteArray js_ba = js_doc.toJson();
    QString js_str = QString::fromUtf8(js_ba);
    return js_str;
}
