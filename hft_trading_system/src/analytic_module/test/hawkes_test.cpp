// #include <vector>
// #include <cmath>
// #include <chrono>
// #include <iostream>
// #include "../hawkes_process.h"
// int main() {
//     // Example event times (in seconds)
//     std::vector<double> event_times = {0.5, 1.2, 1.8, 3.0, 4.5, 5.1, 5.9, 7.0, 8.2, 9.0};

//     // Initialize Hawkes process with initial parameter guesses
//     HawkesProcess hawkes(0.1, 0.5, 1.0);

//     // Measure the start time
//     auto start = std::chrono::high_resolution_clock::now();

//     // Fit the model to the data
//     if (!hawkes.fit(event_times)) {
//         std::cerr << "Failed to fit Hawkes process." << std::endl;
//         return 1;
//     }

//     // Measure the end time
//     auto end = std::chrono::high_resolution_clock::now();

//     // Calculate the duration
//     std::chrono::duration<double> duration = end - start;

//     // Output the duration
//     std::cout << "Time taken to fit Hawkes process: " << duration.count() << " seconds" << std::endl;

//     // Output estimated parameters
//     std::cout << "Estimated mu: " << hawkes.getMu() << std::endl;
//     std::cout << "Estimated alpha: " << hawkes.getAlpha() << std::endl;
//     std::cout << "Estimated beta: " << hawkes.getBeta() << std::endl;

//     // Simulate future events over the next 10 seconds
//     std::vector<double> simulated_events = hawkes.simulate(10.0);

//     // std::cout << "Simulated events:" << std::endl;
//     // for (double t : simulated_events) {
//     //     std::cout << t << std::endl;
//     // }

//     // Compute intensity over time
//     std::vector<double> times;
//     for (double t = 0.0; t <= 10.0; t += 0.1) {
//         times.push_back(t);
//     }
//     // start = std::chrono::high_resolution_clock::now();

//     std::vector<double> intensities = hawkes.intensity(times, event_times);
//     // Output intensities
//     // for (size_t i = 0; i < times.size(); ++i) {
//     //     std::cout << "Time: " << times[i] << ", Intensity: " << intensities[i] << std::endl;
//     // }
//     // end = std::chrono::high_resolution_clock::now();
//     // duration = end - start;
//     // std::cout << "Time taken to run intensity Hawkes process: " << duration.count() << " seconds" << std::endl;


//     return 0;
// }
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include "../hawkes_process.h"
#include <cmath>
// Function to simulate collecting event times from your data source
std::vector<double> collectEventTimes(double window_duration_seconds) {
    // Placeholder implementation: In practice, replace this with actual data retrieval
    // Simulate event times over the past window_duration_seconds
    std::vector<double> event_times;

    // Current time in seconds since UNIX epoch
    double current_time = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Generate synthetic event times for demonstration purposes
    double start_time = current_time - window_duration_seconds;
    double t = start_time;
    double average_interval = 1.0; // Average time between events in seconds

    while (t < current_time) {
        t += -std::log(((double) rand() / RAND_MAX)) * average_interval;
        if (t < current_time) {
            event_times.push_back(t);
        }
    }

    return event_times;
}

int main() {
    // Set the window duration for collecting event times (e.g., last 60 minutes)
    double window_duration_seconds = 60 * 60; // 60 minutes

    // Set the frequency for updating the Hawkes process model (e.g., every 15 minutes)
    double update_interval_seconds = 15 * 1; // 15 minutes

    // Initialize the Hawkes process with initial parameter guesses
    HawkesProcess hawkes(0.1, 0.5, 1.0);

    // Main loop for periodic updating
    while (true) {
        // 1. Data Collection
        std::vector<double> event_times = collectEventTimes(window_duration_seconds);

        // Check if we have enough events to proceed
        if (event_times.size() < 10) {
            std::cerr << "Not enough events collected. Skipping update." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds((int)update_interval_seconds));
            continue;
        }

        // 2. Data Preparation
        // Sort event times (if not already sorted)
        std::sort(event_times.begin(), event_times.end());

        // Convert timestamps to relative times starting from zero
        double t0 = event_times.front();
        for (auto& t : event_times) {
            t -= t0;
        }

        // 3. Parameter Fitting
        if (!hawkes.fit(event_times)) {
            std::cerr << "Failed to fit Hawkes process. Using previous parameters." << std::endl;
        } else {
            // Output estimated parameters
            std::cout << "Estimated parameters at " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
            std::cout << "mu: " << hawkes.getMu() << ", alpha: " << hawkes.getAlpha() << ", beta: " << hawkes.getBeta() << std::endl;
        }

        // 4. Use the Fitted Model
        // For example, compute the intensity at the current time
        std::vector<double> times = { event_times.back() };
        std::vector<double> intensities = hawkes.intensity(times, event_times);

        std::cout << "Current intensity: " << intensities.front() << std::endl;

        // Integrate the intensity into your trading strategy or risk management here

        // 5. Wait until the next update
        std::this_thread::sleep_for(std::chrono::seconds((int)update_interval_seconds));
    }

    return 0;
}
