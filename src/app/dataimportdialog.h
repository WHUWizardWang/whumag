#ifndef DATAIMPORTDIALOG_H
#define DATAIMPORTDIALOG_H

#include "dataio/tableimport.h"

#include <QDialog>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;
class Chip;

// "数据格式与列设置": shows how a data file is read (delimiter, header, skipped lines / columns,
// encoding; detected automatically and adjustable) with a live preview, and lets the user say which
// column is X (longitude), Y (latitude) and the field value -- or three components of a vector field.
class DataImportDialog : public QDialog
{
    Q_OBJECT

public:
    // |initial| is used as the starting point (e.g. the settings chosen last time for this file).
    DataImportDialog(const QString &path, const DataIO::ImportSettings &initial, QWidget *parent = nullptr);

    DataIO::ImportSettings settings() const { return settings_; }
    // One line for the import form: "逗号分隔，跳过 1 行；X = 经度，Y = 纬度，值 = 磁场值".
    QString summary() const;

private:
    enum Role { Ignore, RoleX, RoleY, RoleValue, RoleC1, RoleC2, RoleC3 };

    void readControls();
    void reload();
    void rebuildTable();
    void roleChanged(int column, int role);
    void updateStatus();
    int roleOf(int column) const;

    QString path_;
    DataIO::ImportSettings settings_;
    DataIO::Preview preview_;
    bool keepRoles_ = false;

    QComboBox *encoding_ = nullptr;
    QComboBox *delimiter_ = nullptr;
    QLineEdit *custom_ = nullptr;
    QSpinBox *skipLines_ = nullptr;
    QComboBox *header_ = nullptr;
    QLineEdit *comment_ = nullptr;
    QSpinBox *skipColumns_ = nullptr;
    QLineEdit *missing_ = nullptr;
    QList<QWidget *> textOnly_;
    Chip *formatChip_ = nullptr;
    QLabel *detected_ = nullptr;
    QTableWidget *table_ = nullptr;
    QVector<QComboBox *> roleBoxes_;
    QLabel *status_ = nullptr;
    QPushButton *ok_ = nullptr;
    QTimer *reloadTimer_ = nullptr;
};

#endif // DATAIMPORTDIALOG_H
