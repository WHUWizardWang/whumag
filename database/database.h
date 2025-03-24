#ifndef DATABASE_H
#define DATABASE_H

#include <QWidget>
#include <qdebug.h>
#include <QTableWidget>
#include <QFileDialog>
#include <QDateTime>
#include "databasemanager.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QComboBox>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QMap>
#include <QStyledItemDelegate>
#include "utils.h"
#include "draw/draw_form.h"

namespace Ui {
class database;
}

    // 自定义委托类
    class EnumComboBoxDelegate1 : public QStyledItemDelegate {
        Q_OBJECT

    public:
        EnumComboBoxDelegate1(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override {
            QComboBox *comboBox = new QComboBox(parent);
            foreach(const QString &str,datatype.keys())
            {
                comboBox->addItem(str,datatype.value(str));
            }
            return comboBox;
        }

        void setEditorData(QWidget *editor, const QModelIndex &index) const override {
            QComboBox *comboBox = static_cast<QComboBox *>(editor);
            QString currentValue = static_cast<QString>(index.data(Qt::EditRole).toInt());
            int currentIndex = comboBox->findData(QVariant::fromValue(currentValue));
            if (currentIndex >= 0) {
                comboBox->setCurrentIndex(currentIndex);
            }
        }

        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
            QComboBox *comboBox = static_cast<QComboBox *>(editor);
            QVariant enumValue = comboBox->currentData();
            QString selectedEnum = enumValue.value<QString>();
            model->setData(index, QVariant(static_cast<QString>(selectedEnum)), Qt::EditRole);
        }
    private:
        QMap<QString,QString> datatype = {
            {"实测数据","实测数据"},
            {"处理后数据","处理后数据"}
        };
    };

    // 自定义委托类
    class EnumComboBoxDelegate2 : public QStyledItemDelegate {
        Q_OBJECT

    public:
        EnumComboBoxDelegate2(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override {
            QComboBox *comboBox = new QComboBox(parent);
            foreach(const QString &str,datatype.keys())
            {
                comboBox->addItem(str,datatype.value(str));
            }
            return comboBox;
        }

        void setEditorData(QWidget *editor, const QModelIndex &index) const override {
            QComboBox *comboBox = static_cast<QComboBox *>(editor);
            QString currentValue = static_cast<QString>(index.data(Qt::EditRole).toInt());
            int currentIndex = comboBox->findData(QVariant::fromValue(currentValue));
            if (currentIndex >= 0) {
                comboBox->setCurrentIndex(currentIndex);
            }
        }

        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
            QComboBox *comboBox = static_cast<QComboBox *>(editor);
            QVariant enumValue = comboBox->currentData();
            QString selectedEnum = enumValue.value<QString>();
            model->setData(index, QVariant(static_cast<QString>(selectedEnum)), Qt::EditRole);
        }
    private:
        QMap<QString,QString> datatype = {
            {"船磁","船磁"},
            {"航磁","航磁"},
            {"水下磁测","水下磁测"}
        };
    };

class database : public QWidget
{
    Q_OBJECT

public:
    explicit database(QWidget *parent = nullptr);
    ~database();
    QString proPath;
    void openData(QString filePath,double &xmin,double &xmax,double &ymin,double &ymax);    //返回文件的最值

private slots:
    void on_pushButton_add_clicked();       //增加
    void on_pushButton_delete_clicked();    //删除
    void on_pushButton_update_clicked();    //更新
    void on_pushButton_show_clicked();      //绘图
    void on_pushButton_cancel_clicked();    //取消
    void on_pushButton_filter_clicked();    //查询

    void on_pushButton_cancelfilter_clicked();

private:
    Ui::database *ui;
    QSqlTableModel *model;
    QVector<int> rows;  //存储增加的行号


};

#endif // DATABASE_H
