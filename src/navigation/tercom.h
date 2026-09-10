#pragma once
#ifndef _MATCHINGNAVIGATION_H_
#define _MATCHINGNAVIGATION_H_

#include<iostream>
#include <QString>
#include <QVector>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QVector>
#include "DataStruct.h"
#include "function.h"
#include "qcustomplot.h"
#include "utils.h"
#include "referencemap/KDTree.h"
using namespace nanoflann;
namespace Geomagnetic {
    struct MapData {
        double x, y, magnetic;
    };

    struct INSData {
        //坐标 x,y,航向角heading,磁场强度magnetic
        double x, y, heading, magnetic;
        //中误差sigma
        double sigma=3;
    };

    struct TruePath {
        double x, y;
    };
	class TercomMatching
	{
    public:

        TercomMatching(double x_step , double y_step)
            : x_step(x_step), y_step(y_step) {};
        TercomMatching();
        Datapoint match();
        int ReadBackground(const QString &filePath);
        void ReadINS(const QString &filePath);
        void ReadTruePath(const QString &filePath);
        void setReferencePoint(const SinglePoint& refPoint);
        Datapoint matchWithAdaptiveRotation();
        void saveResult(const QString &filePath,const Datapoint &result);
        void drawResult(Datapoint matchResult);
        ~TercomMatching() {};

        std::vector<TruePath> truePath;
        std::vector<INSData> insData;
        std::vector<MapData> base;
        QCustomPlot* customPlot;
        QString error_str;
    private:
        Datapoint data;
        SinglePoint referencePoint;
        double xg;
        double yg;
        double x_step;
        double y_step;
        PointCloud cloud;
        std::unique_ptr<KDTree2D> kdtree;

        std::vector<INSData> rotateTrack(const std::vector<INSData> &track, double angle, const SinglePoint &center) const;
        std::vector<INSData> tercomMatch(const std::vector<INSData> &rotatedTrack) const;

        // Helper functions
        double calculateDistance(const INSData& insData, const MapData& base) const;
        double IDW(const INSData& insData, size_t K = 10) const;
        SinglePoint rotatePoint(const SinglePoint& point, double angle, const SinglePoint& center) const;
        double calculateMSD(const std::vector<INSData> track, const std::vector<INSData> insdata) const;

	};
}
#endif // !_MATCHINGNAVIGATION_H_
