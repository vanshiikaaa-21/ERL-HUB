// PriorityDispatchQueue.h  -  Unit IV: generic (template) priority queue for dispatch requests.
// Built on std::vector + the heap algorithms; the ordering is supplied by a functor.
#ifndef RELIEFOPS_PRIORITY_DISPATCH_QUEUE_H
#define RELIEFOPS_PRIORITY_DISPATCH_QUEUE_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

namespace ReliefOps {

/// `Compare(a, b)` must return true when `a` is LESS urgent than `b` (the most urgent item is served first).
template <typename T, typename Compare = std::less<T> >
class PriorityDispatchQueue {
public:
    explicit PriorityDispatchQueue(Compare cmp = Compare()) : cmp_(cmp) {}

    void push(const T& item) {
        heap_.push_back(item);
        std::push_heap(heap_.begin(), heap_.end(), cmp_);
    }

    /// Removes and returns the most urgent item.
    T pop() {
        if (heap_.empty()) throw std::out_of_range("PriorityDispatchQueue::pop on an empty queue");
        std::pop_heap(heap_.begin(), heap_.end(), cmp_);
        T item = heap_.back();
        heap_.pop_back();
        return item;
    }

    const T& top() const {
        if (heap_.empty()) throw std::out_of_range("PriorityDispatchQueue::top on an empty queue");
        return heap_.front();
    }

    bool empty() const { return heap_.empty(); }
    std::size_t size() const { return heap_.size(); }

private:
    std::vector<T> heap_;
    Compare cmp_;
};

}  // namespace ReliefOps

#endif  // RELIEFOPS_PRIORITY_DISPATCH_QUEUE_H
