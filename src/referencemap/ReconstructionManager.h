#ifndef RECONSTRUCTIONMANAGER_H
#define RECONSTRUCTIONMANAGER_H
#include "CompressiveSensing.h"
#include <QString>

class ReconstructionManager {
public:
    ReconstructionManager() = default;

    bool processData(const QString& input_file,
                     int n_nonzero_coefs,
                     int sampling_factor,
                     const QString& output_dir,
                     const QString& output_filename);
    double RMS;
private:
    bool saveResults(const CompressiveSensing::ReconstructionResult& result,
                     const QString& output_dir,
                     const QString& output_filename);

    CompressiveSensing cs;
};
#endif // RECONSTRUCTIONMANAGER_H
