#include "VectorPool.h"

using namespace std;

stack<vector<pair<Action, double>>*> VectorPool::action_pool;
stack<vector<double>*> VectorPool::delta_pool;
stack<vector<double>*> VectorPool::probs_pool;
