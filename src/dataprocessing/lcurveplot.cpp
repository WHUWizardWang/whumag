#include "lcurveplot.h"

#include "qcustomplot.h"
#include "thememanager.h"

namespace {

void applyTheme(QCustomPlot *plot)
{
    const ThemeManager &tm = ThemeManager::instance();
    const QColor bg = tm.color("n2"), axis = tm.color("line2"), text = tm.color("t2");
    plot->setBackground(QBrush(bg));
    plot->axisRect()->setBackground(QBrush(bg));
    for (QCPAxis *a : {plot->xAxis, plot->yAxis}) {
        a->setBasePen(QPen(axis));
        a->setTickPen(QPen(axis));
        a->setSubTickPen(QPen(axis));
        a->setTickLabelColor(text);
        a->setLabelColor(text);
        a->grid()->setPen(QPen(tm.color("line"), 1, Qt::DotLine));
        a->grid()->setSubGridVisible(false);
    }
    plot->legend->setBrush(QBrush(bg));
    plot->legend->setTextColor(tm.color("t1"));
    plot->legend->setBorderPen(QPen(tm.color("line")));
    plot->replot();
}

QString parameterText(const Proc::LCurve &c, double p)
{
    return Proc::parameterIsIterationCount(c.method) ? QStringLiteral("n = %1").arg(qRound(p))
                                                     : QStringLiteral("α = %1").arg(p, 0, 'g', 3);
}

} // namespace

QCustomPlot *createLCurvePlot(const Proc::LCurve &curve, QWidget *parent)
{
    auto *plot = new QCustomPlot(parent);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    for (QCPAxis *a : {plot->xAxis, plot->yAxis}) {
        a->setScaleType(QCPAxis::stLogarithmic);
        a->setTicker(logTicker);
        a->setNumberFormat(QStringLiteral("eb"));
        a->setNumberPrecision(0);
    }
    plot->xAxis->setLabel(QStringLiteral("残差范数 ‖R·f − g‖"));
    plot->yAxis->setLabel(QStringLiteral("解范数 ‖f‖"));

    QVector<double> t, x, y;
    for (int i = 0; i < int(curve.points.size()); ++i) {
        const Proc::LCurvePoint &p = curve.points[i];
        if (p.residualNorm > 0 && p.solutionNorm > 0) {
            t << i;
            x << p.residualNorm;
            y << p.solutionNorm;
        }
    }
    auto *line = new QCPCurve(plot->xAxis, plot->yAxis);
    line->setName(QStringLiteral("L 曲线（%1）").arg(Proc::downwardMethodName(curve.method)));
    line->setData(t, x, y, true);
    line->setPen(QPen(QColor("#2456A6"), 1.6));
    line->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor("#2456A6"), 4));

    if (curve.corner >= 0) {
        const Proc::LCurvePoint &c = curve.points[curve.corner];
        auto *mark = plot->addGraph();
        mark->setName(QStringLiteral("拐点 %1").arg(parameterText(curve, c.parameter)));
        mark->setData({c.residualNorm}, {c.solutionNorm});
        mark->setLineStyle(QCPGraph::lsNone);
        mark->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, QColor("#D62828"), 11));
        auto *label = new QCPItemText(plot);
        label->position->setCoords(c.residualNorm, c.solutionNorm);
        label->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
        label->setText(QStringLiteral("  拐点 %1").arg(parameterText(curve, c.parameter)));
        label->setColor(QColor("#D62828"));
        QFont f = label->font();
        f.setBold(true);
        label->setFont(f);
    }
    plot->rescaleAxes();
    plot->xAxis->scaleRange(1.3, plot->xAxis->range().center());
    plot->yAxis->scaleRange(1.3, plot->yAxis->range().center());
    plot->legend->setVisible(true);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight | Qt::AlignTop);
    applyTheme(plot);
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, plot, [plot]() { applyTheme(plot); });
    return plot;
}
