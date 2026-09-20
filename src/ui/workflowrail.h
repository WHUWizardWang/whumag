#ifndef WORKFLOWRAIL_H
#define WORKFLOWRAIL_H

#include <QAbstractButton>
#include <QStringList>
#include <QWidget>

class QButtonGroup;

// One entry of the rail: small step number, icon and label, painted with the theme colours.
class RailButton : public QAbstractButton
{
    Q_OBJECT
public:
    RailButton(const QString &text, const QString &iconName, int number, QWidget *parent = nullptr);
    QSize sizeHint() const override { return QSize(56, 60); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString icon_;
    int number_;
};

// Left navigation: 数据 → 预处理 → 建图 → 评估 → 导航, plus 设置 / 帮助 at the bottom.
class WorkflowRail : public QWidget
{
    Q_OBJECT
public:
    explicit WorkflowRail(QWidget *parent = nullptr);

    static QStringList stageNames();
    int currentStage() const;
    void setCurrentStage(int index);

signals:
    void stageChanged(int index);
    void settingsClicked();
    void helpClicked();

private:
    QButtonGroup *group_;
};

#endif // WORKFLOWRAIL_H
