#ifndef NAV_NAVPLOT_H
#define NAV_NAVPLOT_H

#include "navigationrunner.h"

class QCustomPlot;
class QWidget;

namespace Nav {

// Result chart: the background grid as a heat map, the true path, the INS track and every matched
// track of |outcome|.  The view starts on the tracks; drag / wheel to pan and zoom.
QCustomPlot *createResultPlot(const Outcome &outcome, QWidget *parent = nullptr);

} // namespace Nav

#endif // NAV_NAVPLOT_H
