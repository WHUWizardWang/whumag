#include "navplot.h"

#include "qcustomplot.h"
#include "thememanager.h"

#include <QObject>

namespace Nav {

namespace {

struct TrackStyle
{
    QColor color;
    QCPScatterStyle::ScatterShape shape;
    Qt::PenStyle line;
};

TrackStyle styleFor(const QString &key)
{
    if (key == QLatin1String("truth"))
        return {QColor("#D62828"), QCPScatterStyle::ssCross, Qt::SolidLine};
    if (key == QLatin1String("ins"))
        return {QColor("#4A4F57"), QCPScatterStyle::ssDisc, Qt::DashLine};
    if (key == QLatin1String("tercom"))
        return {QColor("#B02068"), QCPScatterStyle::ssTriangle, Qt::SolidLine};
    if (key == QLatin1String("tercom_plain"))
        return {QColor("#8E6AC8"), QCPScatterStyle::ssTriangleInverted, Qt::SolidLine};
    if (key == QLatin1String("iccp"))
        return {QColor("#2456A6"), QCPScatterStyle::ssCircle, Qt::SolidLine};
    if (key == QLatin1String("tercom_iccp"))
        return {QColor("#C77A0A"), QCPScatterStyle::ssDiamond, Qt::SolidLine};
    return {QColor("#138A52"), QCPScatterStyle::ssSquare, Qt::SolidLine};   // sitan and anything else
}

// Tracks are drawn as curves (QCPGraph would sort the points by x).
void addTrack(QCustomPlot *plot, const Path &path, const QString &name, const TrackStyle &style)
{
    if (path.isEmpty())
        return;
    QVector<double> t(path.size()), x(path.size()), y(path.size());
    for (int i = 0; i < path.size(); ++i) {
        t[i] = i;
        x[i] = path[i].x();
        y[i] = path[i].y();
    }
    auto *curve = new QCPCurve(plot->xAxis, plot->yAxis);
    curve->setName(name);
    curve->setData(t, x, y, true);
    QPen pen(style.color, 1.4, style.line);
    curve->setPen(pen);
    curve->setScatterStyle(QCPScatterStyle(style.shape, style.color, 6));
}

void applyTheme(QCustomPlot *plot)
{
    const ThemeManager &tm = ThemeManager::instance();
    const QColor bg = tm.color("n2"), axis = tm.color("line2"), text = tm.color("t2");
    plot->setBackground(QBrush(bg));
    plot->axisRect()->setBackground(QBrush(bg));
    QList<QCPAxis *> axes = {plot->xAxis, plot->yAxis};
    for (QCPLayoutElement *e : plot->plotLayout()->elements(false))
        if (auto *scale = qobject_cast<QCPColorScale *>(e))
            axes << scale->axis();
    for (QCPAxis *a : axes) {
        a->setBasePen(QPen(axis));
        a->setTickPen(QPen(axis));
        a->setSubTickPen(QPen(axis));
        a->setTickLabelColor(text);
        a->setLabelColor(text);
    }
    QColor legendBg = bg;
    legendBg.setAlpha(225);
    plot->legend->setBrush(QBrush(legendBg));
    plot->legend->setBorderPen(QPen(tm.color("line")));
    plot->legend->setTextColor(tm.color("t1"));
    plot->replot();
}

} // namespace

QCustomPlot *createResultPlot(const Outcome &outcome, QWidget *parent)
{
    auto *plot = new QCustomPlot(parent);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    plot->xAxis->setLabel(QStringLiteral("X"));
    plot->yAxis->setLabel(QStringLiteral("Y"));

    // background field straight from the grid (holes transparent)
    const GridField &g = outcome.grid;
    if (!g.isEmpty() && std::isfinite(g.minValue())) {
        auto *map = new QCPColorMap(plot->xAxis, plot->yAxis);
        map->setName(QStringLiteral("背景场"));
        map->removeFromLegend();
        map->data()->setSize(g.columns(), g.rows());
        map->data()->setRange(QCPRange(g.xMin(), g.xMax()), QCPRange(g.yMin(), g.yMax()));
        for (int ix = 0; ix < g.columns(); ++ix) {
            for (int iy = 0; iy < g.rows(); ++iy) {
                const double v = g.at(ix, iy);
                if (std::isnan(v))
                    map->data()->setAlpha(ix, iy, 0);
                else
                    map->data()->setCell(ix, iy, v);
            }
        }
        auto *scale = new QCPColorScale(plot);
        plot->plotLayout()->addElement(0, 1, scale);
        scale->setLabel(QStringLiteral("背景场"));
        map->setColorScale(scale);
        map->setGradient(QCPColorGradient::gpJet);
        map->setDataRange(QCPRange(g.minValue(), g.maxValue()));
        auto *group = new QCPMarginGroup(plot);
        plot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, group);
        scale->setMarginGroup(QCP::msBottom | QCP::msTop, group);
    }

    addTrack(plot, outcome.truth, QStringLiteral("真实航迹"), styleFor(QStringLiteral("truth")));
    addTrack(plot, outcome.ins, QStringLiteral("INS 航迹"), styleFor(QStringLiteral("ins")));
    for (const MatchedTrack &t : outcome.tracks)
        addTrack(plot, t.path, t.label, styleFor(t.key));

    // start on the tracks, with some of the map around them
    double x0 = std::numeric_limits<double>::max(), x1 = std::numeric_limits<double>::lowest();
    double y0 = x0, y1 = x1;
    auto extend = [&](const Path &p) {
        for (const QPointF &q : p) {
            x0 = std::min(x0, q.x());
            x1 = std::max(x1, q.x());
            y0 = std::min(y0, q.y());
            y1 = std::max(y1, q.y());
        }
    };
    extend(outcome.truth);
    extend(outcome.ins);
    for (const MatchedTrack &t : outcome.tracks)
        extend(t.path);
    if (x0 <= x1) {
        const double pad = std::max({(x1 - x0) * 0.2, (y1 - y0) * 0.2, 2 * std::max(g.dx(), g.dy())});
        plot->xAxis->setRange(x0 - pad, x1 + pad);
        plot->yAxis->setRange(y0 - pad, y1 + pad);
    } else {
        plot->rescaleAxes();
    }

    plot->legend->setVisible(true);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft | Qt::AlignTop);
    applyTheme(plot);
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, plot, [plot]() { applyTheme(plot); });
    return plot;
}

} // namespace Nav
