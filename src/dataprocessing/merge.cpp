#include "merge.h"

rongHe::rongHe()
{
}

rongHe::~rongHe()
{
}


double d2r(double deg)
{
	double rad;
	rad = deg / 180.0 * M_PI;
	return rad;
}

void TongJi(std::vector<double>dt, std::vector<double>down)
{
	// 计算差值
	std::vector<double> delt;
	std::transform(dt.begin(), dt.end(), down.begin(), std::back_inserter(delt),
		[](double a, double b) { return a - b; });

	// 计算统计数据
	double max_dt = *std::max_element(dt.begin(), dt.end());
	double min_dt = *std::min_element(dt.begin(), dt.end());
	double sum_dt = std::accumulate(dt.begin(), dt.end(), 0.0);
	double mean_dt = sum_dt / dt.size();
	double sum_sq_diff_dt = std::inner_product(dt.begin(), dt.end(), dt.begin(), 0.0,
		[](double sum, double diff) { return sum + diff * diff; },
		[](double a, double b) { return a + b; });
	double std_dt = std::sqrt(sum_sq_diff_dt / dt.size());

	double max_down = *std::max_element(down.begin(), down.end());
	double min_down = *std::min_element(down.begin(), down.end());
	double sum_down = std::accumulate(down.begin(), down.end(), 0.0);
	double mean_down = sum_down / down.size();
	double sum_sq_diff_down = std::inner_product(down.begin(), down.end(), down.begin(), 0.0,
		[](double sum, double diff) { return sum + diff * diff; },
		[](double a, double b) { return a + b; });
	double std_down = std::sqrt(sum_sq_diff_down / down.size());

	double max_delt = *std::max_element(delt.begin(), delt.end());
	double min_delt = *std::min_element(delt.begin(), delt.end());
	double sum_delt = std::accumulate(delt.begin(), delt.end(), 0.0);
	double mean_delt = sum_delt / delt.size();
	double sum_sq_diff = std::inner_product(delt.begin(), delt.end(), delt.begin(), 0.0,
		[](double sum, double diff) { return sum + diff * diff; },
		[](double a, double b) { return a + b; });
	double std_delt = std::sqrt(sum_sq_diff / delt.size());

	// 输出统计数据
	std::cout << "Statistics of the difference between theoretical and computed values:\n";
	std::cout << "Max_dongfang: " << max_dt << ", Min_dongfang: " << min_dt << ", Mean_dongfang: " << mean_dt << ", Std Dev_dongfang: " << std_dt << std::endl;
	std::cout << "Max_FusedData: " << max_down << ", Min_FusedData: " << min_down << ", Mean_FusedData: " << mean_down << ", Std Dev_FusedData: " << std_down << std::endl;
    qDebug() << "Max_delt: " << max_delt << ", Min_delt: " << min_delt << ", Mean_delt: " << mean_delt << ", Std Dev_delt: " << std_delt;

}

// p1代表反映距离远近因素影响的权因子，下面是计算p1的权函数 （a为已知的信息，b为待计算的格网点）
double rongHe::calculate_p1(Point a, Point b, double delt_phi0, double delt_lamda0)
{
	delt_phi0 = delt_phi0 * 60.0 / 2.0;       // 内插数据窗口在纬度方向上的半宽度，单位取角分
	delt_lamda0 = delt_lamda0 * 60.0 / 2.0;   // 内插数据窗口在经度方向上的半宽度，单位取角分

	double e = 0.01;                    // 为防止权函数分母趋于0而加上的小正数

	double p1;
	double delt_phi = 0.0;
	double delt_lamda = 0.0;
	double d = 0.0;
	double D = 0.0;

	delt_phi = abs(a.B - b.B) * 60.0;
	delt_lamda = abs(a.L - b.L) * 60.0;

	if ((delt_lamda >= 0 && delt_lamda <= (delt_lamda0 / 3)) && (delt_phi >= 0 && delt_phi <= (delt_phi0 / 3)))
	{
		d = sqrt(delt_phi * delt_phi + delt_lamda * delt_lamda * cos(d2r(b.B)) * cos(d2r(b.B)));
		p1 = 1.0 / (d + e) / (d + e);
		return p1;
	}
	if (delt_lamda > delt_lamda0 || delt_phi > delt_phi0)
	{
		p1 = 0.0;
		return p1;
	}
	else
	{
		d = sqrt(delt_phi * delt_phi + delt_lamda * delt_lamda * cos(d2r(b.B)) * cos(d2r(b.B)));
		D = sqrt(delt_phi0 * delt_phi0 + delt_lamda0 * delt_lamda0 * cos(d2r(b.B)) * cos(d2r(b.B)));
		p1 = pow(27.0 / 4.0 / D * (d / D - 1.0), 2);
		return p1;
	}
}

double rongHe::calculate_p2(double m)
{
	double p2;
	p2 = 1.0 / m / m;
	return p2;
}

void rongHe::readfile0(std::string s)
{
    std::ifstream fin(s);
    std::string line = "";
	double m;
	while (getline(fin, line))
	{
        std::string tmp = "";
		Point point;
		point.B = 0.0;
		point.L = 0.0;
		point.T = 0.0;
        std::istringstream sline(line);
		getline(sline, tmp, ' ');// 读入X
		point.L = stod(tmp);
		getline(sline, tmp, ' ');// 读入Y
		point.B = stod(tmp);
		getline(sline, tmp, ' ');// 读入T
		point.T = stod(tmp);
		point.m = 0.0;
		point0.push_back(point);
	}
	fin.close();
}

void rongHe::readfile()
{
    for (int i = 0; i < doc.size(); i++)
	{
        std::ifstream fin(doc[i]);
        std::string line = "";

		while (getline(fin, line))
		{
            std::string tmp = "";
			Point point;
			point.B = 0.0;
			point.L = 0.0;
			point.T = 0.0;
            std::istringstream sline(line);
			getline(sline, tmp, ' ');// 读入X
			point.L = stod(tmp);
			getline(sline, tmp, ' ');// 读入Y
			point.B = stod(tmp);
			getline(sline, tmp, ' ');// 读入T
			point.T = stod(tmp);
			point.m = doc_m[i];
			doc_P.push_back(point);
		}
		doc_points.push_back(doc_P);
		fin.close();
	}

    qDebug() << "文件读入完成" << endl;
}

//汇总多源数据
void rongHe::allPoints(std::vector<std::vector<Point>>a,std::vector<Point>&all_points)
{
	for (int i = 0; i < a.size(); i++)
	{
		for (int j = 0; j < a[i].size(); j++)
		{
			Point point;
			point = a[i][j];
			all_points.push_back(point);
		}
	}

    std::cout << "数据汇总完成" << endl;
}

std::vector<Point>rongHe::calModel(double delt_phi0 ,double delt_lamda0)
{
    std::cout << "内插处理开始" << endl;
	//构建内插模型

	for (int j = 0; j < data.size(); j++)
	{
		double sum1 = 0.0;
		double sum2 = 0.0;

		for (int i = 0; i < all_point.size(); i++)
		{
			p1 = calculate_p1(all_point[i], data[j], delt_phi0, delt_lamda0);
			p2 = calculate_p2(all_point[i].m);
			sum1 = sum1 + p1 * p2 * all_point[i].T;
			sum2 = sum2 + p1 * p2;
		}

		data[j].T = sum1 / sum2;
	}
	return data;

    std::cout << "内插处理完成" << endl;
}

void rongHe::evaluatePrecision(std::vector<Point>a, std::vector<Point>b)
{
	for (int i = 0; i < a.size(); i++)
	{
		double T_1;
		double T_2;
		T_1 = a[i].T;
		T_2 = b[i].T;
		T1.push_back(T_1);
		T2.push_back(T_2);
	}
	TongJi(T1, T2);
}
void rongHe::createMap(double min_B, double min_L, double max_B, double max_L,double Bint,double Lint)
{
    //获取内插区域中心点坐标
    double B_half = (min_B + max_B) / 2;
    double L_half = (min_L + max_L) / 2;

    //初始化内插区域边界
    double B1 = B_half - Bint;
    double B2 = B_half + Bint;
    double L1 = L_half - Lint;
    double L2 = L_half + Lint;

    //根据中心点扩展生成容器
    int i1 = 1;
    while (B1 > min_B)
    {
        B1 = B1 - Bint;
        i1 = i1 + 1;
    }
    int i2 = 1;
    while (B2 < max_B)
    {
        B2 = B2 + Bint;
        i2 = i2 + 1;
    }
    int row = i1 + i2 + 1;//记录内插容器行数
    int i3 = 1;
    while (L1 > min_L)
    {
        L1 = L1 - Lint;
        i3 = i3 + 1;
    }
    int i4 = 1;
    while (L2 < max_L)
    {
        L2 = L2 + Lint;
        i4 = i4 + 1;
    }
    int col = i3 + i4 + 1;//记录内插容器列数

    Point point;
    for (int i = 0; i < row ; i++)
    {
        for (int j = 0; j < col; j++)
        {
            point.B = B1 + i * Bint;
            point.L = L1 + j * Lint;
            point.T = 0.0;
            point.m = 0.0;
            point0.push_back(point);
        }
    }
}
void rongHe::createMap2(double min_B, double min_L, double max_B, double max_L,double Bint,double Lint)
{

    //初始化内插区域边界
    double B1 = Bint;
    double B2 = Bint;
    double L1 = Lint;
    double L2 = Lint;

    //根据中心点扩展生成容器
    int i1 = 1;
    while (B1 > min_B)
    {
        B1 = B1 - Bint;
        i1 = i1 + 1;
    }
    int i2 = 1;
    while (B2 < max_B)
    {
        B2 = B2 + Bint;
        i2 = i2 + 1;
    }
    int row = i1 + i2 + 1;//记录内插容器行数
    int i3 = 1;
    while (L1 > min_L)
    {
        L1 = L1 - Lint;
        i3 = i3 + 1;
    }
    int i4 = 1;
    while (L2 < max_L)
    {
        L2 = L2 + Lint;
        i4 = i4 + 1;
    }
    int col = i3 + i4 + 1;//记录内插容器列数

    Point point;
    for (int i = 0; i < row ; i++)
    {
        for (int j = 0; j < col; j++)
        {
            point.B = B1 + i * Bint;
            point.L = L1 + j * Lint;
            point.T = 0.0;
            point.m = 0.0;
            point0.push_back(point);
        }
    }
}
void rongHe::rongHe_run(std::string infile0, double delt_phi0, double delt_lamda0)
{
	// *******  读入文件  ******
	//第一个传入的文件需要作为插值数据的格网模板

    readfile0(infile0);
	//传入待融合的多源数据文件
    readfile();

	//***** 数据融合 *****
	data = point0;  //同步模板格网平面坐标信息

	//汇总多源数据
	allPoints(doc_points, all_point);

	//构建内插模型
	data = calModel(delt_phi0, delt_lamda0);

	//***** 精度评估 *****
	evaluatePrecision(point0, data);

    std::cout << "融合程序执行完毕" << endl;
}


void rongHe::rongHe_run2(double delt_phi0, double delt_lamda0,
                         double min_B, double min_L, double max_B, double max_L,double Bint,double Lint)
{

    //传入待融合的多源数据文件
    createMap(min_B, min_L, max_B, max_L,Bint,Lint);
    readfile();

    //***** 数据融合 *****
    data = point0;  //同步模板格网平面坐标信息

    //汇总多源数据
    allPoints(doc_points, all_point);

    //构建内插模型
    data = calModel(delt_phi0, delt_lamda0);

    std::cout << "融合程序执行完毕" << endl;
}

void rongHe::rongHe_run3(double delt_phi0, double delt_lamda0,
                         double min_B, double min_L, double max_B, double max_L,double Bint,double Lint)
{
    // fenqujianmo
    //传入待融合的多源数据文件
    createMap2(min_B, min_L, max_B, max_L,Bint,Lint);
    readfile();

    //***** 数据融合 *****
    data = point0;  //同步模板格网平面坐标信息

    //汇总多源数据
    allPoints(doc_points, all_point);

    //构建内插模型
    data = calModel(delt_phi0, delt_lamda0);

    std::cout << "融合程序执行完毕" << endl;
}

void rongHe::outResult(QString filepath)
{
    std::ofstream fout(filepath.toStdString());
    for (int i = 0; i < data.size(); i = i + 1)
    {
        if (std::isnan(data[i].T) || std::isinf(data[i].T))
            continue;
        fout << data[i].L << " " << data[i].B  << " " << data[i].T << std::endl;
    }
    fout.close();
}
