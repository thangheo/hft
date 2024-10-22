#include <iostream>
#include <vector>
#include <cmath>
#include <numeric> // for std::accumulate
#include <algorithm> // for std::sort
#include <Eigen/Dense> // You can install Eigen for matrix calculations

using namespace std;

// Function to calculate the Hurst Exponent
double calculateHurstExponent(const vector<double>& data) {
    int N = data.size();
    vector<double> mean_adjusted_data(N);
    double mean = accumulate(data.begin(), data.end(), 0.0) / N;

    // Calculate mean-adjusted series
    for (int i = 0; i < N; ++i) {
        mean_adjusted_data[i] = data[i] - mean;
    }

    // Cumulative sum
    vector<double> cumulative_sum(N);
    partial_sum(mean_adjusted_data.begin(), mean_adjusted_data.end(), cumulative_sum.begin());

    // Range (R)
    double R = *max_element(cumulative_sum.begin(), cumulative_sum.end()) - *min_element(cumulative_sum.begin(), cumulative_sum.end());

    // Standard deviation (S)
    double S = sqrt(inner_product(mean_adjusted_data.begin(), mean_adjusted_data.end(), mean_adjusted_data.begin(), 0.0) / N);

    // Hurst exponent
    return log(R / S) / log(N);
}

// Function to calculate the variance ratio
double varianceRatioTest(const vector<double>& data, int lag) {
    int N = data.size();
    double mean = accumulate(data.begin(), data.end(), 0.0) / N;
    double variance = 0.0;
    double lag_variance = 0.0;

    // Calculate variance of differences (lag 1)
    for (int i = 1; i < N; ++i) {
        variance += pow(data[i] - data[i - 1] - mean, 2);
    }

    // Calculate variance of differences with a lag
    for (int i = lag; i < N; ++i) {
        lag_variance += pow(data[i] - data[i - lag] - mean, 2);
    }

    variance /= (N - 1);
    lag_variance /= (N - lag);

    return lag_variance / variance;
}

// Function to calculate the autocorrelation at lag 'lag'
double calculateAutocorrelation(const vector<double>& data, int lag) {
    int N = data.size();
    double mean = accumulate(data.begin(), data.end(), 0.0) / N;
    double numerator = 0.0;
    double denominator = 0.0;

    for (int i = 0; i < N - lag; ++i) {
        numerator += (data[i] - mean) * (data[i + lag] - mean);
    }

    for (int i = 0; i < N; ++i) {
        denominator += pow(data[i] - mean, 2);
    }

    return numerator / denominator;
}

int main() {
    // Example 1-minute price data
    vector<double> price_data = {101.5, 102.0, 101.7, 102.1, 101.8, 101.6, 102.3, 102.5, 102.4, 101.9};

    // Hurst Exponent
    double hurst = calculateHurstExponent(price_data);
    cout << "Hurst Exponent: " << hurst << endl;
    if (hurst < 0.5)
        cout << "Likely mean-reverting." << endl;
    else if (hurst > 0.5)
        cout << "Likely trending." << endl;
    else
        cout << "Likely random walk." << endl;

    // Variance Ratio Test
    double variance_ratio = varianceRatioTest(price_data, 2); // Test with lag of 2
    cout << "Variance Ratio: " << variance_ratio << endl;
    if (variance_ratio < 1)
        cout << "Likely mean-reverting." << endl;
    else if (variance_ratio > 1)
        cout << "Likely trending." << endl;
    else
        cout << "Likely random walk." << endl;

    // Autocorrelation Function for lag 1
    double autocorrelation = calculateAutocorrelation(price_data, 1);
    cout << "Autocorrelation at lag 1: " << autocorrelation << endl;

    return 0;
}
