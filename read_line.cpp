
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cmath>
#include "ReadData.h"
using namespace std;
using namespace Geomagnetic;



int main()
{
    string s;
    int n = 2;
    TimeTrans tt;
    double JD;
    cout << "多源融合数据数量为" << n << endl;
    vector<linePoints>linepoints;
    vector<string>doc_name;

    for (int i = 0; i < n; i++)
    {
        cout << "请输入 融合数据路径" << i + 1 << ": " << endl;
        cin >> s;
        doc_name.push_back(s);
    }
    cout << "文件开始读入，请稍候" << endl;

    for (int i = 0; i < n; i++)
    {
        ifstream fin(doc_name[i]);
        string line = "";

        if (!fin)
        {
            cout << "文件读写失败:" << doc_name[i] << endl;
        }

        while (getline(fin, line))
        {
            string tmp = "";
            linePoints P;

            istringstream sline(line);

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
        }
        fin.close();
        cout << "文件已处理数量：" << i + 1 << "/" << n << ",剩余" << n - i - 1 << endl;
    }
    
    /*for (int i = 0; i < linepoints.size(); i++)
    {
        cout << linepoints[i].docID << ","
            << linepoints[i].year << "-"
            << setw(2) << setfill('0') << linepoints[i].month << "-"
            << setw(2) << setfill('0') << linepoints[i].day << ","
            << setw(2) << setfill('0') << linepoints[i].hour << ":"
            << setw(2) << setfill('0') << linepoints[i].min << ":"
            << setw(2) << setfill('0') << fixed << setprecision(3) << linepoints[i].sec << ","
            << fixed << setprecision(3) << linepoints[i].Tm << ","
            << fixed << setprecision(3) << linepoints[i].G << ","
            << fixed << setprecision(3) << linepoints[i].depth << ","
            << fixed << setprecision(7) << linepoints[i].L << ","
            << fixed << setprecision(7) << linepoints[i].B << ","
            << fixed << setprecision(2) << linepoints[i].V << ","
            << fixed << setprecision(3) << linepoints[i].heading << ","
            << fixed << setprecision(3) << linepoints[i].T1 << ","
            << fixed << setprecision(3) << linepoints[i].v << ","
            << fixed << setprecision(3) << linepoints[i].T2 << ","
            << fixed << setprecision(3) << linepoints[i].adjust << ","
            << fixed << setprecision(3) << linepoints[i].T3 << endl;
        cout << "X:" << linepoints[i].X << "-" << "Y:" << linepoints[i].Y << endl;
        cout << "GPSweek:" << linepoints[i].gpsWeek << "-" << "GPSsecond:" << linepoints[i].gpsSeconds << endl;
    }*/

    return 0;
}
