#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <Eigen/Dense> // Eigen for matrix calculations
#include <span>        // C++20 feature for more flexible data passing

class TimeSeriesAnalysis {
public:
    // Constructor and destructor
    explicit TimeSeriesAnalysis() = default;
    ~TimeSeriesAnalysis() = default;

    // Function to calculate the Hurst Exponent
    double calculateHurstExponent(std::span<const double> data) const;

    // Function to calculate the variance ratio
    double varianceRatioTest(std::span<const double> data, int lag) const;

    // Function to calculate the autocorrelation at lag 'lag'
    double calculateAutocorrelation(std::span<const double> data, int lag) const;

// private:
    // Private members if needed in the future
};
