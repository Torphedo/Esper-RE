#include "scope_timer.hxx"

#include <GLFW/glfw3.h>

scope_timer::scope_timer(double& elapsed_output) : elapsed_output(elapsed_output) {
    start_time = glfwGetTime();
}

scope_timer::scope_timer(std::unordered_map<const char*, double>& map, const char* name)
// I've never seen the "," operator used in the wild, so I think this warrants
// an explanation. "," does nothing but has the lowest operator precedence, so
// the last part after the "," is the result of the expression.
// So this initializer runs the map insertion, then assigns the reference from ".at()".
// - torph
    : elapsed_output((map.insert({name, 0}), map.at(name)))
{
    start_time = glfwGetTime();
}

scope_timer::~scope_timer() {
    elapsed_output = glfwGetTime() - start_time;
}
