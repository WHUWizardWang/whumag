#ifndef EXPLORERPANEL_H
#define EXPLORERPANEL_H

#include <QPair>
#include <QVector>
#include <QWidget>

class QFrame;
class QGridLayout;
class QLineEdit;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;

// Left panel: "工程" header with new / open buttons, a filter box, the project tree (the existing
// QTreeWidget of the main window) and a small properties card for the selected item.
class ExplorerPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ExplorerPanel(QTreeWidget *tree, QWidget *parent = nullptr);
    ~ExplorerPanel() override;

    // Rows of (label, value) shown in the properties card; an empty list hides the card.
    void setProperties(const QVector<QPair<QString, QString>> &rows);

signals:
    void newProjectRequested();
    void openProjectRequested();

private:
    void applyFilter(const QString &needle);
    static bool filterItem(QTreeWidgetItem *item, const QString &needle, bool parentMatched);
    void refreshIcons();
    void updateEmptyState();

    QTreeWidget *tree_;
    bool closing_ = false;
    QLineEdit *search_;
    QToolButton *newBtn_;
    QToolButton *openBtn_;
    QFrame *propsCard_;
    QWidget *propsWrap_;
    QWidget *emptyState_;
    QGridLayout *propsGrid_;
};

#endif // EXPLORERPANEL_H
