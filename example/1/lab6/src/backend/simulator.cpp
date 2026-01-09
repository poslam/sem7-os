#include "simulator.h"

#include <random>
#include <algorithm>
#include <cmath>

#include "common.h"

namespace lab5 {

void Simulator::start() {
    running_ = true;
    thread_ = std::thread([this]() { run(); });
}

void Simulator::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

bool Simulator::pop(Sample& out) {
    std::unique_lock<std::mutex> lk(mu_);
    cv_.wait(lk, [&] { return !queue_.empty() || !running_; });
    if (queue_.empty()) return false;
    out = queue_.front();
    queue_.pop();
    return true;
}

void Simulator::run() {
    using namespace std::chrono;

    std::mt19937 rng(static_cast<unsigned>(now_ms()));
    std::normal_distribution<double> weatherDrift(0.0, 0.02);
    std::normal_distribution<double> shortNoise(0.0, 0.15);

    double simHours = 0.0;
    double weatherOffset = 0.0;
    double noiseState = 0.0;
    constexpr double noiseRho = 0.95;

    constexpr double meanTemp       = 15.0;
    constexpr double dailyAmplitude = 7.0;
    constexpr double minTemp        = -20.0;
    constexpr double maxTemp        = 40.0;

    constexpr auto   step         = seconds(2);
    constexpr double simStepHours = 10.0 / 60.0;
    constexpr double twoPi        = 6.283185307179586;

    while (running_) {
        simHours += simStepHours;
        double hourOfDay  = std::fmod(simHours, 24.0);
        double dailyCycle = std::sin(twoPi * (hourOfDay - 15.0) / 24.0);

        weatherOffset += weatherDrift(rng);
        weatherOffset = std::clamp(weatherOffset, -5.0, 5.0);

        noiseState = noiseRho * noiseState + shortNoise(rng);

        double value = meanTemp
                     + dailyAmplitude * dailyCycle
                     + weatherOffset
                     + noiseState;

        value = std::clamp(value, minTemp, maxTemp);

        Sample s{Clock::now(), value};
        {
            std::lock_guard<std::mutex> lk(mu_);
            queue_.push(s);
        }
        cv_.notify_one();
        std::this_thread::sleep_for(step);
    }

    cv_.notify_all();
}

}  // namespace lab5
