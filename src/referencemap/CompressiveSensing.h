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
    Eigen::VectorXd applySampling(const Eigen::VectorXd& data, int sampling_factor);
    Eigen::VectorXd sparseReconstruction(const Eigen::VectorXd& data,
                                         const Eigen::MatrixXd& dict,
                                         int n_nonzero_coefs);
    double calculateRms(const Eigen::VectorXd& original,
                        const Eigen::VectorXd& reconstructed);
};
#endif // COMPRESSIVESENSING_H
