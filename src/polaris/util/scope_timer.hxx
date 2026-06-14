#pragma once
#include <unordered_map>
#include <chrono>

typedef std::unordered_map<const char*, double> timer_map_t;
extern timer_map_t global_timers;

// A timer that measures the runtime of a scope. The timer starts on declaration,
// and ends when destroyed.
class scope_timer {
    double& elapsed_output;
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    bool add;

public:
    // Elapsed time is written to the specified location on destroy
    scope_timer(double& elapsed_output, bool add = false);

    /// @brief Elapsed time is saved into the map with the specified key on destroy.
    ///
    /// If the specified key doesn't exist, it'll be inserted automatically.
    scope_timer(std::unordered_map<const char*, double>& map, const char* name, bool add = false);

    scope_timer(const char* name, bool add = false);
    ~scope_timer(); // Ends timer
};
