#ifndef MAPCOMPRESSFORM_H
#define MAPCOMPRESSFORM_H

#include <QWidget>

namespace Ui {
class mapcompressform;
}

class mapcompressform : public QWidget
{
    Q_OBJECT

public:
    explicit mapcompressform(QWidget *parent = nullptr);
    ~mapcompressform();
    int getspinBox();  // 稀疏度

private:
    Ui::mapcompressform *ui;
};

#endif // MAPCOMPRESSFORM_H
