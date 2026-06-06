#include "platform/Time.h"

#include <chrono>

double GetSeconds()
{
    using Clock = std::chrono::steady_clock;

    static const Clock::time_point start = Clock::now();

    const std::chrono::duration<double> elapsed = Clock::now() - start;
    return elapsed.count();
}
