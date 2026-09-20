#ifndef UISCALE_H
#define UISCALE_H

#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QtGlobal>
#include <QtMath>

// Text in the redesigned UI is sized in points, so it follows the system text scaling (125 %, 150 %).
// Fixed geometry that has to fit that text (bars, headers, side panels, dialog sizes) is scaled with
// the same factor.  QT_FONT_DPI, when set, wins -- it is what the font engine uses, and lets tests
// simulate other scalings.
namespace UiScale {

inline qreal factor()
{
    static const qreal f = []() {
        const int fontDpi = qEnvironmentVariableIntValue("QT_FONT_DPI");
        if (fontDpi > 0)
            return qMax<qreal>(1.0, fontDpi / 96.0);
        const QScreen *screen = QGuiApplication::primaryScreen();
        return screen ? qMax<qreal>(1.0, screen->logicalDotsPerInch() / 96.0) : qreal(1.0);
    }();
    return f;
}

// Design pixels (at 96 dpi) -> device pixels
inline int dp(int px)
{
    return qRound(px * factor());
}

// A window size given in design pixels, scaled and kept inside the usable screen area.
inline QSize windowSize(int designWidth, int designHeight)
{
    QSize size(dp(designWidth), dp(designHeight));
    if (const QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        size.setWidth(qMin(size.width(), qMax(320, avail.width() - 40)));
        size.setHeight(qMin(size.height(), qMax(240, avail.height() - 60)));
    }
    return size;
}

} // namespace UiScale

#endif // UISCALE_H
