#include "readFile.h"

readFile::readFile()
{
	Xi = 0.0;
	Yi = 0.0;
	Ti = 0.0;

}

readFile::~readFile()
{
	fin.close();
}

void readFile::read_file(std::vector<double>&X, std::vector<double>& Y, std::vector<double>& T,std::string infile)
{
	X.clear();
	Y.clear();
	T.clear();
    std::string line = "";
	fin.open(infile);
	while (getline(fin, line))
	{
        std::string tmp = "";
		double B = 0;
		double L = 0;
        std::istringstream sline(line);
		getline(sline, tmp, ' ');// 读入X
		Xi = stod(tmp);
		X.push_back(Xi);
		getline(sline, tmp, ' ');// 读入Y
		Yi = stod(tmp);
		Y.push_back(Yi);
		getline(sline, tmp, ' ');// 读入T
		Ti = stod(tmp);
		T.push_back(Ti);
	}
}



void readFile::setValue_d(double& a, double b)
{
	a = b;
}

void readFile::setValue_i(int& a, int b)
{
	a = b;
}

void readFile::readfile_run(std::string infile)
{
	//读入文件
	read_file(X, Y, T, infile);

}

void readFile::cal_grid(double x_step,double y_step)
{
    double minX = *std::min_element(X.begin(), X.end());
    double maxX = *std::max_element(X.begin(), X.end());
    double minY = *std::min_element(Y.begin(), Y.end());
    double maxY = *std::max_element(Y.begin(), Y.end());
    gridcol = round((maxX-minX)/x_step +1);
    gridrow = round((maxY-minY)/y_step +1);
}
