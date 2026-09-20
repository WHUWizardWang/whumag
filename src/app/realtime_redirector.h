#ifndef REALTIME_REDIRECTOR_H
#define REALTIME_REDIRECTOR_H
#include <QTextBrowser>
#include <QApplication>
#include "logtextbrowser.h"
#include <iostream>
#include <streambuf>
#include <string>

class TextBrowserStreambuf : public std::streambuf {
public:
    TextBrowserStreambuf(QTextBrowser* browser) : textBrowser(browser) {}

protected:
    virtual int overflow(int c = EOF) {
        if (c != EOF) {
            if (c == '\n') {
                // 换行时将缓冲的内容输出到TextBrowser
                emitLine(buffer);
                buffer.clear();
                QApplication::processEvents(); // 使UI能够更新
            } else {
                buffer += static_cast<char>(c);
            }
        }
        return c;
    }

    virtual std::streamsize xsputn(const char* s, std::streamsize n) {
        buffer.append(s, n);
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            emitLine(line);
            buffer.erase(0, pos + 1);
            QApplication::processEvents(); // 使UI能够更新
        }
        return n;
    }

private:
    // Lines written to std::cout go through the log widget's own append() so they get the
    // time stamp / level colouring (a plain QTextBrowser* call would bypass it).
    void emitLine(const std::string &line) {
        const QString text = QString::fromStdString(line);
        if (auto *log = dynamic_cast<LogTextBrowser *>(textBrowser))
            log->append(text);
        else
            textBrowser->append(text);
    }

    QTextBrowser* textBrowser;
    std::string buffer;
};

class QTextBrowserRedirector {
public:
    QTextBrowserRedirector(QTextBrowser* browser) : textBrowserBuf(browser) {
        // 保存原始的cout缓冲区
        oldBuf = std::cout.rdbuf();
        // 重定向cout到我们的自定义缓冲区
        std::cout.rdbuf(&textBrowserBuf);
    }

    ~QTextBrowserRedirector() {
        // 恢复原始cout缓冲区
        std::cout.rdbuf(oldBuf);
    }

private:
    std::streambuf* oldBuf;
    TextBrowserStreambuf textBrowserBuf;
};
#endif // REALTIME_REDIRECTOR_H
