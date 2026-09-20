#ifndef DATAQUERYDIALOG_H
#define DATAQUERYDIALOG_H

#include <QDialog>

class QStackedWidget;
class QWidget;
class SegmentedControl;

// One window for both data queries.  The two query forms keep their own logic; this dialog only
// hosts them as pages behind a segmented control and takes ownership of them.
class DataQueryDialog : public QDialog
{
    Q_OBJECT
public:
    enum Page { GlobalModel = 0, Anomaly = 1 };

    DataQueryDialog(QWidget *globalModelForm, QWidget *anomalyForm, QWidget *parent = nullptr);

    void showPage(Page page);

private:
    void restyle();

    QWidget *header_;
    SegmentedControl *segmented_;
    QStackedWidget *stack_;
};

#endif // DATAQUERYDIALOG_H
