#include "subarea.h"
using namespace Geomagnetic;
std::pair<int, int> cal_rc(Datapoint &datapoint,double x_step,double y_step)
{
    QVector<double> xx,yy,zz;
    for (const auto elem:datapoint)
    {
        xx.push_back(elem.second.X);
        yy.push_back(elem.second.Y);
        zz.push_back(elem.second.Z);
    }
    double minX = *std::min_element(xx.begin(), xx.end());
    double maxX = *std::max_element(xx.begin(), xx.end());
    double minY = *std::min_element(yy.begin(), yy.end());
    double maxY = *std::max_element(yy.begin(), yy.end());
    int cols = round((maxX-minX)/x_step +1);
    int rows = round((maxY-minY)/y_step +1);
    std::pair<int,int> result(rows,cols);
    return result;
}

double calculateStandardDeviation(const QVector<double>& data) {
    if (data.empty())
    {
        return 0;
    }
    double sum = 0.0;
    for (double value : data)
    {
        sum += value;
    }
    double mean = sum / data.size();
    double sumOfSquares = 0.0;
    for (double value : data)
    {
        sumOfSquares += pow(value - mean, 2);
    }
    double variance = sumOfSquares / data.size(); // 方差
    return sqrt(variance); // 标准差是方差的平方根
}

// 分区并计算每个区的评价指标，创建datapoint_subarea，存储每个区原有的datapoints
void cal_std(Datapoint &datapoint,std::pair<int, int> rc,int window_size,int multiple,
             QVector<QVector<double>> &result,QVector<QVector<Datapoint>> &datapoint_subarea,
             QVector<QVector<Datapoint>> &datapoint_result)
{
    int r = rc.first - 1;
    int c = floor(rc.second/window_size);
    // 填充矩阵
    for (int i = 0; i < r; ++i)
    {
        QVector<double> rows;
        QVector<Datapoint> dp;
        Datapoint temp;
        for (int j = 0; j < c; ++j)
        {
            rows.push_back(0.0);
            dp.push_back(temp);
        }
        result.push_back(rows);
        datapoint_subarea.push_back(dp);
        datapoint_result.push_back(dp);
    }
    //
    for (int i = 0; i < r; ++i)
    {
        for (int j = 0; j < c - 1; ++j)
        {
            QVector<double> temp;
            Datapoint dp_temp;
            int index0 = 0;
            for(int k =0; k < window_size; ++k)
            {
                int index1 = j*window_size  + i*multiple*rc.second + k;
                int index2 = j*window_size  + (i+1)*multiple*rc.second + k;
                temp.push_back(datapoint.at(index1).tMagnetic);
                temp.push_back(datapoint.at(index2).tMagnetic);
                SinglePoint point1, point2;
                point1.X = datapoint.at(index1).X;
                point1.Y = datapoint.at(index1).Y;
                point1.tMagnetic = datapoint.at(index1).tMagnetic;
                point2.X = datapoint.at(index1).X;
                point2.Y = datapoint.at(index1).Y;
                point2.tMagnetic = datapoint.at(index1).tMagnetic;
                dp_temp.insert(std::make_pair(index0, point1));
                dp_temp.insert(std::make_pair(index0+window_size, point2));
                index0 = index0 + 1;
            }
            result[i][j] = calculateStandardDeviation(temp);
            datapoint_subarea[i][j] = dp_temp;
        }
        //
        QVector<double> temp;
        Datapoint dp_temp;
        int index0 = 0;
        for(int k =0; k < rc.second - window_size * (c-1); ++k)
        {

            int index1 = (c-1)*window_size + i*multiple*rc.second + k;
            int index2 = (c-1)*window_size + (i+1)*multiple*rc.second + k;
//            qDebug()<<index1<<"  "<<index2;
            SinglePoint point1, point2;
            point1.X = datapoint.at(index1).X;
            point1.Y = datapoint.at(index1).Y;
            point1.tMagnetic = datapoint.at(index1).tMagnetic;
            point2.X = datapoint.at(index1).X;
            point2.Y = datapoint.at(index1).Y;
            point2.tMagnetic = datapoint.at(index1).tMagnetic;
            temp.push_back(datapoint.at(index1).tMagnetic);
            temp.push_back(datapoint.at(index2).tMagnetic);
            dp_temp.insert(std::make_pair(index0, point1));
            dp_temp.insert(std::make_pair(index0+rc.second - window_size * (c-1), point2));
            index0 = index0 + 1;
        }
        result[i][c-1] = calculateStandardDeviation(temp) ;
        datapoint_subarea[i][c-1] = dp_temp;
    }
    //
    for (int i = 0; i < r; ++i)
    {
        for (int j = 0; j < c; ++j)
        {
            double step = 0.5;
            Datapoint dp_temp;
            int index0 = 0;
            for (int m = 0;m < multiple; ++m)
            {
                for(int k =0; k < window_size; ++k)
                {
                    SinglePoint point;
                    point.X = datapoint_subarea[i][j].at(k).X + step*m;
                    point.Y = datapoint_subarea[i][j].at(k).Y;
                    point.tMagnetic = 0;
                    dp_temp.insert(std::make_pair(index0, point));
                    index0 = index0 + 1;
                }
            }
            datapoint_result[i][j] = dp_temp;
        }
    }
}

std::pair<double, double> findMinMax(const QVector<QVector<double>>& matrix)
{

    // 初始化最小值和最大值为可能的极限值
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    // 遍历矩阵中的所有元素
    for (const auto& row : matrix)
    {
        for (double val : row)
        {
            if (val < minVal)
            {
                minVal = val;
            }
            if (val > maxVal)
            {
                maxVal = val;
            }
        }
    }
    std::pair<int,int> result(minVal,maxVal);
    return result;
}

void create_type(const QVector<QVector<double>>& matrix, QVector<QVector<double>> &result)
{
    int r = matrix.size();
    int c = matrix[0].size();
    // 填充矩阵
    for (int i = 0; i < r; ++i)
    {
        QVector<double> rows;
        for (int j = 0; j < c; ++j)
        {
            rows.push_back(0.0);
        }
        result.push_back(rows);
    }
    std::pair<double, double> minmax = findMinMax(matrix);
    double d1 = (minmax.second - minmax.first)/3+minmax.first;
    double d2 = (minmax.second - minmax.first)/3*2+minmax.first;
    for (int i = 0; i < matrix.size(); ++i)
    {
        for (int j = 0; j < matrix[0].size(); ++j)
        {
            if (matrix[i][j]<d1)
                result[i][j] = 1;
            else if (matrix[i][j]>d1 && matrix[i][j]<d2)
                result[i][j] = 2;
            else if (matrix[i][j]>d2)
                result[i][j] = 3;
        }
    }
}

void subarea_build(Datainfo datainfo,std::pair<int, int> rc,int window_size,int multiple,
           QVector<QVector<double>>& type,QVector<QVector<Datapoint>> &datapoint_subarea,
                   QVector<QVector<Datapoint>> &datapoint_result)
{
    for (int i = 0; i < type.size(); ++i)
    {
        for (int j = 0; j < type[0].size(); ++j)
        {
            if (type[i][j]==1)
            {
                Geomagnetic::Polyhedral poly;
                Geomagnetic::Datapoint tmp_dp = datapoint_subarea[i][j];
                ReadData readdata;
                readdata.DataSet(tmp_dp, datainfo);
                datainfo.PolyQ = 0;
                datainfo.sigma2 = 10.0;
                poly.init(datainfo, tmp_dp);
                poly.ComputeQ(datainfo, tmp_dp);
                poly.ComputeX(datainfo, tmp_dp);
                poly.Result(datainfo, datapoint_result[i][j],tmp_dp);
            }
            else if (type[i][j]==2)
            {
                Geomagnetic::Polyhedral poly;
                Geomagnetic::Datapoint tmp_dp = datapoint_subarea[i][j];
                ReadData readdata;
                readdata.DataSet(tmp_dp, datainfo);
                datainfo.PolyQ = 0;
                datainfo.sigma2 = 10.0;
                poly.init(datainfo, tmp_dp);
                poly.ComputeQ(datainfo, tmp_dp);
                poly.ComputeX(datainfo, tmp_dp);
                poly.Result(datainfo, datapoint_result[i][j],tmp_dp);
            }
            else if (type[i][j]==3)
            {
                Geomagnetic::Polyhedral poly;
                Geomagnetic::Datapoint tmp_dp = datapoint_subarea[i][j];
                ReadData readdata;
                readdata.DataSet(tmp_dp, datainfo);
                datainfo.PolyQ = 0;
                datainfo.sigma2 = 10.0;
                poly.init(datainfo, tmp_dp);
                poly.ComputeQ(datainfo, tmp_dp);
                poly.ComputeX(datainfo, tmp_dp);
                poly.Result(datainfo, datapoint_result[i][j],tmp_dp);
            }
        }
    }
}

void final_merge(QVector<AnoPoint> &ano_pnts, QVector<QVector<Geomagnetic::Datapoint>> &datapoint_result)
{
    for (int i = 0; i < datapoint_result.size(); ++i)
    {
        for (int j = 0; j < datapoint_result[0].size(); ++j)
        {
            Datapoint temp = datapoint_result[i][j];
            for (auto &elem : temp)
            {
                AnoPoint p;
                p.x = elem.second.X;
                p.y = elem.second.Y;
                p.z = elem.second.tMagnetic;
                ano_pnts.push_back(p);
            }
        }
    }
}


void cal_std2(Datapoint &datapoint,std::pair<int, int> rc,int window_size,int multiple,
             QVector<QVector<double>> &result,QVector<QVector<Datapoint>> &datapoint_subarea,
             QVector<QVector<Datapoint>> &datapoint_result)
{
    int r = 5;
    int c = floor(rc.second/window_size);
    // 填充矩阵
    for (int i = 0; i < r; ++i)
    {
        QVector<double> rows;
        QVector<Datapoint> dp;
        Datapoint temp;
        for (int j = 0; j < c; ++j)
        {
            rows.push_back(0.0);
            dp.push_back(temp);
        }
        result.push_back(rows);
        datapoint_subarea.push_back(dp);
        datapoint_result.push_back(dp);
    }
    //
    for (int i = 0; i < r; ++i)
    {
        for (int j = 0; j < c - 1; ++j)
        {
            QVector<double> temp;
            Datapoint dp_temp;
            int index0 = 0;
            for(int k =0; k < window_size; ++k)
            {
                int index1 = j*window_size  + i*multiple*rc.second + k;
                int index2 = j*window_size  + (i+1)*multiple*rc.second + k;
//                qDebug()<<index1<<": "
//                        <<datapoint.at(index1).X<<", "
//                        <<datapoint.at(index1).Y<<", "
//                        <<datapoint.at(index1).tMagnetic;
//                qDebug()<<index2<<": "
//                        <<datapoint.at(index2).X<<", "
//                        <<datapoint.at(index2).Y<<", "
//                       <<datapoint.at(index2).tMagnetic;
                temp.push_back(datapoint.at(index1).tMagnetic);
                temp.push_back(datapoint.at(index2).tMagnetic);
                SinglePoint point1, point2;
                point1.X = datapoint.at(index1).X;
                point1.Y = datapoint.at(index1).Y;
                point1.tMagnetic = datapoint.at(index1).tMagnetic;
                point2.X = datapoint.at(index1).X;
                point2.Y = datapoint.at(index1).Y;
                point2.tMagnetic = datapoint.at(index1).tMagnetic;
                dp_temp.insert(std::make_pair(index0, point1));
                dp_temp.insert(std::make_pair(index0+window_size, point2));
                index0 = index0 + 1;
            }
            result[i][j] = calculateStandardDeviation(temp);
            datapoint_subarea[i][j] = dp_temp;
        }
        //
        QVector<double> temp;
        Datapoint dp_temp;
        int index0 = 0;
        for(int k =0; k < rc.second - window_size * (c-1); ++k)
        {

            int index1 = (c-1)*window_size + i*multiple*rc.second + k;
            int index2 = (c-1)*window_size + (i+1)*multiple*rc.second + k;
//            qDebug()<<index1<<"  "<<index2;
            SinglePoint point1, point2;
            point1.X = datapoint.at(index1).X;
            point1.Y = datapoint.at(index1).Y;
            point1.tMagnetic = datapoint.at(index1).tMagnetic;
            point2.X = datapoint.at(index1).X;
            point2.Y = datapoint.at(index1).Y;
            point2.tMagnetic = datapoint.at(index1).tMagnetic;
            temp.push_back(datapoint.at(index1).tMagnetic);
            temp.push_back(datapoint.at(index2).tMagnetic);
            dp_temp.insert(std::make_pair(index0, point1));
            dp_temp.insert(std::make_pair(index0+rc.second - window_size * (c-1), point2));
            index0 = index0 + 1;
        }
        result[i][c-1] = calculateStandardDeviation(temp) ;
        datapoint_subarea[i][c-1] = dp_temp;
    }
    //
    for (int i = 0; i < r; ++i)
    {
        for (int j = 0; j < c; ++j)
        {
            double step = 0.5;
            Datapoint dp_temp;
            int index0 = 0;
            for (int m = 0;m < multiple; ++m)
            {
                for(int k =0; k < window_size; ++k)
                {
                    SinglePoint point;
                    point.X = datapoint_subarea[i][j].at(k).X + step*m;
                    point.Y = datapoint_subarea[i][j].at(k).Y;
                    point.tMagnetic = 0;
                    dp_temp.insert(std::make_pair(index0, point));
                    index0 = index0 + 1;
                }
            }
            datapoint_result[i][j] = dp_temp;
        }
    }
}
