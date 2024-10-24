// hawkes_process.h

#ifndef HAWKES_PROCESS_H
#define HAWKES_PROCESS_H

#include <vector>
#include <functional>

class HawkesProcess {
public:
    HawkesProcess(double mu, double alpha, double beta);
    ~HawkesProcess();

    /**
     * @brief Fit the Hawkes process parameters to event data.
     * @param event_times Vector of event timestamps.
     * @return True if fitting is successful.
     */
    bool fit(std::vector<double>& event_times);

    /**
     * @brief Simulate events over a time horizon.
     * @param T Time horizon for simulation.
     * @return Vector of simulated event times.
     */
    std::vector<double> simulate(double T);

    /**
     * @brief Compute the intensity function at given times.
     * @param times Vector of times at which to compute intensity.
     * @param event_times Vector of historical event times.
     * @return Vector of intensity values.
     */
    // define event_times

    // Accessors
    double getMu() const;
    double getAlpha() const;
    double getBeta() const;

    // Setters (optional, if you allow manual parameter setting)
    void setMu(double mu);
    void setAlpha(double alpha);
    void setBeta(double beta);
    std::vector<double>* event_times_; // Pointer to event data
    //intensity function
    std::vector<double> intensity(const std::vector<double>& times, const std::vector<double>& event_times);
private:
    double mu_;     // Baseline intensity
    double alpha_;  // Excitation magnitude
    double beta_;   // Decay rate
    /**
     * @brief Log-likelihood function for parameter estimation.
     * @param params Vector of parameters [mu, alpha, beta].
     * @param event_times Vector of event timestamps.
     * @return Negative log-likelihood value.
     */
    double negLogLikelihood(const std::vector<double>& params, const std::vector<double>& event_times);
};

#endif // HAWKES_PROCESS_H
