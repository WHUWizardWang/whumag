// ReconstructionManager.cpp
#include "ReconstructionManager.h"
#include "ReadData.h"
#include <QFile>
#include <QTextStream>
#include <QDir>

bool ReconstructionManager::processData(const QString& input_file,
                                        int n_nonzero_coefs,
                                        int sampling_factor,
                                        const QString& output_dir,
                                        const QString& output_filename)
{
    // 加载数据
    Geomagnetic::Datapoint data;
    // 实现数据加载逻辑...
    Geomagnetic::ReadData readData;
    if (!readData.readGridFromFile(input_file.toStdString(), data)) {
        return false;
    }
    // 执行重构
    auto result = cs.reconstruct(data, n_nonzero_coefs, sampling_factor);

    // 保存结果
    return saveResults(result, output_dir, output_filename);
}

bool ReconstructionManager::saveResults(const CompressiveSensing::ReconstructionResult& result,
                                        const QString& output_dir,
                                        const QString& output_filename)
{
    QDir().mkpath(output_dir);

    QString result_path = output_dir + "/" + output_filename;

    QFile result_file(result_path);

    if (!result_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream result_stream(&result_file);

    for (size_t i = 0; i < result.x.size(); ++i) {
        QString line = QString("%1 %2 %3\n")
        .arg(result.x[i])
            .arg(result.y[i])
            .arg(result.reconstructed_signal[i]);

        result_stream << line;
    }
    RMS = result.rms_error;
    qDebug() << "RMS Error:" << result.rms_error;
    return true;
}
