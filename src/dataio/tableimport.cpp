#include "tableimport.h"

#include "binarytables.h"

#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMap>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextCodec>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <new>

namespace DataIO {

namespace {

constexpr int kSampleLines = 200;   // lines looked at for the automatic text settings

// ---------------------------------------------------------------- text lines
class LineSource
{
public:
    bool open(const QString &path, TextEncoding encoding, QString &encodingName, QString &error)
    {
        file_.setFileName(path);
        if (!file_.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("无法打开文件：%1").arg(file_.errorString());
            return false;
        }
        const QByteArray head = file_.peek(256 * 1024);
        if (head.count('\0') > head.size() / 20 && !head.startsWith("\xFF\xFE") && !head.startsWith("\xFE\xFF")) {
            error = QStringLiteral("无法识别的二进制文件（支持文本、NASA CDF 和 netCDF classic）");
            return false;
        }
        QTextCodec *codec = nullptr;
        if (head.startsWith("\xEF\xBB\xBF") || head.startsWith("\xFF\xFE") || head.startsWith("\xFE\xFF")) {
            codec = QTextCodec::codecForUtfText(head);
        } else if (encoding == TextEncoding::Utf8) {
            codec = QTextCodec::codecForName("UTF-8");
        } else if (encoding == TextEncoding::Gbk) {
            codec = QTextCodec::codecForName("GB18030");
        } else {
            QTextCodec::ConverterState state;
            QTextCodec::codecForName("UTF-8")->toUnicode(head.constData(), head.size(), &state);
            codec = QTextCodec::codecForName(state.invalidChars == 0 ? "UTF-8" : "GB18030");
        }
        if (!codec)
            codec = QTextCodec::codecForName("UTF-8");
        encodingName = QString::fromLatin1(codec->name());
        if (encodingName == QLatin1String("GB18030"))
            encodingName = QStringLiteral("GBK / GB18030");
        stream_.setDevice(&file_);
        stream_.setCodec(codec);
        return true;
    }
    bool next(QString &line)
    {
        if (stream_.atEnd())
            return false;
        line = stream_.readLine();
        ++lineNo_;
        return true;
    }
    qint64 lineNo() const { return lineNo_; }

private:
    QFile file_;
    QTextStream stream_;
    qint64 lineNo_ = 0;
};

QString stripQuotes(QString s)
{
    s = s.trimmed();
    if (s.size() >= 2 && ((s.startsWith('"') && s.endsWith('"')) || (s.startsWith('\'') && s.endsWith('\''))))
        s = s.mid(1, s.size() - 2).trimmed();
    return s;
}

QChar delimiterChar(Delimiter d)
{
    switch (d) {
    case Delimiter::Comma: return QLatin1Char(',');
    case Delimiter::Semicolon: return QLatin1Char(';');
    case Delimiter::Tab: return QLatin1Char('\t');
    case Delimiter::Pipe: return QLatin1Char('|');
    default: return QChar();
    }
}

QStringList splitLine(const QString &line, Delimiter d, const QString &custom)
{
    if (d == Delimiter::Whitespace || d == Delimiter::Auto) {
        static const QRegularExpression ws(QStringLiteral("\\s+"));
        return line.trimmed().split(ws, Qt::SkipEmptyParts);
    }
    if (d == Delimiter::Custom && custom.size() != 1) {
        if (custom.isEmpty())
            return splitLine(line, Delimiter::Whitespace, QString());
        QStringList cells = line.split(custom);
        for (QString &c : cells)
            c = stripQuotes(c);
        return cells;
    }
    const QChar sep = d == Delimiter::Custom ? custom.at(0) : delimiterChar(d);
    QStringList cells;
    QString cur;
    bool quoted = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == QLatin1Char('"') && !quoted && cur.trimmed().isEmpty()) {
            quoted = true;   // a quote opens a field only at its start (so 30°15'20" stays a value)
        } else if (ch == QLatin1Char('"') && quoted) {
            if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                cur += ch;   // "" inside quotes
                ++i;
            } else {
                quoted = false;
            }
        } else if (ch == sep && !quoted) {
            cells << cur.trimmed();
            cur.clear();
        } else {
            cur += ch;
        }
    }
    cells << cur.trimmed();
    // a trailing delimiter ("1,2,3,") does not make an extra column
    if (cells.size() > 1 && cells.last().isEmpty() && line.trimmed().endsWith(sep))
        cells.removeLast();
    return cells;
}

bool isComment(const QString &line, const QString &prefix)
{
    const QString t = line.trimmed();
    return t.isEmpty() || (!prefix.isEmpty() && t.startsWith(prefix));
}

bool decimalCommaFor(Delimiter d, const QString &custom)
{
    return d != Delimiter::Comma && !(d == Delimiter::Custom && custom.contains(QLatin1Char(',')));
}

int numericCount(const QStringList &cells, int skipColumns, bool decimalComma)
{
    int n = 0;
    double v;
    for (int i = skipColumns; i < cells.size(); ++i)
        if (parseNumber(cells[i], v, decimalComma))
            ++n;
    return n;
}

// Chooses the delimiter that splits the sample lines into the same number (>= 2) of fields.
Delimiter detectDelimiter(const QStringList &lines)
{
    const Delimiter order[] = {Delimiter::Tab, Delimiter::Semicolon, Delimiter::Comma, Delimiter::Pipe, Delimiter::Whitespace};
    Delimiter best = Delimiter::Whitespace;
    double bestScore = -1;
    for (Delimiter d : order) {
        QMap<int, int> counts;
        for (const QString &l : lines)
            ++counts[splitLine(l, d, QString()).size()];
        int mode = 0, freq = 0;
        for (auto it = counts.begin(); it != counts.end(); ++it)
            if (it.value() > freq || (it.value() == freq && it.key() > mode)) {
                mode = it.key();
                freq = it.value();
            }
        if (mode < 2 || lines.isEmpty())
            continue;
        const double share = double(freq) / lines.size();
        if (share >= 0.8)
            return d;
        if (share > bestScore) {
            bestScore = share;
            best = d;
        }
    }
    return best;
}

// Resolved text layout: the automatic choices made concrete.
struct TextLayout
{
    QString error;
    QString encoding;
    Delimiter delimiter = Delimiter::Whitespace;
    QString custom;
    bool decimalComma = false;
    int headerLines = 0;
    QStringList headerCells;
    QStringList unitCells;
    QStringList notes;
    QVector<QStringList> sample;   // data lines (split), for the preview
    int columns = 0;
};

TextLayout resolveText(const QString &path, const ImportSettings &s, int maxRows)
{
    TextLayout out;
    LineSource src;
    if (!src.open(path, s.text.encoding, out.encoding, out.error))
        return out;
    QString line;
    for (int i = 0; i < s.text.skipLines; ++i)
        if (!src.next(line))
            break;
    QStringList lines;   // non-comment lines after the skipped ones
    const int want = qMax(kSampleLines, maxRows + 10);
    while (lines.size() < want && src.next(line))
        if (!isComment(line, s.text.commentPrefix))
            lines << line;
    if (lines.isEmpty()) {
        out.error = QStringLiteral("跳过前 %1 行后没有数据行").arg(s.text.skipLines);
        return out;
    }

    out.delimiter = s.text.delimiter == Delimiter::Auto ? detectDelimiter(lines) : s.text.delimiter;
    out.custom = s.text.customDelimiter;
    out.decimalComma = decimalCommaFor(out.delimiter, out.custom);

    QVector<QStringList> split;
    for (const QString &l : lines)
        split << splitLine(l, out.delimiter, out.custom);

    if (s.text.header == HeaderMode::FirstLine) {
        out.headerLines = 1;
    } else if (s.text.header == HeaderMode::Auto) {
        // typical number of numeric cells of a data line: median over the second half of the sample
        std::vector<int> counts;
        for (int i = split.size() / 2; i < split.size(); ++i)
            counts.push_back(numericCount(split[i], s.skipColumns, out.decimalComma));
        std::nth_element(counts.begin(), counts.begin() + counts.size() / 2, counts.end());
        const int typical = counts[counts.size() / 2];
        while (out.headerLines < 5 && out.headerLines < split.size() - 1
               && numericCount(split[out.headerLines], s.skipColumns, out.decimalComma) < typical)
            ++out.headerLines;
    }
    int dataCells = 0;
    for (int i = out.headerLines; i < split.size(); ++i) {
        dataCells = qMax(dataCells, split[i].size());
        if (out.sample.size() < maxRows)
            out.sample << split[i];
    }
    if (out.headerLines > 0) {
        // the column names: the first leading line with as many cells as a data line (with "first line is
        // the header" chosen by the user, that line whatever its length); the line after it may hold units
        int names = s.text.header == HeaderMode::FirstLine ? 0 : -1;
        for (int i = 0; i < out.headerLines && names < 0; ++i)
            if (split[i].size() == dataCells)
                names = i;
        if (names >= 0) {
            out.headerCells = split[names];
            out.notes << QStringLiteral("第 %1 个非注释行作为列标题").arg(names + 1);
            if (names + 1 < out.headerLines && split[names + 1].size() == split[names].size()
                && numericCount(split[names + 1], 0, out.decimalComma) == 0)
                out.unitCells = split[names + 1];
        }
        const int other = out.headerLines - (names >= 0 ? 1 : 0);
        if (other > 0)
            out.notes << QStringLiteral("%1 行说明 / 单位行已跳过").arg(other);
    }
    out.columns = qMax(dataCells, out.headerCells.size()) - s.skipColumns;
    return out;
}

QString columnName(const QStringList &cells, int fileColumn)
{
    const QString h = fileColumn < cells.size() ? stripQuotes(cells[fileColumn]) : QString();
    return h.isEmpty() ? QStringLiteral("第 %1 列").arg(fileColumn + 1) : h;
}

NumericTable readBinary(const QString &path, FileKind kind)
{
    return kind == FileKind::NasaCdf ? readNasaCdf(path) : readNetCdf(path);
}

FileKind fileKind(const QString &path)
{
    switch (binaryKind(path)) {
    case BinaryKind::NasaCdf: return FileKind::NasaCdf;
    case BinaryKind::NetCdf: return FileKind::NetCdf;
    case BinaryKind::Hdf5: return FileKind::Hdf5;
    default: return FileKind::Text;
    }
}

const char *kHdf5Error = "该文件是 netCDF-4 / HDF5 格式，暂不支持；可用 nccopy -k classic 转换为 netCDF classic，或导出为文本";

QString normalisedName(QString n)
{
    n = n.toLower();
    static const QRegularExpression units(QStringLiteral("[\\(（\\[].*$"));
    n.remove(units);
    n.remove(QRegularExpression(QStringLiteral("[\\s_\\-\\.]")));
    return n;
}

} // namespace

QString delimiterName(Delimiter d)
{
    switch (d) {
    case Delimiter::Auto: return QStringLiteral("自动");
    case Delimiter::Whitespace: return QStringLiteral("空格 / 制表符");
    case Delimiter::Comma: return QStringLiteral("逗号");
    case Delimiter::Semicolon: return QStringLiteral("分号");
    case Delimiter::Tab: return QStringLiteral("制表符");
    case Delimiter::Pipe: return QStringLiteral("竖线");
    case Delimiter::Custom: return QStringLiteral("自定义");
    }
    return QString();
}

bool parseNumber(const QString &cell, double &value, bool decimalComma)
{
    QString s = stripQuotes(cell);
    if (s.isEmpty())
        return false;
    bool ok = false;
    value = QLocale::c().toDouble(s, &ok);
    if (ok)
        return std::isfinite(value);

    static const QRegularExpression fortran(QStringLiteral("^[+-]?(\\d+\\.?\\d*|\\.\\d+)[dD][+-]?\\d+$"));
    if (fortran.match(s).hasMatch()) {
        value = QLocale::c().toDouble(QString(s).replace(QLatin1Char('d'), QLatin1Char('e')).replace(QLatin1Char('D'), QLatin1Char('e')), &ok);
        return ok && std::isfinite(value);
    }
    static const QRegularExpression comma(QStringLiteral("^[+-]?\\d+,\\d+$"));
    if (decimalComma && comma.match(s).hasMatch()) {
        value = QLocale::c().toDouble(QString(s).replace(QLatin1Char(','), QLatin1Char('.')), &ok);
        return ok && std::isfinite(value);
    }

    // hemisphere letter in front or at the end
    double sign = 1;
    static const QString hemis = QStringLiteral("NSEWnsew");
    if (s.size() > 1 && hemis.contains(s.back())) {
        if (s.back().toUpper() == QLatin1Char('S') || s.back().toUpper() == QLatin1Char('W'))
            sign = -1;
        s.chop(1);
    } else if (s.size() > 1 && hemis.contains(s.front())) {
        if (s.front().toUpper() == QLatin1Char('S') || s.front().toUpper() == QLatin1Char('W'))
            sign = -1;
        s.remove(0, 1);
    }
    s = s.trimmed();
    value = QLocale::c().toDouble(s, &ok);
    if (ok && sign < 0 && value < 0)
        return false;   // "-30S" is contradictory
    if (!ok) {
        static const QRegularExpression dms(QString::fromUtf8(
            "^([+-]?\\d+(?:\\.\\d+)?)\\s*[°º]\\s*(?:(\\d+(?:\\.\\d+)?)\\s*['′]\\s*)?(?:(\\d+(?:\\.\\d+)?)\\s*(?:\"|″|'')\\s*)?$"));
        const QRegularExpressionMatch m = dms.match(s);
        if (!m.hasMatch())
            return false;
        const double deg = m.captured(1).toDouble();
        const double mins = m.captured(2).isEmpty() ? 0 : m.captured(2).toDouble();
        const double secs = m.captured(3).isEmpty() ? 0 : m.captured(3).toDouble();
        if (mins >= 60 || secs > 60)   // 60.00 seconds: rounded by the writing program
            return false;
        value = std::fabs(deg) + mins / 60 + secs / 3600;
        if (m.captured(1).startsWith(QLatin1Char('-')))
            value = -value;
    }
    value *= sign;
    return std::isfinite(value);
}

Preview loadPreview(const QString &path, const ImportSettings &settings, int maxRows)
{
    Preview p;
    if (!QFileInfo::exists(path)) {
        p.error = QStringLiteral("文件不存在");
        return p;
    }
    p.kind = fileKind(path);
    if (p.kind == FileKind::Hdf5) {
        p.error = QString::fromUtf8(kHdf5Error);
        return p;
    }
    const int skip = qMax(0, settings.skipColumns);
    if (p.kind != FileKind::Text) {
        const NumericTable t = readBinary(path, p.kind);
        if (!t.ok()) {
            p.error = t.error;
            return p;
        }
        p.format = t.format;
        p.notes = t.notes;
        p.totalRows = qint64(t.rowCount());
        for (size_t c = size_t(skip); c < t.columns.size(); ++c) {
            p.headers << t.columns[c].name;
            p.units << t.columns[c].unit;
        }
        const size_t rows = std::min<size_t>(t.rowCount(), size_t(maxRows));
        for (size_t r = 0; r < rows; ++r) {
            QStringList cells;
            for (size_t c = size_t(skip); c < t.columns.size(); ++c)
                cells << formatValue(t.columns[c], t.columns[c].values[r]);
            p.rows << cells;
        }
        if (p.headers.isEmpty())
            p.error = QStringLiteral("跳过前 %1 列后没有剩余的列").arg(skip);
        return p;
    }

    const TextLayout lay = resolveText(path, settings, maxRows);
    if (!lay.error.isEmpty()) {
        p.error = lay.error;
        return p;
    }
    p.delimiter = lay.delimiter;
    p.hasHeader = lay.headerLines > 0;
    p.headerLines = lay.headerLines;
    p.encoding = lay.encoding;
    p.notes = lay.notes;
    p.format = QStringLiteral("文本（%1分隔，%2）").arg(delimiterName(lay.delimiter), lay.encoding);
    if (lay.columns <= 0) {
        p.error = QStringLiteral("跳过前 %1 列后没有剩余的列").arg(skip);
        return p;
    }
    for (int c = 0; c < lay.columns; ++c) {
        p.headers << columnName(lay.headerCells, skip + c);
        p.units << (skip + c < lay.unitCells.size() ? stripQuotes(lay.unitCells[skip + c]) : QString());
    }
    for (const QStringList &cells : lay.sample) {
        QStringList row;
        for (int c = 0; c < lay.columns; ++c)
            row << (skip + c < cells.size() ? stripQuotes(cells[skip + c]) : QString());
        p.rows << row;
    }
    return p;
}

ColumnRoles guessRoles(const Preview &p)
{
    ColumnRoles r;
    const int n = p.headers.size();
    // numeric share of each column in the preview rows
    QVector<bool> numeric(n, false), integral(n, true);
    const bool dc = p.kind == FileKind::Text && decimalCommaFor(p.delimiter, QString());
    for (int c = 0; c < n; ++c) {
        int good = 0, total = 0;
        for (const QStringList &row : p.rows) {
            if (c >= row.size() || row[c].isEmpty())
                continue;
            double v;
            ++total;
            if (parseNumber(row[c], v, dc)) {
                ++good;
                if (v != std::floor(v))
                    integral[c] = false;
            }
        }
        numeric[c] = total > 0 && good >= 0.8 * total;
    }
    // names in order of preference, then substrings
    auto find = [&](const QStringList &names, const QStringList &contains) {
        auto free = [&](int c) { return numeric[c] && c != r.x && c != r.y && c != r.value; };
        for (const QString &name : names)
            for (int c = 0; c < n; ++c)
                if (free(c) && normalisedName(p.headers[c]) == name)
                    return c;
        for (const QString &part : contains)
            for (int c = 0; c < n; ++c)
                if (free(c) && normalisedName(p.headers[c]).contains(part))
                    return c;
        return -1;
    };
    r.x = find({QStringLiteral("lon"), QStringLiteral("long"), QStringLiteral("longitude"), QStringLiteral("lng"),
                QStringLiteral("x"), QStringLiteral("l"), QStringLiteral("easting"), QStringLiteral("east")},
               {QStringLiteral("经度"), QStringLiteral("longitude")});
    r.y = find({QStringLiteral("lat"), QStringLiteral("latitude"), QStringLiteral("y"), QStringLiteral("b"),
                QStringLiteral("northing"), QStringLiteral("north")},
               {QStringLiteral("纬度"), QStringLiteral("latitude")});
    r.value = find({QStringLiteral("f"), QStringLiteral("t"), QStringLiteral("mag"), QStringLiteral("magnetic"), QStringLiteral("tmi"),
                    QStringLiteral("total"), QStringLiteral("totalfield"), QStringLiteral("value"), QStringLiteral("z"),
                    QStringLiteral("ta"), QStringLiteral("dt"), QStringLiteral("anomaly"), QStringLiteral("fscalar")},
                   {QStringLiteral("磁"), QStringLiteral("总场"), QStringLiteral("异常")});
    if (r.value < 0) {
        // a column with the unit nT
        for (int c = 0; c < n && r.value < 0; ++c)
            if (numeric[c] && c != r.x && c != r.y && c < p.units.size() && p.units[c].trimmed().compare(QLatin1String("nT"), Qt::CaseInsensitive) == 0
                && !p.headers[c].contains(QLatin1Char('[')))
                r.value = c;
    }
    if (r.value < 0) {
        // vector data: three components name[0..2]; B_NEC preferred
        // (exactly three: a quaternion q[0..3] is not a field vector)
        int bestScore = -1;
        for (int c = 0; c + 2 < n; ++c) {
            const QString h = p.headers[c];
            if (!h.endsWith(QLatin1String("[0]")))
                continue;
            const QString base = h.left(h.size() - 3);
            if (p.headers[c + 1] != base + QLatin1String("[1]") || p.headers[c + 2] != base + QLatin1String("[2]")
                || (c + 3 < n && p.headers[c + 3] == base + QLatin1String("[3]")))
                continue;
            const int score = (base.startsWith(QLatin1Char('B'), Qt::CaseInsensitive) ? 2 : 0)
                            + (base.contains(QLatin1String("NEC"), Qt::CaseInsensitive) ? 1 : 0);
            if (score > bestScore) {
                bestScore = score;
                for (int k = 0; k < 3; ++k)
                    r.component[k] = c + k;
            }
        }
    }
    // the remaining roles: the first numeric columns not used yet, passing over columns of whole numbers
    // (point numbers, counters, time stamps) while there are enough columns with fractions
    int fractional = 0;
    for (int c = 0; c < n; ++c)
        if (numeric[c] && !integral[c])
            ++fractional;
    auto next = [&](bool allowIntegral) {
        for (int c = 0; c < n; ++c)
            if (numeric[c] && (allowIntegral || !integral[c]) && c != r.x && c != r.y && c != r.value
                && c != r.component[0] && c != r.component[1] && c != r.component[2])
                return c;
        return -1;
    };
    const bool skipIntegral = fractional >= 2;   // coordinates have fractions; the field value may not
    if (r.x < 0) r.x = next(!skipIntegral);
    if (r.y < 0) r.y = next(!skipIntegral);
    if (r.value < 0 && !r.usesComponents()) {
        // usually the column after the coordinates
        for (int c = qMax(r.x, r.y) + 1; c < n && r.value < 0; ++c)
            if (numeric[c] && c != r.x && c != r.y)
                r.value = c;
        if (r.value < 0)
            r.value = next(true);
    }
    return r;
}

ImportResult importToXyz(const QString &path, const ImportSettings &s, const QString &outPath)
{
    ImportResult res;
    try {
        if (!s.roles.complete()) {
            res.error = QStringLiteral("请指定 X（经度）、Y（纬度）和磁场值所在的列");
            return res;
        }
        double missing = 0;
        const bool hasMissing = !s.missingValue.trimmed().isEmpty();
        if (hasMissing && !parseNumber(s.missingValue, missing)) {
            res.error = QStringLiteral("无效值“%1”不是数字").arg(s.missingValue);
            return res;
        }
        auto isMissing = [&](double v) {
            return hasMissing && std::fabs(v - missing) <= 1e-9 * qMax(1.0, std::fabs(missing));
        };

        QSaveFile out(outPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            res.error = QStringLiteral("无法写入 %1：%2").arg(outPath, out.errorString());
            return res;
        }
        QByteArray buffer;
        buffer.reserve(1 << 20);
        res.xMin = res.yMin = res.vMin = std::numeric_limits<double>::max();
        res.xMax = res.yMax = res.vMax = std::numeric_limits<double>::lowest();
        const char *roleNames[] = {"X", "Y", "磁场值", "分量 1", "分量 2", "分量 3"};
        const int cols[] = {s.roles.x, s.roles.y, s.roles.value, s.roles.component[0], s.roles.component[1], s.roles.component[2]};
        auto skip = [&](qint64 row, const QString &why) {
            ++res.skipped;
            if (res.skippedExamples.size() < 5)
                res.skippedExamples << QStringLiteral("第 %1 行：%2").arg(row).arg(why);
        };
        auto emitPoint = [&](const double (&v)[6]) {
            const double value = s.roles.usesComponents() ? std::sqrt(v[3] * v[3] + v[4] * v[4] + v[5] * v[5]) : v[2];
            buffer += QByteArray::number(v[0], 'g', 12);
            buffer += ' ';
            buffer += QByteArray::number(v[1], 'g', 12);
            buffer += ' ';
            buffer += QByteArray::number(value, 'g', 12);
            buffer += '\n';
            if (buffer.size() > (1 << 20) - 256) {
                out.write(buffer);
                buffer.clear();
            }
            res.xMin = qMin(res.xMin, v[0]); res.xMax = qMax(res.xMax, v[0]);
            res.yMin = qMin(res.yMin, v[1]); res.yMax = qMax(res.yMax, v[1]);
            res.vMin = qMin(res.vMin, value); res.vMax = qMax(res.vMax, value);
            ++res.written;
        };
        const int skipCols = qMax(0, s.skipColumns);

        const FileKind kind = fileKind(path);
        if (kind == FileKind::Hdf5) {
            res.error = QString::fromUtf8(kHdf5Error);
            return res;
        }
        if (kind != FileKind::Text) {
            const NumericTable t = readBinary(path, kind);
            if (!t.ok()) {
                res.error = t.error;
                return res;
            }
            for (int k = 0; k < 6; ++k)
                if (cols[k] >= 0 && size_t(cols[k] + skipCols) >= t.columns.size()) {
                    res.error = QStringLiteral("列设置超出文件的列数");
                    return res;
                }
            for (size_t r = 0; r < t.rowCount(); ++r) {
                double v[6] = {0, 0, 0, 0, 0, 0};
                QString why;
                for (int k = 0; k < 6 && why.isEmpty(); ++k) {
                    if (cols[k] < 0)
                        continue;
                    v[k] = t.columns[size_t(cols[k] + skipCols)].values[r];
                    if (!std::isfinite(v[k]))
                        why = QStringLiteral("%1为空（填充值）").arg(QString::fromUtf8(roleNames[k]));
                    else if (isMissing(v[k]))
                        why = QStringLiteral("%1为无效值").arg(QString::fromUtf8(roleNames[k]));
                }
                if (why.isEmpty())
                    emitPoint(v);
                else
                    skip(qint64(r) + 1, why);
            }
            res.notes << QStringLiteral("%1，%2 条记录").arg(t.format).arg(t.rowCount());
        } else {
            const TextLayout lay = resolveText(path, s, 0);
            if (!lay.error.isEmpty()) {
                res.error = lay.error;
                return res;
            }
            LineSource src;
            QString enc;
            if (!src.open(path, s.text.encoding, enc, res.error))
                return res;
            QString line;
            for (int i = 0; i < s.text.skipLines; ++i)
                src.next(line);
            int header = lay.headerLines;
            while (src.next(line)) {
                if (isComment(line, s.text.commentPrefix))
                    continue;
                if (header > 0) {
                    --header;
                    continue;
                }
                const QStringList cells = splitLine(line, lay.delimiter, lay.custom);
                double v[6] = {0, 0, 0, 0, 0, 0};
                QString why;
                for (int k = 0; k < 6 && why.isEmpty(); ++k) {
                    if (cols[k] < 0)
                        continue;
                    const int c = cols[k] + skipCols;
                    if (c >= cells.size() || cells[c].trimmed().isEmpty())
                        why = QStringLiteral("缺少%1（第 %2 列）").arg(QString::fromUtf8(roleNames[k])).arg(c + 1);
                    else if (!parseNumber(cells[c], v[k], lay.decimalComma))
                        why = QStringLiteral("%1不是数字（“%2”）").arg(QString::fromUtf8(roleNames[k]), cells[c].left(20));
                    else if (isMissing(v[k]))
                        why = QStringLiteral("%1为无效值").arg(QString::fromUtf8(roleNames[k]));
                }
                if (why.isEmpty())
                    emitPoint(v);
                else
                    skip(src.lineNo(), why);
            }
            res.notes << QStringLiteral("文本，%1分隔，%2").arg(delimiterName(lay.delimiter), lay.encoding);
        }
        out.write(buffer);
        if (res.written == 0) {
            out.cancelWriting();
            res.error = res.skippedExamples.isEmpty() ? QStringLiteral("文件中没有数据")
                                                      : QStringLiteral("没有一行能按当前设置读出数值，例如 %1").arg(res.skippedExamples.first());
            return res;
        }
        if (!out.commit()) {
            res.error = QStringLiteral("无法写入 %1：%2").arg(outPath, out.errorString());
            return res;
        }
        if (s.roles.usesComponents())
            res.notes << QStringLiteral("磁场值由三个分量计算：√(B1² + B2² + B3²)");
        if (res.xMin < -180 || res.xMax > 360 || res.yMin < -90 || res.yMax > 90)
            res.notes << QStringLiteral("坐标超出经纬度范围（若为平面坐标可忽略）");
        else if (res.yMin >= -90 && res.yMax <= 90 && std::fabs(res.xMax - res.xMin) < 1e-12 && std::fabs(res.yMax - res.yMin) > 0)
            res.notes << QStringLiteral("X 列的数值全部相同，请检查列设置");
    } catch (const std::bad_alloc &) {
        res.error = QStringLiteral("内存不足，文件过大");
    } catch (const std::exception &e) {
        res.error = QStringLiteral("导入出错：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    return res;
}

} // namespace DataIO
