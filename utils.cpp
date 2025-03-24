#include "utils.h"


MultiComboBox::MultiComboBox(QWidget* parent) : QComboBox(parent) {
    edit = new QLineEdit(this);
    list = new QListWidget(this);
    edit->setReadOnly(true);
    this->setModel(list->model());
    this->setView(list);
    this->setLineEdit(edit);
}
void MultiComboBox::addItem(const QString& text, const QVariant& userData) {
    QListWidgetItem* item = new QListWidgetItem(list);
    QCheckBox* checkBox = new QCheckBox(this);
    checkBox->setText(text);
    list->addItem(item);
    list->setItemWidget(item, checkBox);
    connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(stateChangedSlot(int)));
}
void MultiComboBox::stateChangedSlot(int) {
    QString str = "";
    for (int i = 0; i < list->count(); i++) {
        QCheckBox* cb = static_cast<QCheckBox*>(list->itemWidget(list->item(i)));
        if (cb->isChecked()) {
            str += cb->text();
            str += ";";  // 多个内容以分号隔开
        }
    }
    str = str.remove(str.size() - 1,1);
    if (str != "") {
        edit->setText(str);
    }
    else {
        edit->clear();
    }
}


bool isNumeric(const QString &str)
{
    bool ok;
    str.toInt(&ok);     // 尝试转换为整数
    if (ok) return true;
    str.toDouble(&ok);  // 如果整数转换失败，尝试转换为浮点数
    return ok;
}
