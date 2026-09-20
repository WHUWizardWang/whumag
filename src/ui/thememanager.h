#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

// Owns the application look: colour tokens for the light / dark palettes, the accent colour, the
// application font, the Fusion palette and the QSS from :/theme/app.qss.  Every custom-painted
// widget reads its colours from here so a theme switch repaints the whole app consistently.
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum class Mode { Light = 0, Dark = 1, System = 2 };

    static ThemeManager &instance();

    // Reads the saved mode / accent, installs Fusion + palette + font + QSS.  Call once, right
    // after the QApplication is constructed and before any window is shown.
    void init();

    void setMode(Mode mode, bool persist = true);
    Mode mode() const { return mode_; }
    bool isDark() const { return dark_; }

    static QStringList accentNames();
    void setAccentPreset(int index);
    int accentPreset() const { return accent_; }

    // Token names are the ones used in resources/theme/app.qss: n0..n4, line, line2, t1..t3, acc,
    // accSoft, accInk, accLine, onAcc, ok/okBg/okLine, warn.., err.., info.., logBg, barTrack ...
    QColor color(const QString &token) const;
    QString hex(const QString &token) const;

signals:
    void themeChanged();

private:
    ThemeManager() = default;
    void rebuild();
    void applyFont();
    static bool systemPrefersDark();

    QHash<QString, QString> tokens_;
    QString qssTemplate_;
    Mode mode_ = Mode::Light;
    bool dark_ = false;
    int accent_ = 0;
    bool initialised_ = false;
};

#endif // THEMEMANAGER_H
