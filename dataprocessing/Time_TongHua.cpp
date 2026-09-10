#include "Time_TongHua.h"

TimeTongHua::TimeTongHua()
{
}

TimeTongHua::~TimeTongHua()
{
}

void TimeTongHua::ReadData(QString FileName)
{
    // 1. 清空上次读取的数据
    data_real.clear();

    // 2. 打开文件
    QFile file(FileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "TimeTongHua::ReadData – 无法打开文件：" << FileName
                   << "，错误信息：" << file.errorString();
        return;  // 打开失败立即返回
    }


    // 3. 逐行读取，拆分并转换
    QTextStream in(&file);
    int lineNo = 0;
    while (!in.atEnd()) {
        ++lineNo;
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;  // 跳过空行
        }

        // 按空格或逗号拆分成三个字段
        QStringList fields = line.split(QRegExp("\\s+|,"), QString::SkipEmptyParts);
        if (fields.size() != 3) {
            qWarning() << QString("TimeTongHua::ReadData – 文件 %1 第 %2 行格式不正确（不是 3 列）：\"%3\"")
                              .arg(FileName).arg(lineNo).arg(line);
            continue;
        }

        // 尝试把三个字段转换为 double
        bool okLon = false, okLat = false, okMag = false;
        double lon = fields[0].toDouble(&okLon);
        double lat = fields[1].toDouble(&okLat);
        double mag = fields[2].toDouble(&okMag);
        if (!okLon || !okLat || !okMag) {
            qWarning() << QString("TimeTongHua::ReadData – 文件 %1 第 %2 行数值转换失败：\"%3\"")
                              .arg(FileName).arg(lineNo).arg(line);
            continue;
        }

        // 4. 构造 xPoint 并添加到 data_real
        xPoint temp;
        temp.lon = lon;
        temp.lat = lat;
        temp.mag = mag;
        data_real.append(temp);
    }

    file.close();

    // 5. 如果完全没有读取到有效数据，再次提示
    if (data_real.isEmpty()) {
        qWarning() << "TimeTongHua::ReadData – 文件" << FileName << "未读取到任何有效数据。";
    }
}

void TimeTongHua::GetIGRFData(int useGeoid,double height)
{
    int length = data_real.size();
    if (length <= 0) {
        // 没有读取到任何实测点，直接返回
        return;
    }
    int maxThreads = 6;
    omp_set_num_threads(maxThreads);
    // ------------------------
    // 一、清理旧数据并预分配空间
    // ------------------------
    data_igrf0.clear();
    data_igrf1.clear();
    data_igrf0.reserve(length);
    data_igrf1.reserve(length);

    // ------------------------
    // 二、提前计算“十进制年”值
    // ------------------------
    // date0、date1 是类成员，表示起始/结束日期
    double decimalYear0 = date0.year() + date0.dayOfYear() / static_cast<double>(date0.daysInYear());
    double decimalYear1 = date1.year() + date1.dayOfYear() / static_cast<double>(date1.daysInYear());

    // ------------------------
    // 三、用 QVector 代替裸指针数组
    // ------------------------
    // CoordGeodeticArr、UserDateArr0/1 都改为 QVector，底层是连续内存，可直接调用 omg_igrf(...)
    QVector<MAGtype_CoordGeodetic> coordArr(length);
    QVector<MAGtype_Date>       dateArr0(length);
    QVector<MAGtype_Date>       dateArr1(length);

    // 填充 coordArr 与 dateArr0/dateArr1
#pragma omp parallel for schedule(static)
    for (int i = 0; i < length; ++i) {
        // 取出第 i 个实测点
        const xPoint &pt = data_real[i];

        // （1）填充地理坐标与高度信息
        MAGtype_CoordGeodetic &coord = coordArr[i];
        coord.phi               = pt.lat;      // 纬度
        coord.lambda            = pt.lon;      // 经度
        coord.UseGeoid          = useGeoid;    // 是否使用大地水准面
        coord.HeightAboveGeoid  = height;      // 传入的观测高度
        if (!useGeoid) {
            // 若不使用大地水准面，就把 HeightAboveEllipsoid 也设成同样高度
            coord.HeightAboveEllipsoid = height;
        }

        // （2）填充起始日期与结束日期信息（十进制年 + 年/月/日）
        MAGtype_Date &ud0 = dateArr0[i];
        ud0.DecimalYear = decimalYear0;
        ud0.Year        = date0.year();
        ud0.Month       = date0.month();
        ud0.Day         = date0.day();

        MAGtype_Date &ud1 = dateArr1[i];
        ud1.DecimalYear = decimalYear1;
        ud1.Year        = date1.year();
        ud1.Month       = date1.month();
        ud1.Day         = date1.day();
    }

    // ------------------------
    // 四、分配用于存储 IGRF 计算结果的 QVector
    // ------------------------
    QVector<MAGtype_GeoMagneticElements> geoArr0(length), errArr0(length);
    QVector<MAGtype_GeoMagneticElements> geoArr1(length), errArr1(length);

    // ------------------------
    // 五、调用 omg_igrf 进行批量计算
    //    （一次传入整个 coordArr/ dateArr0 数组，
    //     接收在 geoArr0/ errArr0 中；第二次传入 dateArr1，结果在 geoArr1/ errArr1）
    // ------------------------
    omg_igrf(coordArr.data(), dateArr0.data(), geoArr0.data(), errArr0.data(), length);
    omg_igrf(coordArr.data(), dateArr1.data(), geoArr1.data(), errArr1.data(), length);

    // ------------------------
    // 六、提取 F 分量（磁力强度）到 data_igrf0/data_igrf1
    // ------------------------
    for (int i = 0; i < length; ++i) {
        data_igrf0.append(geoArr0[i].F);
        data_igrf1.append(geoArr1[i].F);
    }
}

void TimeTongHua::CalMag(QString FileName,int useGeoid,double height,QDate date00,QDate date11,QString dir)
{
    date0 = date00;
    date1 = date11;
    ReadData(FileName);
    GetIGRFData(useGeoid,height);
    for (int i = 0; i < data_real.size(); ++i)
    {
        xPoint temp;
        temp.lat = data_real[i].lat;
        temp.lon = data_real[i].lon;
        temp.mag = - data_igrf0[i] + data_igrf1[i] + data_real[i].mag;
        data_tonghua.append(temp);
    }
    out2file(dir);
}

void TimeTongHua::out2file(QString dir)
{
    QFile file(dir);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "无法打开文件进行写入：" << file.errorString();
        }
    QTextStream out(&file);
    for(auto elem:data_tonghua)
    {
        out<<elem.lon<<" "<<elem.lat<<" "<<elem.mag<<Qt::endl;
    }
}
