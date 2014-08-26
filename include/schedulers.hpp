#ifndef SCHEDULERS_HPP
#define SCHEDULERS_HPP

#include <vector>
#include <deque>
#include <queue>

template <class Task>
class lifo_scheduler {
 public:
  inline void push(Task&& item) {
    container_.push_back(std::forward<Task>(item));
  }
  inline void top(Task& item) { container_.front(item); }
  inline void pop() { container_.pop_front(); }
  inline bool empty() { return container_.empty(); }
  inline bool size() { return container_.size(); }

 private:
  std::deque<Task> container_;
};

template <class Task>
class fifo_scheduler {
 public:
  inline void push(Task&& item) {
    container_.push_back(std::forward<Task>(item));
  }
  inline void top(Task& item) { container_.back(item); }
  inline void pop() { container_.pop_back(); }
  inline bool empty() { return container_.empty(); }
  inline bool size() { return container_.size(); }

 private:
  std::deque<Task> container_;
};

template <class Task>
class priority_scheduler {
 public:
  inline void push(Task&& item) { container_.push(std::forward<Task>(item)); }
  inline void top(Task& item) { container_.top(item); }
  inline void pop() { container_.pop(); }
  inline bool empty() { return container_.empty(); }
  inline bool size() { return container_.size(); }

 private:
  std::priority_queue<Task> container_;
};

#endif
