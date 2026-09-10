#ifndef MAPTAYLORLEGENDREFORM_H
#define MAPTAYLORLEGENDREFORM_H

#include <QWidget>

namespace Ui {
class maptaylorlegendreform;
}

class maptaylorlegendreform : public QWidget
{
    Q_OBJECT

public:
    explicit maptaylorlegendreform(QWidget *parent = nullptr);
    ~maptaylorlegendreform();
    int getspinbox();   // 阶数

private:
    Ui::maptaylorlegendreform *ui;
};

#endif // MAPTAYLORLEGENDREFORM_H
