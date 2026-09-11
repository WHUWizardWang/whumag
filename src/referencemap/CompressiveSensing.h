#ifndef COMPRESSIVESENSING_H
#define COMPRESSIVESENSING_H
#include <eigen-3.4.0/Eigen/Dense>
#include <random>
#include <set>
#include "omp.h"
#include "DataStruct.h"

class CompressiveSensing {
public:
    CompressiveSensing() = default;

    struct ReconstructionResult {
        std::vector<double> x;
        std::vector<double> y;
        std::vector<double> reconstructed_signal;
        double rms_error;
    };

    ReconstructionResult reconstruct(const Geomagnetic::Datapoint& data,
                                     int n_nonzero_coefs,
                                     int sampling_factor);

private:
    Eigen::MatrixXd createDctDictionary(int size);
    // Picks which indices are treated as "measured" -- unlike the old
    // applySampling(), the caller keeps this index set instead of it being
    // discarded, since the reconstruction step must know exactly which
    // positions are real measurements vs. unknown/to-be-filled.
    std::vector<int> selectSampledIndices(int size, int sampling_factor);
    // measured_data/sampled_indices are parallel arrays: measured_data(k) is
    // the true value at position sampled_indices[k]. Every unmeasured
    // position is treated as genuinely unknown, never as zero.
    Eigen::VectorXd sparseReconstruction(const Eigen::VectorXd& measured_data,
                                         const Eigen::MatrixXd& dict,
                                         const std::vector<int>& sampled_indices,
                                         int n_nonzero_coefs);
    double calculateRms(const Eigen::VectorXd& original,
                        const Eigen::VectorXd& reconstructed);
};
#endif // COMPRESSIVESENSING_H
