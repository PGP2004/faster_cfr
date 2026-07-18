#include "vector_pool.h"

using namespace std;
stack<vector<double>*> VectorPool::delta_pool;
stack<vector<double>*> VectorPool::probs_pool;
