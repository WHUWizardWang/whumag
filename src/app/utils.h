#ifndef UTILS_H
#define UTILS_H
#include <QString>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>

// 设置输入框一直以.txt为后缀
class txtLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    txtLineEdit(QWidget *parent = nullptr) : QLineEdit(parent){
        setText(".txt");
        connect(this,&QLineEdit::textChanged,this,&txtLineEdit::onTextChanged);
    }

private:
    void onTextChanged(const QString &newText)
    {
        if(!newText.endsWith(".txt"))
        {
            QString baseName = newText.trimmed();
            if(!baseName.isEmpty())
            {
                setText(baseName+".txt");
                setCursorPosition(baseName.length());
            }
        }
    }
};

// 设置combobox为多选框
class MultiComboBox : public QComboBox {
    Q_OBJECT
public:
    MultiComboBox(QWidget* parent = nullptr);
    void addItem(const QString& text, const QVariant& userData = QVariant());  // 重写父类addItem的方法
public slots:
    void stateChangedSlot(int);   // 下拉列表中的内容被选中时触发
private:
    QListWidget* list;
    QLineEdit* edit;
};




bool isNumeric(const QString &str);         // 检查输入的字符串是否为数字

#endif // UTILS_H
