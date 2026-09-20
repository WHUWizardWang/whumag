#include "thememanager.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>

namespace {

QColor mixColors(const QColor &a, const QColor &b, double t)
{
    return QColor(qRound(a.red() * (1 - t) + b.red() * t),
                  qRound(a.green() * (1 - t) + b.green() * t),
                  qRound(a.blue() * (1 - t) + b.blue() * t));
}

const char *kAccentLight[] = {"#B02068", "#0E7C86", "#2456A6", "#A4550B"};
const char *kAccentDark[] = {"#C23A7B", "#14909A", "#3A6FCF", "#C7691E"};

QSettings uiSettings()
{
    return QSettings("YourCompany", "YourApp");
}

} // namespace

ThemeManager &ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

QStringList ThemeManager::accentNames()
{
    return {QStringLiteral("覆盆子"), QStringLiteral("海青"), QStringLiteral("靛蓝"), QStringLiteral("琥珀")};
}

bool ThemeManager::systemPrefersDark()
{
#ifdef Q_OS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                  QSettings::NativeFormat);
    return reg.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    return false;
#endif
}

void ThemeManager::init()
{
    QSettings s = uiSettings();
    mode_ = static_cast<Mode>(qBound(0, s.value("ui/theme", int(Mode::Light)).toInt(), 2));
    accent_ = qBound(0, s.value("ui/accent", 0).toInt(), 3);

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    applyFont();

    QFile f(":/theme/app.qss");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        qssTemplate_ = QString::fromUtf8(f.readAll());
    initialised_ = true;
    rebuild();
}

void ThemeManager::setMode(Mode mode, bool persist)
{
    mode_ = mode;
    if (persist)
        uiSettings().setValue("ui/theme", int(mode));
    rebuild();
}

void ThemeManager::setAccentPreset(int index)
{
    accent_ = qBound(0, index, 3);
    uiSettings().setValue("ui/accent", accent_);
    rebuild();
}

QColor ThemeManager::color(const QString &token) const
{
    return QColor(tokens_.value(token, QStringLiteral("#FF00FF")));
}

QString ThemeManager::hex(const QString &token) const
{
    return tokens_.value(token, QStringLiteral("#FF00FF"));
}

void ThemeManager::applyFont()
{
    QFont font = QApplication::font();
    if (QFontDatabase().families().contains(QStringLiteral("Microsoft YaHei UI")))
        font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSizeF(9.5);
    font.setStyleStrategy(QFont::PreferAntialias);
    QApplication::setFont(font);
}

void ThemeManager::rebuild()
{
    if (!initialised_)
        return;

    dark_ = (mode_ == Mode::Dark) || (mode_ == Mode::System && systemPrefersDark());
    QHash<QString, QString> t;

    if (!dark_) {
        t = {{"n0", "#E9EEF1"}, {"n1", "#F5F8FA"}, {"n2", "#FFFFFF"}, {"n3", "#EDF2F5"}, {"n4", "#DEE6EB"},
             {"line", "#D6DEE4"}, {"line2", "#BAC7D0"},
             {"t1", "#15242F"}, {"t2", "#485B68"}, {"t3", "#5B6E7C"},
             {"ok", "#1B7A4B"}, {"okBg", "#E1F3E9"}, {"okLine", "#A9D8BE"},
             {"warn", "#95590A"}, {"warnBg", "#FBEFD3"}, {"warnLine", "#E9CB8A"},
             {"err", "#B53A31"}, {"errBg", "#FBE3DF"}, {"errLine", "#EAB1AA"},
             {"info", "#26629B"}, {"infoBg", "#E1EDF7"}, {"infoLine", "#A9C8E4"},
             {"logBg", "#FFFFFF"}, {"barTrack", "#DCE5EA"}, {"mode", "light"}};
    } else {
        t = {{"n0", "#0A1219"}, {"n1", "#101A23"}, {"n2", "#16232E"}, {"n3", "#1B2C39"}, {"n4", "#243748"},
             {"line", "#22343F"}, {"line2", "#334A5B"},
             {"t1", "#E4EDF3"}, {"t2", "#A9BAC6"}, {"t3", "#8496A4"},
             {"ok", "#4CC38A"}, {"okBg", "#12301F"}, {"okLine", "#1F5A3A"},
             {"warn", "#E6A63C"}, {"warnBg", "#33260D"}, {"warnLine", "#6A4D17"},
             {"err", "#FF7B70"}, {"errBg", "#3A1815"}, {"errLine", "#7A3129"},
             {"info", "#6FB4F0"}, {"infoBg", "#12283A"}, {"infoLine", "#24506F"},
             {"logBg", "#0D161E"}, {"barTrack", "#243748"}, {"mode", "dark"}};
    }

    const QColor n1(t["n1"]);
    const QColor acc(dark_ ? kAccentDark[accent_] : kAccentLight[accent_]);
    t["acc"] = acc.name();
    t["onAcc"] = "#FFFFFF";
    if (!dark_) {
        t["accSoft"] = mixColors(n1, acc, 0.12).name();
        t["accInk"] = mixColors(acc, Qt::black, 0.14).name();
        t["accLine"] = mixColors(n1, acc, 0.42).name();
        t["accHover"] = mixColors(acc, Qt::black, 0.10).name();
        t["accPress"] = mixColors(acc, Qt::black, 0.20).name();
    } else {
        t["accSoft"] = mixColors(n1, acc, 0.30).name();
        t["accInk"] = mixColors(acc, Qt::white, 0.52).name();
        t["accLine"] = mixColors(n1, acc, 0.62).name();
        t["accHover"] = mixColors(acc, Qt::white, 0.12).name();
        t["accPress"] = mixColors(acc, Qt::black, 0.12).name();
    }
    t["accDisabled"] = mixColors(acc, n1, 0.55).name();
    t["tipBg"] = t["t1"];
    t["tipFg"] = t["n1"];
    tokens_ = t;

    QPalette pal;
    const QColor n2(t["n2"]), t1(t["t1"]), t3(t["t3"]);
    pal.setColor(QPalette::Window, n1);
    pal.setColor(QPalette::WindowText, t1);
    pal.setColor(QPalette::Base, n2);
    pal.setColor(QPalette::AlternateBase, n1);
    pal.setColor(QPalette::Text, t1);
    pal.setColor(QPalette::Button, n2);
    pal.setColor(QPalette::ButtonText, t1);
    pal.setColor(QPalette::BrightText, Qt::white);
    pal.setColor(QPalette::ToolTipBase, QColor(t["tipBg"]));
    pal.setColor(QPalette::ToolTipText, QColor(t["tipFg"]));
    pal.setColor(QPalette::PlaceholderText, t3);
    pal.setColor(QPalette::Highlight, QColor(t["accSoft"]));
    pal.setColor(QPalette::HighlightedText, QColor(t["accInk"]));
    pal.setColor(QPalette::Link, QColor(t["accInk"]));
    pal.setColor(QPalette::Light, n2);
    pal.setColor(QPalette::Midlight, QColor(t["n3"]));
    pal.setColor(QPalette::Mid, QColor(t["line"]));
    pal.setColor(QPalette::Dark, QColor(t["line2"]));
    pal.setColor(QPalette::Shadow, QColor(t["line2"]));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        pal.setColor(QPalette::Disabled, role, t3);
    QApplication::setPalette(pal);

    QString css = qssTemplate_;
    for (auto it = t.constBegin(); it != t.constEnd(); ++it)
        css.replace('@' + it.key() + '@', it.value());
    qApp->setStyleSheet(css);

    emit themeChanged();
}
