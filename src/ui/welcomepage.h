#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QFrame>
#include <QStringList>
#include <QWidget>

class QLabel;
class QVBoxLayout;

// A card that reacts to hover and emits clicked() (used for quick-start entries and recent projects).
class ClickableCard : public QFrame
{
    Q_OBJECT
public:
    explicit ClickableCard(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
};

// Shown in the centre of the main window until a project is open.
class WelcomePage : public QWidget
{
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);

    // Full paths of the .proj files, newest first.
    void setRecentProjects(const QStringList &projectFiles);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void openRecentRequested(const QString &projectFile);
    void quickStartRequested(int which);   // 0 = 一键成图, 1 = 整图建模, 2 = 匹配导航

private:
    static QString relativeTime(const QDateTime &t);
    QVBoxLayout *recentLayout_;
    QLabel *recentEmpty_;
};

#endif // WELCOMEPAGE_H
