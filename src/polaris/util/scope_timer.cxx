#include "scope_timer.hxx"

#include <GLFW/glfw3.h>

std::unordered_map<const char*, double> global_timers;

scope_timer::scope_timer(double& elapsed_output, bool add)
    : elapsed_output(elapsed_output), add(add)
{
    start_time = std::chrono::steady_clock::now();
}

scope_timer::scope_timer(std::unordered_map<const char*, double>& map, const char* name, bool add)
// I've never seen the "," operator used in the wild, so I think this warrants
// an explanation. "," does nothing but has the lowest operator precedence, so
// the last part after the "," is the result of the expression.
// So this initializer runs the map insertion, then assigns the reference from ".at()".
// - torph
    : elapsed_output((map.insert({name, 0}), map.at(name))), add(add)
{
    start_time = std::chrono::steady_clock::now();
}

scope_timer::scope_timer(const char* name, bool add)
    : elapsed_output((global_timers.insert({name, 0}),
      global_timers.at(name))), add(add)
{
    start_time = std::chrono::steady_clock::now();
}

scope_timer::~scope_timer() {
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration<double, std::milli>(end - start_time);
    if (add) {
        elapsed_output += duration.count();
    } else {
        elapsed_output = duration.count();
    }
}
