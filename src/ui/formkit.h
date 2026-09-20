#ifndef FORMKIT_H
#define FORMKIT_H

#include <QString>

class QLabel;
class QLayout;
class QTableView;
class QVBoxLayout;
class QWidget;

// Small helpers for the hand-built layouts of the redesigned forms, so captions, hints and
// window headers look the same everywhere.
namespace FormKit {

// Bold caption that sits above a field.
QLabel *caption(const QString &text);

// Grey, word-wrapped helper text.
QLabel *hint(const QString &text);

// Window header: soft icon badge, title (17px) and a one-line subtitle.
QWidget *header(const QString &iconName, const QString &title, const QString &subtitle);

// Numbered section heading ("1  选择数据") for forms that read top to bottom.
QWidget *stepHeading(int number, const QString &title);

// Light caption, the input and an optional hint stacked with even spacing.  Spin boxes, combo
// boxes and line edits are allowed to fill the width they are given.
QVBoxLayout *field(const QString &captionText, QWidget *input, const QString &hintText = QString());

// Same, for an input made of several widgets (a path edit with its "浏览" button, ...).
QVBoxLayout *field(const QString &captionText, QLayout *input, const QString &hintText = QString());

// Shows a centered title + hint on the table's viewport while its model has no rows.  The table must
// already have its model, and keep it (the hint follows that model's row changes).
void emptyStateFor(QTableView *table, const QString &title, const QString &hint);

// Sets the QSS "role" property ("primary", "ghost", "danger", "link", ...) on a widget.
void setRole(QWidget *widget, const char *role);

} // namespace FormKit

#endif // FORMKIT_H
