#ifndef SCHEDULERS_HPP
#define SCHEDULERS_HPP

#include <vector>
#include <deque>
#include <queue>

template <class Task>
class fifo_scheduler {
 public:
  inline void push(Task&& item) {
    container_.push_back(std::forward<Task>(item));
  }
  inline void top(Task& item) { item = std::move(container_.front()); }
  inline void pop() { container_.pop_front(); }
  inline bool empty() { return container_.empty(); }
  inline bool size() { return container_.size(); }

 private:
  std::deque<Task> container_;
};

template <class Task>
class lifo_scheduler {
 public:
  inline void push(Task&& item) {
    container_.push_back(std::forward<Task>(item));
  }
  inline void top(Task& item) { item = std::move(container_.back()); }
  inline void pop() { container_.pop_back(); }
  inline bool empty() { return container_.empty(); }
  inline bool size() { return container_.size(); }

 private:
  std::deque<Task> container_;
};

namespace detail {
template <class Task>
class priority_comparator {
 public:
  bool operator()(Task const& a, Task const& b) {
    return std::get<0>(a) < std::get<0>(b) ||
           (std::get<0>(a) == std::get<0>(b) &&
            std::get<1>(a) < std::get<1>(b));
  }
};
}

template <class Task>
class priority_scheduler {
 public:
  inline void push(Task&& item) {
    container_.push_back(std::forward<Task>(item));
    std::push_heap(container_.begin(), container_.end(),
                   detail::priority_comparator<Task>());
  }
  inline void top(Task& item) {
    std::pop_heap(container_.begin(), container_.end(),
                  detail::priority_comparator<Task>());
    item = std::move(container_.back());
  }
  inline void pop() { container_.pop_back(); }
  inline bool empty() { return container_.empty(); }
  inline bool size() { return container_.size(); }

 private:
  std::vector<Task> container_;
};

#endif
