// hawkes_process.cpp

#include "hawkes_process.h"
#include <cmath>
#include <iostream>
#include <algorithm>

// Include optimization library (e.g., NLopt)
#include <nlopt.hpp>

HawkesProcess::HawkesProcess(double mu, double alpha, double beta)
    : mu_(mu), alpha_(alpha), beta_(beta) {
}

HawkesProcess::~HawkesProcess() {
}

double HawkesProcess::getMu() const {
    return mu_;
}

double HawkesProcess::getAlpha() const {
    return alpha_;
}

double HawkesProcess::getBeta() const {
    return beta_;
}

void HawkesProcess::setMu(double mu) {
    mu_ = mu;
}

void HawkesProcess::setAlpha(double alpha) {
    alpha_ = alpha;
}

void HawkesProcess::setBeta(double beta) {
    beta_ = beta;
}

bool HawkesProcess::fit(std::vector<double>& event_times) {
    // Initial parameter guesses
    std::vector<double> params = {mu_, alpha_, beta_};

    // Set lower and upper bounds
    std::vector<double> lb = {1e-6, 1e-6, 1e-6}; // Parameters must be positive
    std::vector<double> ub = {1e6, 1.0, 1e6};    // Adjust as needed

    // Create NLopt optimizer
    nlopt::opt opt(nlopt::LN_BOBYQA, 3); // Using BOBYQA algorithm

    opt.set_lower_bounds(lb);
    opt.set_upper_bounds(ub);

    // Set objective function
    opt.set_min_objective(
        [](const std::vector<double>& x, std::vector<double>& grad, void* data) -> double {
            HawkesProcess* hp = reinterpret_cast<HawkesProcess*>(data);
            return hp->negLogLikelihood(x, *(hp->event_times_));
        },
        this);

    // Set stopping criteria
    opt.set_xtol_rel(1e-6);
    opt.set_maxeval(1000);

    // Store event times for use in the objective function
    this->event_times_ = &event_times;

    double minf;
    nlopt::result result = nlopt::result::FAILURE;
    try {
        result = opt.optimize(params, minf);
    } catch (std::exception& e) {
        std::cerr << "NLopt failed: " << e.what() << std::endl;
        return false;
    }

    if (result < 0) {
        std::cerr << "NLopt failed with code " << result << std::endl;
        return false;
    }

    // Update parameters
    mu_ = params[0];
    alpha_ = params[1];
    beta_ = params[2];

    return true;
}

double HawkesProcess::negLogLikelihood(const std::vector<double>& params, const std::vector<double>& event_times) {
    double mu = params[0];
    double alpha = params[1];
    double beta = params[2];

    size_t n = event_times.size();
    if (n == 0) return 0.0;

    double log_likelihood = 0.0;
    std::vector<double> lambda(n, mu);

    // Compute the intensity at each event time
    for (size_t i = 1; i < n; ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < i; ++j) {
            sum += alpha * std::exp(-beta * (event_times[i] - event_times[j]));
        }
        lambda[i] += sum;
    }

    // Compute the log-likelihood
    double integral = mu * event_times.back();
    for (size_t i = 0; i < n; ++i) {
        log_likelihood += std::log(lambda[i]);
        if (i > 0) {
            double delta_t = event_times[i] - event_times[i - 1];
            integral += (alpha / beta) * (1.0 - std::exp(-beta * delta_t));
        }
    }

    log_likelihood -= integral;

    // Return negative log-likelihood
    return -log_likelihood;
}

std::vector<double> HawkesProcess::simulate(double T) {
    std::vector<double> simulated_events;
    double t = 0.0;
    double lambda = mu_;
    double s = 0.0;

    while (t < T) {
        // Generate next event time
        double u = ((double) rand() / (RAND_MAX));
        double w = -std::log(u) / lambda;
        t += w;

        // Compute the conditional intensity at time t
        double lambda_t = mu_;
        for (double ti : simulated_events) {
            if (ti < t) {
                lambda_t += alpha_ * std::exp(-beta_ * (t - ti));
            } else {
                break;
            }
        }

        // Accept or reject the event
        double D = ((double) rand() / (RAND_MAX));
        if (D * lambda <= lambda_t) {
            simulated_events.push_back(t);
            lambda = lambda_t;
        } else {
            lambda = lambda_t;
        }
    }

    return simulated_events;
}

std::vector<double> HawkesProcess::intensity(const std::vector<double>& times, const std::vector<double>& event_times) {
    std::vector<double> intensities(times.size(), mu_);

    for (size_t i = 0; i < times.size(); ++i) {
        double t = times[i];
        double sum = 0.0;
        for (double s : event_times) {
            if (s < t) {
                sum += alpha_ * std::exp(-beta_ * (t - s));
            } else {
                break;
            }
        }
        intensities[i] += sum;
    }

    return intensities;
}
