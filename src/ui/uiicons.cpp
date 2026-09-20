#include "uiicons.h"

#include "thememanager.h"

#include <QHash>
#include <QPainter>
#include <QSvgRenderer>
#include <QtMath>

namespace {

const QHash<QString, QString> &iconData()
{
    static const QHash<QString, QString> data = {
#include "uiicons_data.inc"
    };
    return data;
}

} // namespace

namespace UiIcons {

bool contains(const QString &name)
{
    return iconData().contains(name);
}

QPixmap pixmap(const QString &name, int size, const QColor &color, qreal strokeWidth, qreal dpr)
{
    static QHash<QString, QPixmap> cache;
    const QString key = QStringLiteral("%1|%2|%3|%4|%5").arg(name).arg(size).arg(color.name(QColor::HexArgb)).arg(strokeWidth).arg(dpr);
    auto hit = cache.constFind(key);
    if (hit != cache.constEnd())
        return hit.value();

    const QString body = iconData().value(name);
    if (body.isEmpty())
        return QPixmap();

    const QString svg = QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%1\" "
        "stroke-opacity=\"%2\" stroke-width=\"%3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">%4</svg>")
        .arg(color.name(QColor::HexRgb))
        .arg(color.alphaF())
        .arg(strokeWidth)
        .arg(body);

    QSvgRenderer renderer(svg.toUtf8());
    QPixmap px(qCeil(size * dpr), qCeil(size * dpr));
    px.setDevicePixelRatio(dpr);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&p, QRectF(0, 0, size, size));
    p.end();

    cache.insert(key, px);
    return px;
}

QIcon icon(const QString &name, const QColor &normal, const QColor &active, int size)
{
    QIcon ic;
    QColor disabled = normal;
    disabled.setAlphaF(0.45);
    // two render sizes so the icon stays crisp when Qt scales it a little
    for (int s : {size, size * 2}) {
        ic.addPixmap(pixmap(name, s, normal), QIcon::Normal, QIcon::Off);
        ic.addPixmap(pixmap(name, s, active), QIcon::Active, QIcon::Off);
        ic.addPixmap(pixmap(name, s, active), QIcon::Selected, QIcon::Off);
        ic.addPixmap(pixmap(name, s, active), QIcon::Normal, QIcon::On);
        ic.addPixmap(pixmap(name, s, disabled), QIcon::Disabled, QIcon::Off);
    }
    return ic;
}

QIcon icon(const QString &name, int size)
{
    const ThemeManager &tm = ThemeManager::instance();
    return icon(name, tm.color("t2"), tm.color("accInk"), size);
}

} // namespace UiIcons
