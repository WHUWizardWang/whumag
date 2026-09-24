#ifndef TABLEIMPORT_H
#define TABLEIMPORT_H

#include <QString>
#include <QStringList>
#include <QVector>

// Reading survey data files of many layouts into the program's own format (one point per line:
// "x y value", space separated).  Text files (.txt / .csv / .dat / .xyz ...) with any delimiter,
// header and comment lines, and the binary formats of binarytables.h (NASA CDF, netCDF classic).
// No GUI dependency: the import dialog shows a preview and lets the user override the automatic choices.
namespace DataIO {

enum class Delimiter { Auto, Whitespace, Comma, Semicolon, Tab, Pipe, Custom };
enum class HeaderMode { Auto, FirstLine, None };
enum class TextEncoding { Auto, Utf8, Gbk };

struct TextOptions
{
    TextEncoding encoding = TextEncoding::Auto;
    Delimiter delimiter = Delimiter::Auto;
    QString customDelimiter;
    int skipLines = 0;              // lines dropped at the top of the file before anything else
    HeaderMode header = HeaderMode::Auto;
    QString commentPrefix = QStringLiteral("#");
};

// Column roles (indices into the preview columns, -1 = unused).  The value is either one column or the
// magnitude of three components (total field from vector data).
struct ColumnRoles
{
    int x = -1;
    int y = -1;
    int value = -1;
    int component[3] = {-1, -1, -1};
    bool usesComponents() const { return value < 0 && component[0] >= 0 && component[1] >= 0 && component[2] >= 0; }
    bool complete() const { return x >= 0 && y >= 0 && (value >= 0 || usesComponents()); }
};

struct ImportSettings
{
    TextOptions text;
    int skipColumns = 0;            // leading columns dropped (text and binary)
    ColumnRoles roles;
    QString missingValue;           // e.g. "99999": rows with this x / y / value are skipped (empty: none)
};

enum class FileKind { Text, NasaCdf, NetCdf, Hdf5 };

struct Preview
{
    QString error;
    FileKind kind = FileKind::Text;
    QString format;                 // "文本（逗号分隔）", "NASA CDF 3.8", ...
    QStringList headers;            // one per column, after the skipped columns
    QStringList units;              // may be empty strings
    QVector<QStringList> rows;      // the first rows, as text
    qint64 totalRows = -1;          // known for binary files
    // what the automatic text settings resolved to
    Delimiter delimiter = Delimiter::Whitespace;
    bool hasHeader = false;
    int headerLines = 0;            // header + unit lines after skipLines
    QString encoding;
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
};

Preview loadPreview(const QString &path, const ImportSettings &settings, int maxRows = 100);

// Roles from the column names (经度 / lon / longitude / x ...), else the first numeric columns.
ColumnRoles guessRoles(const Preview &preview);

struct ImportResult
{
    QString error;
    qint64 written = 0;
    qint64 skipped = 0;
    QStringList skippedExamples;    // "第 12 行：磁场值不是数字（“--”）", at most a few
    double xMin = 0, xMax = 0, yMin = 0, yMax = 0, vMin = 0, vMax = 0;
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
};

// Writes "x y value" lines to |outPath|.
ImportResult importToXyz(const QString &path, const ImportSettings &settings, const QString &outPath);

QString delimiterName(Delimiter d);

// Number parsing used for text cells: C-locale numbers, Fortran "1.0D+03", decimal comma ("12,5" when
// the delimiter is not a comma), hemisphere letters ("120.5E", "S30.2") and degree-minute-second
// ("120°30'15.2\"E", "120°30.25'").
bool parseNumber(const QString &cell, double &value, bool decimalComma = false);

} // namespace DataIO

#endif // TABLEIMPORT_H
