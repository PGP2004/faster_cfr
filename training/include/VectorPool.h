#pragma once
#include <vector>
#include <stack>
#include "Utils.h"
using std::vector;
using std::pair;
using std::stack;


// Object pool for vector buffers to avoid allocations in hot path
class VectorPool {
private:

    static stack<vector<pair<Action, double>>*> action_pool;
    static stack<vector<double>*> delta_pool;
    static stack<vector<double>*> probs_pool;
    static stack<ChanceUndo*> chance_undo_pool;
    static stack<ActionUndo*> action_undo_pool;

public:
   
    struct ActionBuffer {
        vector<pair<Action, double>>* buf;
        ActionBuffer() {
            if (VectorPool::action_pool.empty()) {
                throw std::logic_error("ran outa actions");
            } else {
                buf = VectorPool::action_pool.top();
                VectorPool::action_pool.pop();
            }
            buf->clear();
        }

        ~ActionBuffer() {
            if (buf) {
                VectorPool::action_pool.push(buf);
            }
        }

        ActionBuffer& operator=(const ActionBuffer&) = delete;
        vector<pair<Action, double>>& get() { return *buf; }
    };

    struct DeltaBuffer {
        vector<double>* buf;
        DeltaBuffer() {
            if (VectorPool::delta_pool.empty()) {
                throw std::logic_error("ran outa deltas");
            } else {
                buf = VectorPool::delta_pool.top();
                VectorPool::delta_pool.pop();
            }
            buf->clear();
        }
        
        ~DeltaBuffer() {
            if (buf) {
                VectorPool::delta_pool.push(buf);
            }
        }

        DeltaBuffer(const DeltaBuffer&) = delete;
        DeltaBuffer& operator=(const DeltaBuffer&) = delete;
        vector<double>& get() {return *buf; }
    };

    struct ProbsBuffer {
        vector<double>* buf;
        ProbsBuffer() {
            if (VectorPool::probs_pool.empty()) {
                throw std::logic_error("ran outa probs");
            } else {
                buf = VectorPool::probs_pool.top();
                VectorPool::probs_pool.pop();
            }
            buf->clear();
        }
        ~ProbsBuffer() {
            if (buf) {
                VectorPool::probs_pool.push(buf);
            }
        }

        ProbsBuffer(const ProbsBuffer&) = delete;
        ProbsBuffer& operator=(const ProbsBuffer&) = delete;
        vector<double>& get() { return *buf; }
    };

    static void preallocate(size_t count = 100) {
        for (size_t i = 0; i < count; i++) {
            auto* action_buf = new vector<pair<Action, double>>();
            action_buf->reserve(6);
            action_pool.push(action_buf);

            auto* delta_buf = new vector<double>();
            delta_buf->reserve(6);
            delta_pool.push(delta_buf);

            auto* probs_buf = new vector<double>();
            probs_buf->reserve(6);
            probs_pool.push(probs_buf);
        }
    }
};