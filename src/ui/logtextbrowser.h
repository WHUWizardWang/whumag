#ifndef LOGTEXTBROWSER_H
#define LOGTEXTBROWSER_H

#include <QTextBrowser>
#include <QTime>
#include <QVector>

// Drop-in replacement for the QTextBrowser used as the main window log.  Existing code keeps
// calling append(text); each line gets a time stamp and a level (guessed from the wording) and is
// coloured accordingly.  The level filter of the log panel works on the stored entries.
class LogTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    enum Level { Info = 0, Warn = 1, Error = 2 };

    explicit LogTextBrowser(QWidget *parent = nullptr);

    // Hides QTextEdit::append() on purpose so every existing ui->textBrowser->append(...) call
    // in the application goes through the level detection.
    void append(const QString &text);
    void log(Level level, const QString &text);

    void setMinimumLevel(int minLevel);   // 0 = all, 1 = warnings and errors, 2 = errors only
    int warningCount() const { return warnings_; }
    int errorCount() const { return errors_; }
    void clearLog();

signals:
    void countsChanged(int warnings, int errors);

private:
    struct Entry {
        QTime time;
        Level level;
        QString text;
    };
    static Level detect(const QString &text);
    void renderEntry(const Entry &e);
    void rebuild();

    QVector<Entry> entries_;
    int minLevel_ = 0;
    int warnings_ = 0;
    int errors_ = 0;
};

#endif // LOGTEXTBROWSER_H
