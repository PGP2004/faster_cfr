#include "Profiler.h"

// Define the thread-local call stack
thread_local std::vector<Profiler::ScopedTimer*> Profiler::call_stack;

