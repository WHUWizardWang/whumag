#ifndef UIICONS_H
#define UIICONS_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

// Stroke icons on a 24x24 grid (tools/ui_icons.json).  They are recoloured at render time so the
// same shape works in both themes and in the normal / hover / disabled states.
namespace UiIcons {

bool contains(const QString &name);

// Renders |name| at |size| logical pixels in |color|.
QPixmap pixmap(const QString &name, int size, const QColor &color, qreal strokeWidth = 1.6, qreal dpr = 1.0);

// Theme-aware icon: normal = secondary text colour, active/selected = accent, disabled = faded.
QIcon icon(const QString &name, int size = 16);
QIcon icon(const QString &name, const QColor &normal, const QColor &active, int size = 16);

} // namespace UiIcons

#endif // UIICONS_H
