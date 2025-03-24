#ifndef CONTOURPLOTTER_H
#define CONTOURPLOTTER_H

#include <QMainWindow>
#include "draw/qcustomplot.h"
#include <vector>

class ContourPlotter : public QMainWindow
{
    Q_OBJECT

public:
    explicit ContourPlotter(QWidget *parent = nullptr);
    ~ContourPlotter();
    QCustomPlot *customPlot;
    bool loadData(const QString& filepath);
    void plotContour();
    void setRMS(double rms) { m_rms = rms; }
    void savePlot(const QString& pngpath);
    void findContours(const std::vector<std::vector<double>>& data, double level,
                      std::vector<QVector<double>>& contourX,
                      std::vector<QVector<double>>& contourY);
    void interpolateEdge(double x1, double y1, double z1,
                         double x2, double y2, double z2,
                         double level, double& x, double& y);
    std::vector<double> generateContourLevels(const std::vector<std::vector<double>>& data, int numLevels);
private:
    std::vector<double> X, Y, Z;
    std::vector<std::vector<double>> Z_grid;
    double m_rms;

    // 插值函数
    double bilinearInterpolate(double x, double y,
                               const std::vector<double>& gridX,
                               const std::vector<double>& gridY,
                               const std::vector<std::vector<double>>& gridZ);
};

#endif // CONTOURPLOTTER_H
