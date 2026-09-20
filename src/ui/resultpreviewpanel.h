#ifndef RESULTPREVIEWPANEL_H
#define RESULTPREVIEWPANEL_H

#include <QList>
#include <QWidget>

#include "uiwidgets.h"

class QLabel;
class QTextBrowser;

// Right-hand side of the modelling windows: a title with a status chip, a card that hosts the
// result views (heat map / contour containers created by the .ui) with an empty-state hint, and
// the log below.  It takes over the widgets it is given.
class ResultPreviewPanel : public QWidget
{
    Q_OBJECT
public:
    ResultPreviewPanel(const QList<QWidget *> &pages, QTextBrowser *log, QWidget *parent = nullptr);

    void setStatus(const QString &text, Chip::Kind kind);

    // Shows the "尚无预览" hint unless one of the (visible) pages already holds a result.
    void refreshEmptyState();

private:
    void restyle();

    QList<QWidget *> pages_;
    QTextBrowser *log_;
    Chip *chip_;
    QLabel *empty_;
};

#endif // RESULTPREVIEWPANEL_H
