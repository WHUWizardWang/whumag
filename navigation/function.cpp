#include "function.h"

QVector<QPointF> readPointsFromFile(const QString &fileName) {
    QVector<QPointF> points;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "打开文件失败：" << fileName;
        return points;
    }

    // 支持空白、逗号、分号、制表符做分隔
    static const QRegularExpression sepRe(R"([,\s;]+)");
    QTextStream in(&file);
    int lineNo = 0;

    while (!in.atEnd()) {
        ++lineNo;
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        // 跳过注释
        if (line.startsWith('#') || line.startsWith("//"))
            continue;

        // 分割并过滤空字段
        QStringList fields = line.split(sepRe, Qt::SkipEmptyParts);
        if (fields.size() < 2) {
            qWarning() << "第" << lineNo << "行格式错误（少于2个字段）:" << line;
            continue;
        }

        bool ok1 = false, ok2 = false;
        double x = fields.at(0).toDouble(&ok1);
        double y = fields.at(1).toDouble(&ok2);
        if (!ok1 || !ok2) {
            qWarning() << "第" << lineNo << "行数值转换失败:" << line;
            continue;
        }

        points.append(QPointF(x, y));
    }

    file.close();
    return points;
}
