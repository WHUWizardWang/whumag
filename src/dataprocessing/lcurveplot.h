#ifndef LCURVEPLOT_H
#define LCURVEPLOT_H

#include "continuation.h"

class QCustomPlot;
class QWidget;

// L-curve chart: log residual norm vs log solution norm, the chosen corner marked and labelled.
QCustomPlot *createLCurvePlot(const Proc::LCurve &curve, QWidget *parent = nullptr);

#endif // LCURVEPLOT_H
