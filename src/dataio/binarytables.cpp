#include "binarytables.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QtZlib/zlib.h>

#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>

namespace DataIO {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// Thrown on a truncated / inconsistent file; turned into NumericTable::error.
struct FormatError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// Bounds-checked view of the file (memory-mapped, or a decompressed copy).
class Bytes
{
public:
    Bytes() = default;
    Bytes(const uchar *data, qint64 size) : d_(data), n_(size) {}

    qint64 size() const { return n_; }
    const uchar *at(qint64 off, qint64 len) const
    {
        if (off < 0 || len < 0 || off > n_ || len > n_ - off)
            throw FormatError("文件被截断或结构损坏");
        return d_ + off;
    }
    quint32 u32(qint64 off) const   // headers are always big-endian
    {
        const uchar *p = at(off, 4);
        return quint32(p[0]) << 24 | quint32(p[1]) << 16 | quint32(p[2]) << 8 | quint32(p[3]);
    }
    qint32 i32(qint64 off) const { return qint32(u32(off)); }
    qint64 i64(qint64 off) const { return qint64(quint64(u32(off)) << 32 | u32(off + 4)); }
    QString text(qint64 off, qint64 len) const
    {
        const char *p = reinterpret_cast<const char *>(at(off, len));
        return QString::fromLatin1(p, int(strnlen(p, size_t(len)))).trimmed();
    }

private:
    const uchar *d_ = nullptr;
    qint64 n_ = 0;
};

// Maps the file (falls back to reading it for files that cannot be mapped).
class FileBytes
{
public:
    explicit FileBytes(const QString &path) : file_(path)
    {
        if (!file_.open(QIODevice::ReadOnly))
            throw FormatError(QStringLiteral("无法打开文件：%1").arg(file_.errorString()).toStdString());
        const qint64 n = file_.size();
        if (uchar *m = n > 0 ? file_.map(0, n) : nullptr) {
            bytes_ = Bytes(m, n);
        } else {
            copy_ = file_.readAll();
            bytes_ = Bytes(reinterpret_cast<const uchar *>(copy_.constData()), copy_.size());
        }
    }
    const Bytes &bytes() const { return bytes_; }

private:
    QFile file_;
    QByteArray copy_;
    Bytes bytes_;
};

// Swaps the bytes of one value when the data encoding differs from the machine (little-endian).
inline void fromEncoding(uchar *p, int size, bool bigEndian)
{
    if (bigEndian)
        std::reverse(p, p + size);
}

template <typename T> T load(const uchar *p, bool bigEndian)
{
    uchar b[sizeof(T)];
    std::memcpy(b, p, sizeof(T));
    fromEncoding(b, int(sizeof(T)), bigEndian);
    T v;
    std::memcpy(&v, b, sizeof(T));
    return v;
}

QByteArray gunzip(const uchar *data, qint64 size, qint64 expected)
{
    QByteArray out;
    out.resize(int(qMax<qint64>(expected, size * 4)));
    z_stream s;
    std::memset(&s, 0, sizeof(s));
    if (inflateInit2(&s, 15 + 32) != Z_OK)   // +32: zlib or gzip header, detected automatically
        throw FormatError("解压失败");
    s.next_in = const_cast<Bytef *>(data);
    s.avail_in = uInt(size);
    int ret = Z_OK;
    while (ret != Z_STREAM_END) {
        if (s.total_out >= uLong(out.size()))
            out.resize(out.size() * 2);
        s.next_out = reinterpret_cast<Bytef *>(out.data()) + s.total_out;
        s.avail_out = uInt(out.size() - int(s.total_out));
        ret = inflate(&s, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&s);
            throw FormatError("GZIP 数据损坏，无法解压");
        }
        if (ret == Z_OK && s.avail_in == 0 && s.avail_out != 0)
            break;   // input exhausted without an end marker
    }
    out.resize(int(s.total_out));
    inflateEnd(&s);
    return out;
}

// CDF run-length encoding: a zero byte followed by n stands for n + 1 zero bytes.
QByteArray unRle(const uchar *data, qint64 size)
{
    QByteArray out;
    out.reserve(int(size * 2));
    for (qint64 i = 0; i < size; ++i) {
        if (data[i] == 0) {
            if (i + 1 >= size)
                throw FormatError("RLE 数据损坏");
            out.append(int(data[i + 1]) + 1, '\0');
            ++i;
        } else {
            out.append(char(data[i]));
        }
    }
    return out;
}

// ============================================================ NASA CDF
namespace cdf {

enum RecordType { CDR = 1, GDR = 2, rVDR = 3, ADR = 4, AgrEDR = 5, VXR = 6, VVR = 7, zVDR = 8, AzEDR = 9, CCR = 10, CPR = 11, CVVR = 13 };

int typeSize(int t)
{
    switch (t) {
    case 1: case 11: case 41: case 51: case 52: return 1;
    case 2: case 12: return 2;
    case 4: case 14: case 21: case 44: return 4;
    case 8: case 22: case 31: case 33: case 45: return 8;
    case 32: return 16;
    default: return 0;
    }
}

bool isCharType(int t) { return t == 51 || t == 52; }

double decode(const uchar *p, int type, bool be)
{
    switch (type) {
    case 1: case 41: return double(qint8(p[0]));
    case 11: return double(p[0]);
    case 2: return load<qint16>(p, be);
    case 12: return load<quint16>(p, be);
    case 4: return load<qint32>(p, be);
    case 14: return load<quint32>(p, be);
    case 8: case 33: return double(load<qint64>(p, be));
    case 21: case 44: return double(load<float>(p, be));
    case 22: case 31: case 45: return load<double>(p, be);
    case 32: return load<double>(p, be) * 1000.0 + load<double>(p + 8, be) * 1e-9;   // EPOCH16 -> ms
    default: return kNaN;
    }
}

struct Variable
{
    QString name;
    bool z = false;
    int num = 0;
    int type = 0;
    int maxRec = -1;
    bool recVary = true;
    qint64 vxrHead = 0;
    qint64 cprOffset = 0;   // 0: not compressed
    int valuesPerRecord = 1;
    QString unit;
    bool hasFill = false;
    double fill = 0;
};

class Reader
{
public:
    Reader(const Bytes &b, NumericTable &t) : b_(b), table_(t) {}

    void read()
    {
        const quint32 magic1 = b_.u32(0), magic2 = b_.u32(4);
        if (magic1 == 0xCDF26002 || magic1 == 0x0000FFFF)
            throw FormatError("CDF 2.x 旧版格式暂不支持，请用 CDF 工具（cdfconvert）转换为 3.x");
        if (magic1 != 0xCDF30001)
            throw FormatError("不是 NASA CDF 文件");
        if (magic2 == 0xCCCC0001) {   // the whole file is compressed
            const qint64 ccr = 8;
            if (b_.u32(ccr + 8) != CCR)
                throw FormatError("压缩的 CDF 文件结构损坏");
            const qint64 cpr = b_.i64(ccr + 12), uSize = b_.i64(ccr + 20);
            const qint64 dataOff = ccr + 32, dataLen = b_.i64(ccr) - 32;
            QByteArray plain = decompress(cpr, b_.at(dataOff, dataLen), dataLen, uSize);
            unpacked_ = QByteArray::fromRawData("\xCD\xF3\x00\x01\x00\x00\xFF\xFF", 8) + plain;
            b_ = Bytes(reinterpret_cast<const uchar *>(unpacked_.constData()), unpacked_.size());
            table_.notes << QStringLiteral("整个文件为压缩存储，已解压");
        }

        const qint64 cdr = 8;
        if (b_.u32(cdr + 8) != CDR)
            throw FormatError("CDF 文件头损坏");
        const qint64 gdr = b_.i64(cdr + 12);
        const int version = b_.i32(cdr + 20), release = b_.i32(cdr + 24), encoding = b_.i32(cdr + 28);
        table_.format = QStringLiteral("NASA CDF %1.%2").arg(version).arg(release);
        switch (encoding) {
        case 4: case 6: case 13: bigEndian_ = false; break;
        case 1: case 2: case 5: case 7: case 9: case 11: case 12: bigEndian_ = true; break;
        default: throw FormatError(QStringLiteral("不支持的 CDF 数据编码（%1，VAX 浮点）").arg(encoding).toStdString());
        }
        if (b_.u32(gdr + 8) != GDR)
            throw FormatError("CDF 全局描述记录损坏");
        const qint64 rVdrHead = b_.i64(gdr + 12), zVdrHead = b_.i64(gdr + 20), adrHead = b_.i64(gdr + 28);
        const int rNumDims = b_.i32(gdr + 56);
        std::vector<int> rDims;
        for (int i = 0; i < rNumDims; ++i)
            rDims.push_back(b_.i32(gdr + 84 + 4 * i));

        std::vector<Variable> vars;
        readVdrChain(rVdrHead, false, rDims, vars);
        readVdrChain(zVdrHead, true, rDims, vars);
        readAttributes(adrHead, vars);
        build(vars);
    }

private:
    QByteArray decompress(qint64 cpr, const uchar *data, qint64 len, qint64 expected)
    {
        if (b_.u32(cpr + 8) != CPR)
            throw FormatError("CDF 压缩参数记录损坏");
        const int cType = b_.i32(cpr + 12);
        if (cType == 5)
            return gunzip(data, len, expected);
        if (cType == 1)
            return unRle(data, len);
        if (cType == 0)
            return QByteArray(reinterpret_cast<const char *>(data), int(len));
        throw FormatError(QStringLiteral("不支持的 CDF 压缩方式（%1，Huffman）").arg(cType).toStdString());
    }

    void readVdrChain(qint64 off, bool z, const std::vector<int> &rDims, std::vector<Variable> &vars)
    {
        int guard = 0;
        while (off > 0 && guard++ < 100000) {
            if (b_.u32(off + 8) != (z ? zVDR : rVDR))
                throw FormatError("CDF 变量描述记录损坏");
            Variable v;
            v.z = z;
            v.type = b_.i32(off + 20);
            v.maxRec = b_.i32(off + 24);
            v.vxrHead = b_.i64(off + 28);
            const qint32 flags = b_.i32(off + 44);
            v.recVary = flags & 1;
            const int numElems = b_.i32(off + 64);
            v.num = b_.i32(off + 68);
            if (flags & 4)
                v.cprOffset = b_.i64(off + 72);
            v.name = b_.text(off + 84, 256);
            std::vector<int> dims;
            qint64 p = off + 340;
            if (z) {
                const int n = b_.i32(p);
                p += 4;
                for (int i = 0; i < n; ++i, p += 4)
                    dims.push_back(b_.i32(p));
            } else {
                dims = rDims;
            }
            int count = numElems;
            for (size_t i = 0; i < dims.size(); ++i, p += 4)
                if (b_.i32(p) != 0)   // dimension variance
                    count *= dims[i];
            v.valuesPerRecord = count;
            vars.push_back(v);
            off = b_.i64(off + 12);
        }
    }

    // Only UNITS and FILLVAL of the variable-scope attributes are used.
    void readAttributes(qint64 off, std::vector<Variable> &vars)
    {
        int guard = 0;
        while (off > 0 && guard++ < 100000) {
            if (b_.u32(off + 8) != ADR)
                return;   // attributes are optional for reading the data
            const int scope = b_.i32(off + 28);
            const QString name = b_.text(off + 68, 256).toUpper();
            if ((scope == 2 || scope == 4) && (name == QLatin1String("UNITS") || name == QLatin1String("FILLVAL"))) {
                for (bool zEntries : {false, true}) {
                    qint64 e = b_.i64(off + (zEntries ? 48 : 20));
                    int g = 0;
                    while (e > 0 && g++ < 100000) {
                        const int dataType = b_.i32(e + 24), entry = b_.i32(e + 28), numElems = b_.i32(e + 32);
                        for (Variable &v : vars) {
                            if (v.z != zEntries || v.num != entry)
                                continue;
                            if (name == QLatin1String("UNITS") && isCharType(dataType))
                                v.unit = b_.text(e + 56, numElems);
                            else if (name == QLatin1String("FILLVAL") && typeSize(dataType) > 0 && !isCharType(dataType)) {
                                v.hasFill = true;
                                v.fill = decode(b_.at(e + 56, typeSize(dataType)), dataType, bigEndian_);
                            }
                        }
                        e = b_.i64(e + 12);
                    }
                }
            }
            off = b_.i64(off + 12);
        }
    }

    // Copies the records of |v| into |out| (records x valuesPerRecord doubles).
    void readRecords(const Variable &v, qint64 vxr, std::vector<double> &out, int depth)
    {
        if (depth > 32)
            throw FormatError("CDF 索引记录嵌套过深");
        const int esize = typeSize(v.type);
        const qint64 recBytes = qint64(esize) * v.valuesPerRecord;
        const qint64 records = qint64(v.maxRec) + 1;
        int guard = 0;
        while (vxr > 0 && guard++ < 1000000) {
            if (b_.u32(vxr + 8) != VXR)
                throw FormatError("CDF 变量索引记录损坏");
            const int n = b_.i32(vxr + 20), used = b_.i32(vxr + 24);
            for (int i = 0; i < used && i < n; ++i) {
                const qint64 first = b_.i32(vxr + 28 + 4 * i), last = b_.i32(vxr + 28 + 4 * n + 4 * i);
                const qint64 off = b_.i64(vxr + 28 + 8 * n + 8 * i);
                const quint32 type = b_.u32(off + 8);
                if (type == VXR) {
                    readRecords(v, off, out, depth + 1);
                    continue;
                }
                if (first < 0 || last < first)
                    continue;
                const qint64 count = qMin(last, records - 1) - first + 1;
                if (count <= 0)
                    continue;
                const uchar *data = nullptr;
                QByteArray plain;
                if (type == VVR) {
                    data = b_.at(off + 12, count * recBytes);
                } else if (type == CVVR) {
                    const qint64 cSize = b_.i64(off + 16);
                    plain = decompress(v.cprOffset, b_.at(off + 24, cSize), cSize, (last - first + 1) * recBytes);
                    if (plain.size() < count * recBytes)
                        throw FormatError("CDF 压缩数据长度不符");
                    data = reinterpret_cast<const uchar *>(plain.constData());
                } else {
                    throw FormatError("CDF 数据记录类型未知");
                }
                double *dst = out.data() + first * v.valuesPerRecord;
                const qint64 values = count * v.valuesPerRecord;
                for (qint64 k = 0; k < values; ++k)
                    dst[k] = decode(data + k * esize, v.type, bigEndian_);
            }
            vxr = b_.i64(vxr + 12);
        }
    }

    void build(const std::vector<Variable> &vars)
    {
        // columns: the record-varying numeric variables with the most records
        int maxRec = -1;
        for (const Variable &v : vars)
            if (v.recVary && !isCharType(v.type) && typeSize(v.type) > 0)
                maxRec = qMax(maxRec, v.maxRec);
        if (maxRec < 0)
            throw FormatError("文件中没有按记录存储的数值变量");
        QStringList skipped;
        for (const Variable &v : vars) {
            if (!v.recVary || isCharType(v.type) || typeSize(v.type) == 0 || v.maxRec != maxRec || v.valuesPerRecord < 1) {
                skipped << v.name;
                continue;
            }
            if (v.valuesPerRecord > 64) {
                skipped << QStringLiteral("%1（每条记录 %2 个值）").arg(v.name).arg(v.valuesPerRecord);
                continue;
            }
            std::vector<double> all(size_t(maxRec + 1) * v.valuesPerRecord, kNaN);
            readRecords(v, v.vxrHead, all, 0);
            if (v.hasFill)
                for (double &x : all)
                    if (x == v.fill)
                        x = kNaN;
            const NumericColumn::Kind kind = (v.type == 31 || v.type == 32) ? NumericColumn::EpochMs
                                           : v.type == 33 ? NumericColumn::Tt2000Ns : NumericColumn::Number;
            for (int k = 0; k < v.valuesPerRecord; ++k) {
                NumericColumn c;
                c.name = v.valuesPerRecord == 1 ? v.name : QStringLiteral("%1[%2]").arg(v.name).arg(k);
                c.unit = v.unit;
                c.kind = kind;
                c.values.resize(size_t(maxRec) + 1);
                for (int r = 0; r <= maxRec; ++r)
                    c.values[r] = all[size_t(r) * v.valuesPerRecord + k];
                table_.columns.push_back(std::move(c));
            }
        }
        if (!skipped.isEmpty())
            table_.notes << QStringLiteral("未作为列的变量（非记录变量、字符型或记录数不同）：%1").arg(skipped.join(QStringLiteral("、")));
    }

    Bytes b_;
    NumericTable &table_;
    QByteArray unpacked_;
    bool bigEndian_ = false;
};

} // namespace cdf

// ============================================================ netCDF classic
namespace nc {

int typeSize(int t)
{
    switch (t) {
    case 1: case 2: case 7: return 1;
    case 3: case 8: return 2;
    case 4: case 5: case 9: return 4;
    case 6: case 10: case 11: return 8;
    default: return 0;
    }
}

double decode(const uchar *p, int t)
{
    switch (t) {
    case 1: return double(qint8(p[0]));
    case 7: return double(p[0]);
    case 3: return load<qint16>(p, true);
    case 8: return load<quint16>(p, true);
    case 4: return load<qint32>(p, true);
    case 9: return load<quint32>(p, true);
    case 5: return double(load<float>(p, true));
    case 6: return load<double>(p, true);
    case 10: return double(load<qint64>(p, true));
    case 11: return double(load<quint64>(p, true));
    default: return kNaN;
    }
}

struct Attribute
{
    int type = 0;
    std::vector<double> numbers;
    QString text;
};

struct Variable
{
    QString name;
    std::vector<int> dims;
    QHash<QString, Attribute> atts;
    int type = 0;
    qint64 vsize = 0;
    qint64 begin = 0;
    bool record = false;
};

class Reader
{
public:
    Reader(const Bytes &b, NumericTable &t) : b_(b), table_(t) {}

    void read()
    {
        const uchar *m = b_.at(0, 4);
        if (m[0] != 'C' || m[1] != 'D' || m[2] != 'F' || (m[3] != 1 && m[3] != 2 && m[3] != 5))
            throw FormatError("不是 netCDF classic 文件");
        version_ = m[3];
        table_.format = QStringLiteral("netCDF classic（CDF-%1）").arg(version_);
        pos_ = 4;
        qint64 numrecs = nonNeg();
        // dimensions
        readTag(0x0A);
        const qint64 ndims = nonNeg();
        for (qint64 i = 0; i < ndims; ++i) {
            const QString name = name_();
            const qint64 len = nonNeg();
            dimNames_ << name;
            dimLens_.push_back(len);
            if (len == 0)
                recordDim_ = int(i);
        }
        readAttributes();   // global attributes are not used
        readTag(0x0B);
        const qint64 nvars = nonNeg();
        for (qint64 i = 0; i < nvars; ++i) {
            Variable v;
            v.name = name_();
            const qint64 nd = nonNeg();
            for (qint64 k = 0; k < nd; ++k) {
                const qint64 id = version_ == 5 ? b_.i64(adv(8)) : b_.i32(adv(4));
                if (id < 0 || id >= dimLens_.size())
                    throw FormatError("netCDF 维度编号错误");
                v.dims.push_back(int(id));
            }
            v.atts = readAttributes();
            v.type = b_.i32(adv(4));
            v.vsize = nonNeg();
            v.begin = version_ == 1 ? b_.u32(adv(4)) : b_.i64(adv(8));
            v.record = !v.dims.empty() && v.dims.front() == recordDim_;
            vars_.push_back(v);
        }
        // record size
        int nrec = 0;
        for (const Variable &v : vars_)
            if (v.record) {
                ++nrec;
                recSize_ += v.vsize;
            }
        if (nrec == 1)
            for (const Variable &v : vars_)
                if (v.record)
                    recSize_ = qint64(typeSize(v.type)) * slab(v);
        if (numrecs == (version_ == 5 ? -1 : qint64(0xFFFFFFFF))) {   // streaming: count from the file size
            qint64 first = std::numeric_limits<qint64>::max();
            for (const Variable &v : vars_)
                if (v.record)
                    first = qMin(first, v.begin);
            numrecs = recSize_ > 0 ? (b_.size() - first) / recSize_ : 0;
        }
        numrecs_ = numrecs;
        if (recordDim_ >= 0)
            dimLens_[recordDim_] = numrecs_;
        build();
    }

private:
    qint64 adv(qint64 n)
    {
        const qint64 p = pos_;
        b_.at(p, n);
        pos_ += n;
        return p;
    }
    qint64 nonNeg() { return version_ == 5 ? b_.i64(adv(8)) : qint64(b_.u32(adv(4))); }
    QString name_()
    {
        const qint64 n = nonNeg();
        const qint64 p = adv((n + 3) / 4 * 4);
        return QString::fromUtf8(reinterpret_cast<const char *>(b_.at(p, n)), int(n));
    }
    void readTag(quint32 expected)
    {
        const quint32 tag = b_.u32(adv(4));
        if (tag != expected && tag != 0)
            throw FormatError("netCDF 文件头损坏");
    }
    QHash<QString, Attribute> readAttributes()
    {
        QHash<QString, Attribute> atts;
        readTag(0x0C);
        const qint64 n = nonNeg();
        for (qint64 i = 0; i < n; ++i) {
            const QString name = name_();
            Attribute a;
            a.type = b_.i32(adv(4));
            const qint64 count = nonNeg();
            const int size = typeSize(a.type);
            if (size == 0)
                throw FormatError("netCDF 属性类型错误");
            const qint64 p = adv((count * size + 3) / 4 * 4);
            if (a.type == 2)
                a.text = QString::fromUtf8(reinterpret_cast<const char *>(b_.at(p, count)), int(count)).trimmed();
            else
                for (qint64 k = 0; k < count; ++k)
                    a.numbers.push_back(decode(b_.at(p + k * size, size), a.type));
            atts.insert(name, a);
        }
        return atts;
    }
    qint64 slab(const Variable &v) const   // values per record (record variables) or in total
    {
        qint64 n = 1;
        for (size_t i = v.record ? 1 : 0; i < v.dims.size(); ++i)
            n *= dimLens_[v.dims[i]];
        return n;
    }

    // All values of a variable, with _FillValue / missing_value -> NaN and scale_factor / add_offset applied.
    std::vector<double> values(const Variable &v) const
    {
        const int size = typeSize(v.type);
        const qint64 per = slab(v);
        const qint64 recs = v.record ? numrecs_ : 1;
        std::vector<double> out(size_t(per * recs));
        for (qint64 r = 0; r < recs; ++r) {
            const qint64 base = v.begin + (v.record ? r * recSize_ : 0);
            const uchar *p = b_.at(base, per * size);
            for (qint64 k = 0; k < per; ++k)
                out[size_t(r * per + k)] = decode(p + k * size, v.type);
        }
        std::vector<double> fills;
        for (const char *key : {"_FillValue", "missing_value"})
            if (v.atts.contains(QLatin1String(key)))
                for (double f : v.atts.value(QLatin1String(key)).numbers)
                    fills.push_back(f);
        double scale = 1, offset = 0;
        if (v.atts.contains(QStringLiteral("scale_factor")) && !v.atts.value(QStringLiteral("scale_factor")).numbers.empty())
            scale = v.atts.value(QStringLiteral("scale_factor")).numbers.front();
        if (v.atts.contains(QStringLiteral("add_offset")) && !v.atts.value(QStringLiteral("add_offset")).numbers.empty())
            offset = v.atts.value(QStringLiteral("add_offset")).numbers.front();
        for (double &x : out) {
            for (double f : fills)
                if (x == f || (std::fabs(f) > 1e30 && std::fabs(x) >= std::fabs(f) * 0.999999))
                    x = kNaN;
            x = x * scale + offset;
        }
        return out;
    }

    NumericColumn column(const Variable &v, std::vector<double> data) const
    {
        NumericColumn c;
        c.name = v.name;
        c.unit = v.atts.value(QStringLiteral("units")).text;
        c.values = std::move(data);
        return c;
    }

    const Variable *coordinate(int dim) const
    {
        for (const Variable &v : vars_)
            if (v.dims.size() == 1 && v.dims.front() == dim && v.name == dimNames_[dim] && v.type != 2)
                return &v;
        return nullptr;
    }

    void build()
    {
        // old GMT grid layout: z as a 1-D array, with x_range / y_range / dimension
        const Variable *gz = nullptr, *gx = nullptr, *gy = nullptr, *gd = nullptr;
        for (const Variable &v : vars_) {
            if (v.name == QLatin1String("z") && v.dims.size() == 1) gz = &v;
            if (v.name == QLatin1String("x_range")) gx = &v;
            if (v.name == QLatin1String("y_range")) gy = &v;
            if (v.name == QLatin1String("dimension")) gd = &v;
        }
        if (gz && gx && gy && gd) {
            const std::vector<double> xr = values(*gx), yr = values(*gy), dim = values(*gd), z = values(*gz);
            const qint64 nx = qint64(dim.at(0)), ny = qint64(dim.at(1));
            if (nx * ny != qint64(z.size()) || nx < 1 || ny < 1)
                throw FormatError("GMT 网格尺寸不符");
            const bool pixel = gz->atts.value(QStringLiteral("node_offset")).numbers.empty() ? false
                             : gz->atts.value(QStringLiteral("node_offset")).numbers.front() != 0;
            const double dx = pixel ? (xr[1] - xr[0]) / nx : (nx > 1 ? (xr[1] - xr[0]) / (nx - 1) : 0);
            const double dy = pixel ? (yr[1] - yr[0]) / ny : (ny > 1 ? (yr[1] - yr[0]) / (ny - 1) : 0);
            NumericColumn cx, cy;
            cx.name = QStringLiteral("x");
            cy.name = QStringLiteral("y");
            for (qint64 j = 0; j < ny; ++j)   // rows start at the top
                for (qint64 i = 0; i < nx; ++i) {
                    cx.values.push_back(xr[0] + (i + (pixel ? 0.5 : 0)) * dx);
                    cy.values.push_back(yr[1] - (j + (pixel ? 0.5 : 0)) * dy);
                }
            table_.columns.push_back(std::move(cx));
            table_.columns.push_back(std::move(cy));
            table_.columns.push_back(column(*gz, z));
            table_.notes << QStringLiteral("旧版 GMT 网格（%1 × %2）").arg(nx).arg(ny);
            return;
        }

        // 2-D grids z(y, x): the pair of dimensions with the most cells
        const Variable *grid = nullptr;
        qint64 gridCells = 0;
        for (const Variable &v : vars_) {
            if (v.dims.size() != 2 || v.type == 2 || v.record)
                continue;
            const qint64 cells = dimLens_[v.dims[0]] * dimLens_[v.dims[1]];
            if (cells > gridCells) {
                grid = &v;
                gridCells = cells;
            }
        }
        if (grid) {
            const int dy = grid->dims[0], dx = grid->dims[1];
            const qint64 ny = dimLens_[dy], nx = dimLens_[dx];
            const Variable *cx = coordinate(dx), *cy = coordinate(dy);
            const std::vector<double> xs = cx ? values(*cx) : std::vector<double>(), ys = cy ? values(*cy) : std::vector<double>();
            NumericColumn colX, colY;
            colX.name = cx ? cx->name : dimNames_[dx];
            colY.name = cy ? cy->name : dimNames_[dy];
            if (cx) colX.unit = cx->atts.value(QStringLiteral("units")).text;
            if (cy) colY.unit = cy->atts.value(QStringLiteral("units")).text;
            for (qint64 j = 0; j < ny; ++j)
                for (qint64 i = 0; i < nx; ++i) {
                    colX.values.push_back(cx ? xs[size_t(i)] : double(i));
                    colY.values.push_back(cy ? ys[size_t(j)] : double(j));
                }
            table_.columns.push_back(std::move(colX));
            table_.columns.push_back(std::move(colY));
            QStringList used;
            for (const Variable &v : vars_)
                if (v.dims == grid->dims && v.type != 2) {
                    table_.columns.push_back(column(v, values(v)));
                    used << v.name;
                }
            table_.notes << QStringLiteral("二维网格 %1（%2 × %3），按行展开为 x y 值").arg(used.join(QStringLiteral("、"))).arg(nx).arg(ny);
            if (!cx || !cy)
                table_.notes << QStringLiteral("缺少坐标变量的维度用序号代替");
            return;
        }

        // 1-D variables along the longest dimension
        QMap<int, QStringList> byDim;
        for (const Variable &v : vars_)
            if (v.dims.size() == 1 && v.type != 2)
                byDim[v.dims.front()] << v.name;
        int best = -1;
        for (auto it = byDim.begin(); it != byDim.end(); ++it)
            if (best < 0 || dimLens_[it.key()] > dimLens_[best])
                best = it.key();
        if (best < 0 || dimLens_[best] == 0)
            throw FormatError("文件中没有可导入的一维或二维数值变量");
        for (const Variable &v : vars_)
            if (v.dims.size() == 1 && v.dims.front() == best && v.type != 2)
                table_.columns.push_back(column(v, values(v)));
        table_.notes << QStringLiteral("沿维度 %1（%2 个）的一维变量作为列").arg(dimNames_[best]).arg(dimLens_[best]);
    }

    Bytes b_;
    NumericTable &table_;
    int version_ = 1;
    qint64 pos_ = 0;
    QStringList dimNames_;
    QVector<qint64> dimLens_;
    int recordDim_ = -1;
    qint64 numrecs_ = 0;
    qint64 recSize_ = 0;
    std::vector<Variable> vars_;
};

} // namespace nc

template <typename Fn> NumericTable guarded(const QString &path, Fn fn)
{
    NumericTable t;
    try {
        FileBytes file(path);
        fn(file.bytes(), t);
    } catch (const FormatError &e) {
        t = NumericTable();
        t.error = QString::fromUtf8(e.what());
    } catch (const std::bad_alloc &) {
        t = NumericTable();
        t.error = QStringLiteral("内存不足，文件过大");
    } catch (const std::exception &e) {
        t = NumericTable();
        t.error = QStringLiteral("读取出错：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    return t;
}

} // namespace

BinaryKind binaryKind(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return BinaryKind::None;
    const QByteArray h = f.read(8);
    if (h.size() >= 4) {
        const uchar *p = reinterpret_cast<const uchar *>(h.constData());
        if (p[0] == 0xCD && p[1] == 0xF3 && p[2] == 0x00 && p[3] == 0x01)
            return BinaryKind::NasaCdf;
        if ((p[0] == 0xCD && p[1] == 0xF2) || (p[0] == 0 && p[1] == 0 && p[2] == 0xFF && p[3] == 0xFF))
            return BinaryKind::NasaCdf;   // CDF 2.x, reported by the reader
        if (p[0] == 'C' && p[1] == 'D' && p[2] == 'F' && (p[3] == 1 || p[3] == 2 || p[3] == 5))
            return BinaryKind::NetCdf;
        if (h.startsWith("\x89HDF"))
            return BinaryKind::Hdf5;
    }
    return BinaryKind::None;
}

NumericTable readNasaCdf(const QString &path)
{
    return guarded(path, [](const Bytes &b, NumericTable &t) { cdf::Reader(b, t).read(); });
}

NumericTable readNetCdf(const QString &path)
{
    return guarded(path, [](const Bytes &b, NumericTable &t) { nc::Reader(b, t).read(); });
}

QString formatValue(const NumericColumn &column, double value)
{
    if (!std::isfinite(value))
        return QString();
    if (column.kind == NumericColumn::EpochMs) {
        const qint64 unixMs = qint64(std::llround(value)) - 62167219200000LL;   // 0000-01-01 -> 1970-01-01
        return QDateTime::fromMSecsSinceEpoch(unixMs, Qt::UTC).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    }
    if (column.kind == NumericColumn::Tt2000Ns) {
        // TT2000: ns since 2000-01-01 12:00:00 TT; UTC = TT - 32.184 s - leap seconds (5 after 2000 up to 2017)
        double s = value * 1e-9 + 946727935.816;
        static const double leaps[] = {1136073600, 1230768000, 1341100800, 1435708800, 1483228800};
        for (double l : leaps)
            if (s - 1 >= l)
                s -= 1;
        return QDateTime::fromMSecsSinceEpoch(qint64(std::llround(s * 1000)), Qt::UTC).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    }
    return QString::number(value, 'g', 12);
}

} // namespace DataIO
