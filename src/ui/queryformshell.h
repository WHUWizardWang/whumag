#ifndef QUERYFORMSHELL_H
#define QUERYFORMSHELL_H

#include <QList>
#include <QWidget>

class QComboBox;
class QFrame;
class QPushButton;
class QRadioButton;
class QTableView;
class QTextBrowser;
class QVBoxLayout;
class Chip;

// Rebuilds the layout of the two query forms (global field model / anomaly grid): parameters in a
// left panel, results (status, optional extra views, table) in a right panel.  The widgets the
// forms already own (combo box, radio buttons, sub-forms, table, buttons) are moved into the new
// layout, so all their existing signal / slot wiring keeps working.
namespace QueryFormShell {

struct Parts {
    QWidget *form = nullptr;
    QComboBox *product = nullptr;
    QRadioButton *radioPoint = nullptr;
    QRadioButton *radioFile = nullptr;   // null when the form has no "点文件" mode
    QRadioButton *radioGrid = nullptr;
    QList<QWidget *> subforms;           // parameter sub-forms, one per mode
    QPushButton *query = nullptr;
    QTableView *table = nullptr;
    QPushButton *exportButton = nullptr;
    QPushButton *importButton = nullptr; // stays hidden, as in the old layout
    QTextBrowser *log = nullptr;         // kept alive but hidden
    QFrame *oldLeft = nullptr;           // the two frames of the old .ui, removed here
    QFrame *oldRight = nullptr;
};

struct Shell {
    QVBoxLayout *leftLayout = nullptr;   // parameter column; extras go in at |leftInsertIndex|
    int leftInsertIndex = 0;
    QVBoxLayout *middle = nullptr;       // result column between the header and the table
    Chip *statusChip = nullptr;
    QWidget *leftPanel = nullptr;
    QWidget *rightPanel = nullptr;
};

Shell apply(const Parts &parts, const QString &productCaption);

} // namespace QueryFormShell

#endif // QUERYFORMSHELL_H
