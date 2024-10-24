#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include "../hawkes_process.h"
int main() {
    // Example event times (in seconds)
    std::vector<double> event_times = {0.5, 1.2, 1.8, 3.0, 4.5, 5.1, 5.9, 7.0, 8.2, 9.0};

    // Initialize Hawkes process with initial parameter guesses
    HawkesProcess hawkes(0.1, 0.5, 1.0);

    // Measure the start time
    auto start = std::chrono::high_resolution_clock::now();

    // Fit the model to the data
    if (!hawkes.fit(event_times)) {
        std::cerr << "Failed to fit Hawkes process." << std::endl;
        return 1;
    }

    // Measure the end time
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    std::chrono::duration<double> duration = end - start;

    // Output the duration
    std::cout << "Time taken to fit Hawkes process: " << duration.count() << " seconds" << std::endl;

    // Output estimated parameters
    std::cout << "Estimated mu: " << hawkes.getMu() << std::endl;
    std::cout << "Estimated alpha: " << hawkes.getAlpha() << std::endl;
    std::cout << "Estimated beta: " << hawkes.getBeta() << std::endl;

    // Simulate future events over the next 10 seconds
    std::vector<double> simulated_events = hawkes.simulate(10.0);

    std::cout << "Simulated events:" << std::endl;
    for (double t : simulated_events) {
        std::cout << t << std::endl;
    }

    // Compute intensity over time
    std::vector<double> times;
    for (double t = 0.0; t <= 10.0; t += 0.1) {
        times.push_back(t);
    }
    std::vector<double> intensities = hawkes.intensity(times, event_times);

    // Output intensities
    for (size_t i = 0; i < times.size(); ++i) {
        std::cout << "Time: " << times[i] << ", Intensity: " << intensities[i] << std::endl;
    }

    return 0;
}