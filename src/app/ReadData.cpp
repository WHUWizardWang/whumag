#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include<limits>
#include <cmath>
#include "DataStruct.h"
#include"ReadData.h"
#include <random>


namespace Geomagnetic {


    const double pi = 3.1415926535;
    constexpr double EARTH_RADIUS = 6378137.0; // WGS84椭球体的赤道半径(米)

    // Convert calendar date to Julian Date
    double TimeTrans::calendarDateToJulianDate(int year, int month, int day, int hour, int minute, int second) {
        // The algorithm for converting a calendar date to a Julian Date
        if (month <= 2) {
            year--;
            month += 12;
        }
        int A = year / 100;
        int B = 2 - A + A / 4;

        double JD = floor(365.25 * (year + 4716.0)) + floor(30.6001 * (1.0 + month))
            + day + B - 1524.5 + (hour + (minute + second / 60.0) / 60.0) / 24.0;
        return JD;
    }

    // Convert Julian Date to GPS week and seconds of the week
    void TimeTrans::JulianDateToGPS(double JD, int& gpsWeek, double& gpsSeconds) {
        // GPS epoch start
        const double JD_GPS_epoch = 2444244.5; // January 6, 1980
        double JD_since_epoch = JD - JD_GPS_epoch;

        gpsWeek = static_cast<int>(JD_since_epoch / 7.0);
        gpsSeconds = (JD_since_epoch - static_cast<double>(gpsWeek) * 7.0) * 86400.0;
    }

    // Read the data from file and fill the map
    bool ReadData::readDataFromFile(const std::string& filename, Datapoint& datapoints) {
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Failed to open file." << endl;
            return false;
        }

        std::string line;
        // Skip header
        getline(file, line);

        // Read each line
        while (getline(file, line)) {
            std::istringstream iss(line);
            std::string dateStr, timeStr;
            double mag1, depth1, lon, lat, height;

            if (!(iss >> dateStr >> timeStr >> mag1 >> depth1 >> lon >> lat >> height)) {
                std::cerr << "Error parsing line." << endl;
                continue;  // Skip malformed line
            }

            // Parse date and time
            int month, day, year;
            int hour, minute, second;
            char dummy;

            std::istringstream(dateStr) >> month >> dummy >> day >> dummy >> year;
            year += 2000; // Since the format provided gives only two digits for the year
            std::istringstream(timeStr) >> hour >> dummy >> minute >> dummy >> second;
            TimeTrans timetrans;
            double JD = timetrans.calendarDateToJulianDate(year, month, day, hour, minute, second);
            int gpsWeek;
            double gpsSeconds;
            timetrans.JulianDateToGPS(JD, gpsWeek, gpsSeconds);

            SinglePoint point;
            point.gpsWeek = gpsWeek;
            point.gpsSeconds = gpsSeconds;
            point.tMagnetic = mag1;  // Assuming mag1 corresponds to tMagnetic
            point.lon = lon;
            point.lat = lat;
            point.height = height;

            // Store the point using the GPS seconds as key
            datapoints.insert(std::make_pair(gpsSeconds, point));
        }

        file.close();
        return true;
    }

    bool ReadData::readGridFromFile(const std::string& filename, Datapoint& datapoints)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        std::string line;
        int index = 0;
        int validLines = 0;
        bool hasData = false;

        // 清空现有数据
        datapoints.clear();

        while (getline(file, line))
        {
            // 跳过空行
            if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }

            // 尝试识别分隔符
            bool useComma = (line.find(',') != std::string::npos);

            // 去除可能的引号
            line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());

            double X, Y, tm;
            bool parsedSuccessfully = false;

            if (useComma) {
                // 使用逗号作为分隔符解析
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream iss(line);
                if (iss >> X >> Y >> tm) {
                    parsedSuccessfully = true;
                }
            } else {
                // 默认使用空格作为分隔符
                std::istringstream iss(line);
                if (iss >> X >> Y >> tm) {
                    parsedSuccessfully = true;
                }
            }

            // 如果解析失败，尝试检查是否有制表符
            if (!parsedSuccessfully && line.find('\t') != std::string::npos) {
                std::replace(line.begin(), line.end(), '\t', ' ');
                std::istringstream iss(line);
                if (iss >> X >> Y >> tm) {
                    parsedSuccessfully = true;
                }
            }

            // 最后一次尝试：尝试使用更宽松的解析方式
            if (!parsedSuccessfully) {
                // 移除所有非数字字符（保留小数点和正负号）
                std::string cleanLine;
                bool inNumber = false;
                int numberCount = 0;

                for (char c : line) {
                    if (isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
                        if (!inNumber) {
                            if (numberCount > 0) cleanLine += ' ';
                            inNumber = true;
                        }
                        cleanLine += c;
                    } else {
                        if (inNumber) {
                            inNumber = false;
                            numberCount++;
                        }
                    }
                }

                std::istringstream iss(cleanLine);
                if (iss >> X >> Y >> tm) {
                    parsedSuccessfully = true;
                }
            }

            if (parsedSuccessfully) {
                SinglePoint point;
                point.X = X;
                point.Y = Y;
                point.tMagnetic = tm;
                point.lon = X;
                point.lat = Y;

                datapoints.insert(std::make_pair(index, point));
                validLines++;
                hasData = true;
            } else {
                // 跳过第一行，可能是标题行
                if (index > 0) {
                    std::cerr << "警告: 无法解析第 " << (index + 1) << " 行: " << line << std::endl;
                }
            }

            index++;
        }

        file.close();

        if (!hasData) {
            std::cerr << "文件中未找到有效数据." << std::endl;
            return false;
        }

        std::cout << "成功读取 " << validLines << " 行有效数据（共 " << index << " 行）." << std::endl;
        return true;
    }

    void ReadData::DataSet(Datapoint& datapoint, Datainfo& datainfo)
    {
        datainfo.DataNum = datapoint.size();
        double totalX = 0;
        double totalY = 0;
        double totalLon = 0;
        double totalLat = 0;
        for (const auto& elem : datapoint)
        {
            totalX += elem.second.X;
            totalY += elem.second.Y;
            totalLon += elem.second.lon;
            totalLat += elem.second.lat;
        }
        datainfo.Cx = totalX / datainfo.DataNum;
        datainfo.Cy = totalY / datainfo.DataNum;
        datainfo.Clon = totalLon / datainfo.DataNum;
        datainfo.Clat = totalLat / datainfo.DataNum;
    }
    void ReadData::selectRandomData(const Datapoint& allData, Datapoint& data_sparse, int n) {
        // 设置随机数生成器
        std::random_device rd;
        std::mt19937 g(rd());

        // 准备一个包含所有键的向量
        std::vector<double> keys;
        keys.reserve(allData.size());
        for (const auto& entry : allData) {
            keys.push_back(entry.first);
        }

        // 打乱键的顺序
        shuffle(keys.begin(), keys.end(), g);
        int count = keys.size() / n;
        for (size_t i = 0; i < count && i < keys.size(); ++i) {
            data_sparse[keys[i]] = allData.at(keys[i]);
        }
    }
    void ReadData::selectLineData(const Datapoint& allData, Datapoint& train, int n)
    {
        for (auto& elem : allData)
        {
            if (int(elem.second.Y*10+0.1) % n < 1 || int(elem.second.Y*10-0.1) % n < 1)
            {
				train.insert(elem);
			}
		}
    }
    void ReadData::selectLineData_sub(const Datapoint& allData, Datapoint& train, int n)
    {
        for (auto& elem : allData)
        {
            if (int(elem.second.Y*10+0.1) % n < 1 || int(elem.second.Y*10-0.1) % n < 1)
            {
                train.insert(elem);
            }
        }
    }
    void ReadData::resultOut(const Datapoint& dataresult, const std::string& filename)
    {
        // 打开文件
        std::ofstream outputFile(filename);

        // 检查文件是否成功打开
        if (!outputFile.is_open())
        {
            std::cout << "无法打开文件！" << endl;
            return;
        }

        // 将数据写入文件
        for (const auto& pair : dataresult)
        {
            const SinglePoint& point = pair.second;
            outputFile << point.X << ","
                << point.Y << ","
                << point.tMagnetic << std::endl;
        }

        // 关闭文件
        outputFile.close();
        std::ofstream outputFile1("data.tmp");
        // 将数据写入文件
        for (const auto& pair : dataresult)
        {
            const SinglePoint& point = pair.second;
            outputFile1 << point.X << " "
                << point.Y << " "
                << point.tMagnetic << std::endl;

        }
        // 关闭文件
        outputFile1.close();
        std::cout << "数据已成功写入文件 " << filename << std::endl;
    }
    void ReadData::getDatarowcol(std::vector<double>X, std::vector<double>Y, std::vector<double>T, int& row, int& col,double step_x,double step_y)
    {
        double minX = *std::min_element(X.begin(), X.end());
        double maxX = *std::max_element(X.begin(), X.end());
        double minY = *std::min_element(Y.begin(), Y.end());
        double maxY = *std::max_element(Y.begin(), Y.end());

        // 计算行数和列数
        row = static_cast<int>((maxY - minY) / step_y) + 1;
        col = static_cast<int>((maxX - minX) / step_x) + 1;

    }
    void CoordTrans::BLH2XYZ(Datapoint& datapoint)
    {
        double a = 6378137.0;         // 参考椭球的长半轴, 单位 m
        double b = 6356752.31414;    // 参考椭球的短半轴, 单位 m
        double e2 = (a * a - b * b) / (a * a);
        for (auto& elem : datapoint)
        {
            double B = (elem.second.lat) * pi / 180;
            double L = (elem.second.lon) * pi / 180;
            double H = elem.second.height;
            double N = a / sqrt(1 - e2 * sin(B) * sin(B));
            elem.second.X = (N + H) * cos(B) * cos(L);
            elem.second.Y = (N + H) * cos(B) * sin(L);
            elem.second.Z = (N * (1 - e2) + H) * sin(B);
        }
    }

    void CoordTrans::XYZ2BLH(Datapoint& datapoint)
    {
        double a = 6378137.0;         // 参考椭球的长半轴, 单位 m
        double b = 6356752.31414;    // 参考椭球的短半轴, 单位 m
        double e2 = (a * a - b * b) / (a * a); // 第一偏心率的平方
        double ep2 = (a * a - b * b) / (b * b); // 第二偏心率的平方
        for (auto& elem : datapoint) {
            //根据XY计算经度lon
            SinglePoint temp = elem.second;
            temp.lon = atan2(temp.Y, temp.X) * 180 / 3.1415926535;

            // 计算辅助参数p
            double p = sqrt(temp.X * temp.X + temp.Y * temp.Y);

            // 初始化B的值
            double B = atan2(temp.Z, p * (1 - e2));
            double B0 = 0;
            //迭代计算B
            while (fabs(B - B0) > 1e-12) {
                B0 = B;
                double N = a / sqrt(1 - e2 * sin(B) * sin(B));
                B = atan2(temp.Z + N * e2 * sin(B), p);
            }

            //根据B计算纬度lat
            temp.lat = B * 180 / 3.1415926535;

            //假设已知height，实际中需要根据XYZ计算
            double N = a / sqrt(1 - e2 * sin(B) * sin(B));
            temp.height = p / cos(B) - N;
        }
    }
    void CoordTrans::BL2XY(Datapoint& datapoint)
    {
        double a = 6378137.0;         // 参考椭球的长半轴, 单位 m
        double b = 6356752.31414;    // 参考椭球的短半轴, 单位 m
        double e2 = (a * a - b * b) / (a * a);
        for (auto& elem : datapoint)
        {
            double B = (elem.second.lat) * pi / 180;
            double L = (elem.second.lon) * pi / 180;
            double N = a / sqrt(1 - e2 * sin(B) * sin(B));
            elem.second.X = N * cos(B) * cos(L);
            elem.second.Y = N * cos(B) * sin(L);
        }
    }
    void ReadData::selectLineData(const Datapoint& allData, Datapoint& train, Datapoint& all,int n)
        {
            for (auto& elem : allData)
            {
                if (int(elem.second.Y * 10 + 0.1) % n < 1)
                {
                    train.insert(elem);
                }
                all.insert(elem);
            }
        }

    void ReadData::selectLineData1(const Datapoint& allData, Datapoint& train, Datapoint& test,Datapoint& all)
        {
        int flag =0;
            for (auto& elem : allData)
            {
                flag++;
                if (flag % 10 < 1)
                {
                    train.insert(elem);
                }
                else
                {
                    test.insert(elem);
                }
            }
        }

    //大地坐标转投影坐标
    void ReadData::DadiPoint2ProjectPoint(double B, double L,double &x,double &y)
    {
        //把度转化为弧度
        B = B * pi / 180;
        L = L * pi / 180;

        double N, t, n, c, V, Xz, m1, m2, m3, m4, m5, m6, a0, a2, a4, a6, a8, M0, M2, M4, M6, M8, x0, y0, l;

        int L_num;
        double L_center;

        //中央子午线经度，6°带
        L_num = (int)(L * 180 / pi / 6.0) + 1;
        L_center = 6 * L_num - 3;

        //中央子午线经度，3°带
        //L_num = (int)(L * 180 / pi / 3.0 + 0.5);
        //L_center = 3 * L_num;

        l = (L / pi * 180 - L_center) * 3600; //求带号、中央经线、经差

        M0 = a * (1 - e);
        M2 = 3.0 / 2.0 * e * M0;
        M4 = 5.0 / 4.0 * e * M2;
        M6 = 7.0 / 6.0 * e * M4;
        M8 = 9.0 / 8.0 * e * M6;

        a0 = M0 + M2 / 2.0 + 3.0 / 8.0 * M4 + 5.0 / 16.0 * M6 + 35.0 / 128.0 * M8;
        a2 = M2 / 2.0 + M4 / 2 + 15.0 / 32.0 * M6 + 7.0 / 16.0 * M8;
        a4 = M4 / 8.0 + 3.0 / 16.0 * M6 + 7.0 / 32.0 * M8;
        a6 = M6 / 32.0 + M8 / 16.0;
        a8 = M8 / 128.0;

        Xz = a0 * B - a2 / 2.0 * sin(2 * B) + a4 / 4.0 * sin(4 * B) - a6 / 6.0 * sin(6 * B) + a8 / 8.0 * sin(8 * B);  //计算子午线弧长
        c = a * a / b;
        V = sqrt(1 + e1 * cos(B) * cos(B));
        N = c / V;
        t = tan(B);
        n = e1 * cos(B) * cos(B);

        m1 = N * cos(B);
        m2 = N / 2.0 * sin(B) * cos(B);
        m3 = N / 6.0 * pow(cos(B), 3) * (1 - t * t + n);
        m4 = N / 24.0 * sin(B) * pow(cos(B), 3) * (5 - t * t + 9 * n);
        m5 = N / 120.0 * pow(cos(B), 5) * (5 - 18 * t * t + pow(t, 4) + 14 * n - 58 * n * t * t);
        m6 = N / 720.0 * sin(B) * pow(cos(B), 5) * (61 - 58 * t * t + pow(t, 4));
        x0 = Xz + m2 * l * l / pow(p_0, 2) + m4 * pow(l, 4) / pow(p_0, 4) + m6 * pow(l, 6) / pow(p_0, 6);
        y0 = m1 * l / p_0 + m3 * pow(l, 3) / pow(p_0, 3) + m5 * pow(l, 5) / pow(p_0, 5);   //计算x y坐标

        x = x0;
        //double y = y0 + 500000 + 1000000 * L_num;    //化为国家统一坐标
        y = y0 + 500000;     //化为国家统一坐标
    }

    int ReadData::ReadLines(QStringList FileList,QString savepath)
    {
        Datapoint result;
        std::string s;
        int n = FileList.size();
        if (n<1)
            return -1;
        TimeTrans tt;
        double JD;


        qDebug() << "多源融合数据数量为" << n << endl;

        std::vector<std::string>doc_name;
        for (int i = 0; i < n; i++)
        {
            QString str= FileList.at(i);
            doc_name.push_back(str.toStdString());
        }

        qDebug() << "文件开始读入，请稍候" << endl;
        std::vector<linePoints> lp;
        for (int i = 0; i < n; i++)
        {
            std::ifstream fin(doc_name[i]);
            std::string line = "";

            if (!fin)
            {
                qDebug() << "文件读写失败:" << QString::fromStdString(doc_name[i]) << endl;
            }

            while (getline(fin, line))
            {
                std::string tmp = "";
                linePoints P;

                std::istringstream sline(line);

                //****** 获取原始文件信息 ******
                getline(sline, tmp, ',');
                P.docID = i;

                getline(sline, tmp, ',');
                P.year = stoi(tmp.substr(0, 4));
                P.month = stoi(tmp.substr(5, 2));
                P.day = stoi(tmp.substr(8, 2));

                getline(sline, tmp, ',');
                P.hour = stoi(tmp.substr(0, 2));
                P.min = stoi(tmp.substr(3, 2));
                P.sec = stod(tmp.substr(6, 6));

                getline(sline, tmp, ',');
                P.Tm = stod(tmp);

                getline(sline, tmp, ',');
                P.G = stod(tmp);

                getline(sline, tmp, ',');
                P.depth = stod(tmp);

                getline(sline, tmp, ',');
                P.L = stod(tmp);

                getline(sline, tmp, ',');
                P.B = stod(tmp);

                getline(sline, tmp, ',');
                P.V = stod(tmp);

                getline(sline, tmp, ',');
                P.heading = stod(tmp);

                getline(sline, tmp, ',');
                P.T1 = stod(tmp);

                getline(sline, tmp, ',');
                P.v = stod(tmp);

                getline(sline, tmp, ',');
                P.T2 = stod(tmp);

                getline(sline, tmp, ',');
                P.adjust = stod(tmp);

                getline(sline, tmp, ',');
                P.T3 = stod(tmp);

                //****** 统一数据管理 ******(需要填充算法来处理上面的原始数据)
                //P.gpsWeek = 0.0;
                //P.gpsSeconds = 0.0;
                JD = tt.calendarDateToJulianDate(P.year, P.month, P.day, P.hour, P.min, P.sec);
                tt.JulianDateToGPS(JD, P.gpsWeek, P.gpsSeconds);
                P.xMagnetic = 0.0;
                P.yMagnetic = 0.0;
                P.zMagnetic = 0.0;
                P.tMagnetic = 0.0;
                P.lat = P.B;
                P.lon = P.L;
                P.height = 0.0 - P.depth;
                DadiPoint2ProjectPoint(P.lat, P.lon, P.X, P.Y);
                //P.X = 0.0;
                //P.Y = 0.0;
                P.Z = 0.0;
                P.cluster = P.docID;
                linepoints.push_back(P);
                lp.push_back(P);
            }
            fin.close();
            // QString str = "文件已处理数量：" +QString::number( i + 1) + "/" +QString::number( n) + ",剩余"+ QString::number( n - i - 1);
//            QCoreApplication::processEvents();
//            ui->textBrowser->append("多源融合数据数量为");
            // qDebug() << "文件已处理数量：" << i + 1 << "/" << n << ",剩余" << n - i - 1 << endl;
            // lines.push_back(lp);
        }
        //
//        for (auto &lp:linepoints)
//        {
//            SinglePoint point;
//            point.X = lp.X;
//            point.lat = lp.lat;
//            point.Y = lp.Y;
//            point.lon = lp.lon;
//            point.tMagnetic = lp.tMagnetic;
//            result.insert(make_pair(result.size(), point));
//        }
//        return result;
        QFile file(savepath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "fail" ;
        }
        QTextStream out(&file);
        for (auto& point : linepoints)
        {
            out.setRealNumberPrecision(12);
            out << point.lon<< " " << point.lat<< " " << point.Tm<<Qt::endl;
        }
        file.close();
        return 0;
    }
    bool ReadData::createGridData(const Datapoint& datapoints, Datapoint& dataresult, double dx, double dy)
    {
        // 清空原有数据
        dataresult.clear();
        if (datapoints.empty())
        {
            std::cerr << "无法创建网格数据，原始数据为空！" << std::endl;
            return false;
        }

        // 获得datapoints的经纬度最大值最小值
        double xMax = std::numeric_limits<double>::min();
        double xMin = std::numeric_limits<double>::max();
        double yMax = std::numeric_limits<double>::min();
        double yMin = std::numeric_limits<double>::max();
        for (const auto& elem : datapoints)
        {
            xMax = std::max(xMax, elem.second.X);
            xMin = std::min(xMin, elem.second.X);
            yMax = std::max(yMax, elem.second.Y);
            yMin = std::min(yMin, elem.second.Y);
        }

        // 计算网格点数量
        int nx = static_cast<int>((xMax - xMin) / dx) + 1;
        int ny = static_cast<int>((yMax - yMin) / dy) + 1;

        // 使用整数索引创建网格，避免浮点数精度问题
        for (int i = 0; i < nx; ++i)
        {
            double x = xMin + i * dx;
            for (int j = 0; j < ny; ++j)
            {
                double y = yMin + j * dy;

                SinglePoint point;
                point.X = x;
                point.lat = x;
                point.Y = y;
                point.lon = y;
                point.tMagnetic = 0;
                dataresult.insert(std::make_pair(dataresult.size(), point));
            }
        }

        // 输出调试信息
        std::cout << "网格创建: " << nx << " x " << ny << " = " << dataresult.size() << " 点" << std::endl;
        std::cout << "X范围: " << xMin << " 到 " << xMax << ", 步长: " << dx << std::endl;
        std::cout << "Y范围: " << yMin << " 到 " << yMax << ", 步长: " << dy << std::endl;

        return true;
    }

    Datapoint ReadData::setDataResult(Datapoint& datapoint, double interval)
    {
        Datapoint dataresult;
        double xMax = std::numeric_limits<double>::min();
        double xMin = std::numeric_limits<double>::max();
        double yMax = std::numeric_limits<double>::min();
        double yMin = std::numeric_limits<double>::max();
        for (auto& elem : datapoint)
        {
            double x, y;
            x = elem.second.X*10;
            y = elem.second.Y*10;
            elem.second.X = round(x)/10.0;
            elem.second.Y = round(y)/10.0;
            xMax = std::max(xMax, elem.second.X);
            xMin = std::min(xMin, elem.second.X);
            yMax = std::max(yMax, elem.second.Y);
            yMin = std::min(yMin, elem.second.Y);
        }
        for (double x = xMin; x <= xMax; x += interval)
        {
            for (double y = yMin; y <= yMax; y += interval)
            {
                SinglePoint point;
                point.X = x;
                point.lat = x;
                point.Y = y;
                point.lon = y;
                point.tMagnetic = 0;
                dataresult.insert(std::make_pair(dataresult.size(), point));
            }
        }
        return dataresult;
    }
}

