#include "dataimportdialog.h"

#include "formkit.h"
#include "thememanager.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include <QComboBox>
#include <QDir>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const int kPreviewRows = 100;

QString roleText(int role)
{
    static const char *names[] = {"忽略", "X（经度）", "Y（纬度）", "磁场值", "分量 1", "分量 2", "分量 3"};
    return QString::fromUtf8(names[role]);
}

} // namespace

DataImportDialog::DataImportDialog(const QString &path, const DataIO::ImportSettings &initial, QWidget *parent)
    : QDialog(parent), path_(path), settings_(initial)
{
    setWindowTitle(tr("数据格式与列设置"));
    resize(UiScale::windowSize(900, 680));
    setMinimumSize(UiScale::dp(640), UiScale::dp(480));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(26, 22, 26, 18);
    root->setSpacing(12);
    root->addWidget(FormKit::header(QStringLiteral("import"), tr("数据格式与列设置"), QFileInfo(path).fileName()));

    auto *fileRow = new QHBoxLayout;
    fileRow->setSpacing(8);
    formatChip_ = new Chip(QString(), Chip::Info);
    auto *pathLabel = new ElideLabel(QDir::toNativeSeparators(path));
    pathLabel->setProperty("role", QStringLiteral("hint"));
    fileRow->addWidget(formatChip_);
    fileRow->addWidget(pathLabel, 1);
    root->addLayout(fileRow);

    // 1  how the file is read
    root->addWidget(FormKit::stepHeading(1, tr("读取方式")));
    encoding_ = new QComboBox;
    encoding_->addItems({tr("自动识别"), QStringLiteral("UTF-8"), QStringLiteral("GBK / GB18030")});
    delimiter_ = new QComboBox;
    for (DataIO::Delimiter d : {DataIO::Delimiter::Auto, DataIO::Delimiter::Whitespace, DataIO::Delimiter::Comma,
                                DataIO::Delimiter::Semicolon, DataIO::Delimiter::Tab, DataIO::Delimiter::Pipe, DataIO::Delimiter::Custom})
        delimiter_->addItem(d == DataIO::Delimiter::Auto ? tr("自动识别") : DataIO::delimiterName(d), int(d));
    custom_ = new QLineEdit;
    custom_->setPlaceholderText(tr("分隔字符"));
    custom_->setMaximumWidth(UiScale::dp(90));
    auto *delimRow = new QHBoxLayout;
    delimRow->setSpacing(6);
    delimRow->addWidget(delimiter_, 1);
    delimRow->addWidget(custom_);
    skipLines_ = new QSpinBox;
    skipLines_->setRange(0, 100000);
    skipLines_->setSuffix(tr(" 行"));
    header_ = new QComboBox;
    header_->addItems({tr("自动识别"), tr("第一行是列标题"), tr("没有列标题")});
    comment_ = new QLineEdit;
    comment_->setPlaceholderText(tr("无"));
    skipColumns_ = new QSpinBox;
    skipColumns_->setRange(0, 1000);
    skipColumns_->setSuffix(tr(" 列"));
    missing_ = new QLineEdit;
    missing_->setPlaceholderText(tr("如 99999，可留空"));

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);
    grid->addLayout(FormKit::field(tr("分隔符"), delimRow), 0, 0);
    grid->addLayout(FormKit::field(tr("列标题"), header_), 0, 1);
    grid->addLayout(FormKit::field(tr("文字编码"), encoding_), 0, 2);
    grid->addLayout(FormKit::field(tr("跳过开头"), skipLines_), 1, 0);
    grid->addLayout(FormKit::field(tr("跳过前几列"), skipColumns_), 1, 1);
    grid->addLayout(FormKit::field(tr("注释行开头"), comment_), 1, 2);
    grid->addLayout(FormKit::field(tr("无效值"), missing_), 1, 3);
    for (int c = 0; c < 4; ++c)
        grid->setColumnStretch(c, 1);
    root->addLayout(grid);
    textOnly_ = {encoding_, delimiter_, custom_, skipLines_, header_, comment_};
    detected_ = FormKit::hint(QString());
    root->addWidget(detected_);

    // 2  columns
    root->addSpacing(2);
    root->addWidget(FormKit::stepHeading(2, tr("指定各列的含义")));
    root->addWidget(FormKit::hint(tr("在每列上方选择它的含义。只有三分量数据时，选“分量 1、2、3”，磁场值按 √(B1²+B2²+B3²) 计算。"
                                     "下表为前 %1 行预览，无法识别为数字的格子标为红色。").arg(kPreviewRows)));
    table_ = new QTableWidget;
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->verticalHeader()->setDefaultSectionSize(UiScale::dp(24));
    table_->horizontalHeader()->setDefaultSectionSize(UiScale::dp(130));
    table_->horizontalHeader()->setTextElideMode(Qt::ElideRight);
    root->addWidget(table_, 1);

    status_ = new QLabel;
    status_->setWordWrap(true);
    root->addWidget(status_);

    auto *buttons = new QDialogButtonBox;
    ok_ = buttons->addButton(tr("确定"), QDialogButtonBox::AcceptRole);
    buttons->addButton(tr("取消"), QDialogButtonBox::RejectRole);
    FormKit::setRole(ok_, "primary");
    ok_->setMinimumWidth(96);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    // initial values
    const DataIO::TextOptions &t = settings_.text;
    encoding_->setCurrentIndex(int(t.encoding));
    delimiter_->setCurrentIndex(delimiter_->findData(int(t.delimiter)));
    custom_->setText(t.customDelimiter);
    skipLines_->setValue(t.skipLines);
    header_->setCurrentIndex(int(t.header));
    comment_->setText(t.commentPrefix);
    skipColumns_->setValue(settings_.skipColumns);
    missing_->setText(settings_.missingValue);
    keepRoles_ = settings_.roles.complete();

    reloadTimer_ = new QTimer(this);
    reloadTimer_->setSingleShot(true);
    reloadTimer_->setInterval(200);
    connect(reloadTimer_, &QTimer::timeout, this, &DataImportDialog::reload);
    auto later = [this]() { reloadTimer_->start(); };
    auto later2 = [this]() {
        keepRoles_ = false;   // the columns move: guess the roles again
        reloadTimer_->start();
    };
    connect(encoding_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, later);
    connect(delimiter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, later2);
    connect(custom_, &QLineEdit::textChanged, this, later2);
    connect(skipLines_, QOverload<int>::of(&QSpinBox::valueChanged), this, later2);
    connect(header_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, later2);
    connect(comment_, &QLineEdit::textChanged, this, later2);
    connect(skipColumns_, QOverload<int>::of(&QSpinBox::valueChanged), this, later2);
    connect(missing_, &QLineEdit::textChanged, this, [this]() {
        settings_.missingValue = missing_->text().trimmed();
        updateStatus();
    });
    reload();
}

void DataImportDialog::readControls()
{
    DataIO::TextOptions &t = settings_.text;
    t.encoding = DataIO::TextEncoding(encoding_->currentIndex());
    t.delimiter = DataIO::Delimiter(delimiter_->currentData().toInt());
    t.customDelimiter = custom_->text();
    t.skipLines = skipLines_->value();
    t.header = DataIO::HeaderMode(header_->currentIndex());
    t.commentPrefix = comment_->text().trimmed();
    settings_.skipColumns = skipColumns_->value();
    settings_.missingValue = missing_->text().trimmed();
    custom_->setVisible(t.delimiter == DataIO::Delimiter::Custom);
}

void DataImportDialog::reload()
{
    readControls();
    setCursor(Qt::WaitCursor);
    preview_ = DataIO::loadPreview(path_, settings_, kPreviewRows);
    unsetCursor();

    const bool text = preview_.kind == DataIO::FileKind::Text;
    for (QWidget *w : textOnly_)
        w->setEnabled(text);
    formatChip_->setText(preview_.ok() ? (text ? tr("文本") : preview_.format) : tr("无法读取"));
    formatChip_->setKind(preview_.ok() ? Chip::Info : Chip::Error);

    if (!preview_.ok()) {
        detected_->setText(QString());
        table_->clear();
        table_->setRowCount(0);
        table_->setColumnCount(0);
        roleBoxes_.clear();
        updateStatus();
        return;
    }
    QStringList facts;
    if (text) {
        facts << tr("%1分隔").arg(DataIO::delimiterName(preview_.delimiter))
              << preview_.encoding;
    } else if (preview_.totalRows >= 0) {
        facts << tr("%1 条记录，%2 列").arg(preview_.totalRows).arg(preview_.headers.size());
    }
    facts << preview_.notes;
    detected_->setText(tr("识别结果：%1").arg(facts.join(QStringLiteral("；"))));

    if (!keepRoles_ || !settings_.roles.complete()
        || qMax(qMax(settings_.roles.x, settings_.roles.y), qMax(settings_.roles.value, settings_.roles.component[2])) >= preview_.headers.size())
        settings_.roles = DataIO::guessRoles(preview_);
    keepRoles_ = true;
    rebuildTable();
}

int DataImportDialog::roleOf(int column) const
{
    const DataIO::ColumnRoles &r = settings_.roles;
    if (column == r.x) return RoleX;
    if (column == r.y) return RoleY;
    if (column == r.value) return RoleValue;
    for (int k = 0; k < 3; ++k)
        if (column == r.component[k])
            return RoleC1 + k;
    return Ignore;
}

void DataImportDialog::rebuildTable()
{
    const int cols = preview_.headers.size();
    table_->clear();
    table_->setColumnCount(cols);
    table_->setRowCount(preview_.rows.size() + 1);
    QStringList headers;
    for (int c = 0; c < cols; ++c) {
        const QString unit = c < preview_.units.size() ? preview_.units[c] : QString();
        headers << (unit.isEmpty() ? preview_.headers[c] : QStringLiteral("%1 (%2)").arg(preview_.headers[c], unit));
    }
    table_->setHorizontalHeaderLabels(headers);
    QStringList rowLabels{tr("含义")};
    for (int r = 0; r < preview_.rows.size(); ++r)
        rowLabels << QString::number(r + 1);
    table_->setVerticalHeaderLabels(rowLabels);

    roleBoxes_.clear();
    for (int c = 0; c < cols; ++c) {
        auto *box = new QComboBox;
        for (int role = Ignore; role <= RoleC3; ++role)
            box->addItem(roleText(role));
        box->setCurrentIndex(roleOf(c));
        connect(box, QOverload<int>::of(&QComboBox::activated), this, [this, c](int role) { roleChanged(c, role); });
        table_->setCellWidget(0, c, box);
        roleBoxes_ << box;
    }
    table_->setRowHeight(0, UiScale::dp(34));
    for (int r = 0; r < preview_.rows.size(); ++r)
        for (int c = 0; c < cols; ++c)
            table_->setItem(r + 1, c, new QTableWidgetItem(c < preview_.rows[r].size() ? preview_.rows[r][c] : QString()));
    table_->resizeColumnsToContents();
    for (int c = 0; c < cols; ++c)
        table_->setColumnWidth(c, qBound(UiScale::dp(110), table_->columnWidth(c) + UiScale::dp(8), UiScale::dp(240)));
    updateStatus();
}

void DataImportDialog::roleChanged(int column, int role)
{
    DataIO::ColumnRoles &r = settings_.roles;
    // the column loses its previous role, and the role leaves its previous column
    for (int *slot : {&r.x, &r.y, &r.value, &r.component[0], &r.component[1], &r.component[2]})
        if (*slot == column)
            *slot = -1;
    switch (role) {
    case RoleX: r.x = column; break;
    case RoleY: r.y = column; break;
    case RoleValue:
        r.value = column;
        r.component[0] = r.component[1] = r.component[2] = -1;   // one or the other
        break;
    case RoleC1: case RoleC2: case RoleC3:
        r.component[role - RoleC1] = column;
        r.value = -1;
        break;
    default: break;
    }
    for (int c = 0; c < roleBoxes_.size(); ++c) {
        const QSignalBlocker block(roleBoxes_[c]);
        roleBoxes_[c]->setCurrentIndex(roleOf(c));
    }
    updateStatus();
}

void DataImportDialog::updateStatus()
{
    const ThemeManager &tm = ThemeManager::instance();
    // tint the cells of the used columns that are not numbers
    const bool dc = preview_.kind == DataIO::FileKind::Text && preview_.delimiter != DataIO::Delimiter::Comma;
    int bad = 0;
    for (int c = 0; c < table_->columnCount(); ++c) {
        const bool used = roleOf(c) != Ignore;
        for (int r = 1; r < table_->rowCount(); ++r) {
            QTableWidgetItem *it = table_->item(r, c);
            if (!it)
                continue;
            double v;
            const bool fail = used && !DataIO::parseNumber(it->text(), v, dc);
            bad += fail ? 1 : 0;
            it->setBackground(fail ? QBrush(tm.color("errBg")) : QBrush());
            it->setForeground(used ? QBrush() : QBrush(tm.color("t3")));
        }
    }

    QString text;
    Chip::Kind kind = Chip::Ok;
    const DataIO::ColumnRoles &r = settings_.roles;
    if (!preview_.ok()) {
        text = preview_.error;
        kind = Chip::Error;
    } else if (!r.complete()) {
        QStringList missing;
        if (r.x < 0) missing << roleText(RoleX);
        if (r.y < 0) missing << roleText(RoleY);
        if (r.value < 0 && !r.usesComponents())
            missing << (r.component[0] >= 0 || r.component[1] >= 0 || r.component[2] >= 0 ? tr("分量 1、2、3") : roleText(RoleValue));
        text = tr("还需指定：%1").arg(missing.join(QStringLiteral("、")));
        kind = Chip::Warn;
    } else {
        text = summary();
        if (bad > 0) {
            text += tr("。预览中有 %1 个格子不是数字，导入时这些行会被跳过。").arg(bad);
            kind = Chip::Warn;
        }
        if (!settings_.missingValue.isEmpty()) {
            double v;
            if (!DataIO::parseNumber(settings_.missingValue, v)) {
                text = tr("无效值“%1”不是数字").arg(settings_.missingValue);
                kind = Chip::Error;
            }
        }
    }
    const QString colour = tm.color(kind == Chip::Error ? "err" : kind == Chip::Warn ? "warn" : "ok").name();
    status_->setText(QStringLiteral("<span style=\"color:%1\">%2</span>").arg(colour, text.toHtmlEscaped()));
    ok_->setEnabled(preview_.ok() && r.complete() && kind != Chip::Error);
}

QString DataImportDialog::summary() const
{
    const DataIO::ColumnRoles &r = settings_.roles;
    auto name = [this](int c) { return c >= 0 && c < preview_.headers.size() ? preview_.headers[c] : QStringLiteral("?"); };
    QStringList parts;
    if (preview_.kind == DataIO::FileKind::Text) {
        QString how = tr("%1分隔").arg(DataIO::delimiterName(preview_.delimiter));
        if (settings_.text.skipLines > 0)
            how += tr("，跳过开头 %1 行").arg(settings_.text.skipLines);
        if (preview_.hasHeader)
            how += tr("，有列标题");
        parts << how;
    } else {
        parts << preview_.format;
    }
    if (settings_.skipColumns > 0)
        parts << tr("跳过前 %1 列").arg(settings_.skipColumns);
    QString cols = tr("X = %1，Y = %2，").arg(name(r.x), name(r.y));
    cols += r.usesComponents() ? tr("磁场值 = |%1, %2, %3|").arg(name(r.component[0]), name(r.component[1]), name(r.component[2]))
                               : tr("磁场值 = %1").arg(name(r.value));
    parts << cols;
    if (!settings_.missingValue.isEmpty())
        parts << tr("无效值 %1").arg(settings_.missingValue);
    return parts.join(QStringLiteral("；"));
}
