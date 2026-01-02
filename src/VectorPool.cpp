#include "VectorPool.h"

// Define thread_local static members
thread_local std::stack<vector<pair<Action, double>>*> VectorPool::action_pool;
thread_local std::stack<vector<double>*> VectorPool::delta_pool;
thread_local std::stack<vector<double>*> VectorPool::probs_pool;

