#include "logtextbrowser.h"

#include "thememanager.h"

#include <QFont>
#include <QScrollBar>

LogTextBrowser::LogTextBrowser(QWidget *parent) : QTextBrowser(parent)
{
    QFont mono;
    mono.setFamilies({QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"), QStringLiteral("Courier New")});
    mono.setStyleHint(QFont::Monospace);
    mono.setPixelSize(12);
    setFont(mono);
    setOpenExternalLinks(false);
    document()->setDocumentMargin(6);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &LogTextBrowser::rebuild);
}

LogTextBrowser::Level LogTextBrowser::detect(const QString &text)
{
    static const char *errorWords[] = {"错误", "失败", "异常", "出错", "无法", "未能", "崩溃"};
    static const char *warnWords[] = {"警告", "提示", "注意", "未找到", "为空", "不存在", "过少", "过多"};
    for (const char *w : errorWords)
        if (text.contains(QString::fromUtf8(w)))
            return Error;
    for (const char *w : warnWords)
        if (text.contains(QString::fromUtf8(w)))
            return Warn;
    return Info;
}

void LogTextBrowser::append(const QString &text)
{
    const QStringList lines = text.split('\n');
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (!line.isEmpty())
            log(detect(line), line);
    }
}

void LogTextBrowser::log(Level level, const QString &text)
{
    Entry e{QTime::currentTime(), level, text};
    entries_.push_back(e);
    if (level == Warn)
        ++warnings_;
    else if (level == Error)
        ++errors_;
    emit countsChanged(warnings_, errors_);

    if (entries_.size() > 3200) {
        entries_.remove(0, entries_.size() - 3000);
        rebuild();
        return;
    }
    if (int(level) >= minLevel_)
        renderEntry(e);
}

void LogTextBrowser::renderEntry(const Entry &e)
{
    const ThemeManager &tm = ThemeManager::instance();
    QString levelName, levelColor, textColor = tm.hex("t2");
    switch (e.level) {
    case Error: levelName = "ERROR"; levelColor = tm.hex("err"); textColor = tm.hex("t1"); break;
    case Warn: levelName = "WARN "; levelColor = tm.hex("warn"); textColor = tm.hex("t1"); break;
    default: levelName = "INFO "; levelColor = tm.hex("t3"); break;
    }
    const QString html = QStringLiteral("<span style=\"color:%1\">%2</span>&nbsp;&nbsp;<span style=\"color:%3;font-weight:%4\">%5</span>"
                                        "&nbsp;&nbsp;<span style=\"color:%6\">%7</span>")
                             .arg(tm.hex("t3"), e.time.toString("hh:mm:ss"), levelColor, e.level == Info ? "400" : "600",
                                  levelName.toHtmlEscaped().replace(' ', QStringLiteral("&nbsp;")), textColor,
                                  e.text.toHtmlEscaped());
    QTextBrowser::append(html);
}

void LogTextBrowser::rebuild()
{
    QTextBrowser::clear();
    for (const Entry &e : qAsConst(entries_))
        if (int(e.level) >= minLevel_)
            renderEntry(e);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void LogTextBrowser::setMinimumLevel(int minLevel)
{
    minLevel_ = minLevel;
    rebuild();
}

void LogTextBrowser::clearLog()
{
    entries_.clear();
    warnings_ = errors_ = 0;
    QTextBrowser::clear();
    emit countsChanged(0, 0);
}
