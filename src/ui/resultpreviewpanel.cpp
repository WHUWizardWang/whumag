#include "resultpreviewpanel.h"

#include "formkit.h"
#include "thememanager.h"
#include "uiscale.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QVariant>

ResultPreviewPanel::ResultPreviewPanel(const QList<QWidget *> &pages, QTextBrowser *log, QWidget *parent)
    : QWidget(parent), pages_(pages), log_(log), emptyTitle_(tr("尚无预览")),
      emptyHint_(tr("设置左侧参数并开始处理后，热力图与等值线会显示在这里"))
{
    setObjectName("resultPreview");
    setAttribute(Qt::WA_StyledBackground, true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 18);
    root->setSpacing(12);

    auto *title = new QLabel(tr("结果预览"));
    title->setProperty("role", QStringLiteral("h2"));
    chip_ = new Chip(tr("未开始"), Chip::Neutral);
    auto *head = new QHBoxLayout;
    head->setSpacing(10);
    head->addWidget(title);
    head->addWidget(chip_);
    head->addStretch(1);
    root->addLayout(head);

    auto *card = new QFrame;
    card->setProperty("role", QStringLiteral("card"));
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(10, 10, 10, 10);
    cl->setSpacing(0);
    empty_ = new QLabel;
    empty_->setAlignment(Qt::AlignCenter);
    empty_->setTextFormat(Qt::RichText);
    cl->addWidget(empty_, 1);
    for (QWidget *page : pages_)
        cl->addWidget(page, 1);
    root->addWidget(card, 1);

    root->addWidget(FormKit::caption(tr("日志")));
    log_->setFixedHeight(UiScale::dp(120));
    root->addWidget(log_);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &ResultPreviewPanel::restyle);
    restyle();
    refreshEmptyState();
}

void ResultPreviewPanel::setStatus(const QString &text, Chip::Kind kind)
{
    chip_->setText(text);
    chip_->setKind(kind);
}

void ResultPreviewPanel::refreshEmptyState()
{
    bool hasPreview = false;
    for (QWidget *page : pages_)
        if (!page->isHidden() && page->layout() && page->layout()->count() > 0)
            hasPreview = true;
    empty_->setVisible(!hasPreview);
}

void ResultPreviewPanel::restyle()
{
    const ThemeManager &tm = ThemeManager::instance();
    setStyleSheet(QStringLiteral("#resultPreview { background: %1; border: none; }").arg(tm.hex("n2")));
    log_->setStyleSheet(QStringLiteral(
        "QTextBrowser { background: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 4px 8px;"
        " font-family: 'Cascadia Mono', Consolas, 'Courier New', monospace; font-size: 9pt; }")
                            .arg(tm.hex("logBg"), tm.hex("t2"), tm.hex("line")));
    empty_->setText(QStringLiteral("<div style='font-size:9.75pt; font-weight:600; color:%1;'>%3</div>"
                                   "<div style='font-size:9pt; color:%2;'>%4</div>")
                        .arg(tm.hex("t2"), tm.hex("t3"), emptyTitle_.toHtmlEscaped(), emptyHint_.toHtmlEscaped()));
}

void ResultPreviewPanel::setEmptyText(const QString &title, const QString &hint)
{
    emptyTitle_ = title;
    emptyHint_ = hint;
    restyle();
}
