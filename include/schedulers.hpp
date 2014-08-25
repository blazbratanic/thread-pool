#ifndef TaskHREADED_CONTaskAINER_HPP
#define TaskHREADED_CONTaskAINER_HPP

#include <vector>
#include <deque>
#include <priority_queue>

template <class Task>
class lifo_scheduler {
 public:
  inline void push(Task const& item) { container.push_back(item); }
  inline void top(Task& item) { container.front(item); }
  inline bool pop() { container.pop_front(); }
  inline bool empty() { return container.empty(); }
  inline bool size() { return container.size(); }

 private:
  std::deque<Task> container_;
};

template <class Task>
class fifo_scheduler {
 public:
  inline void push(Task const& item) { container.push_back(item); }
  inline void top(Task& item) { container.back(item); }
  inline bool pop() { container.pop_back(); }
  inline bool empty() { return container.empty(); }
  inline bool size() { return container.size(); }

 private:
  std::deque<Task> container_;
};

template <class Task>
class priority_scheduler {
 public:
  inline void push(Task const& item) { container.push(item); }
  inline void top(Task& item) { container.top(item); }
  inline bool pop() { container.pop(); }
  inline bool empty() { return container.empty(); }
  inline bool size() { return container.size(); }

 private:
  std::priority_queue<Task> container_;
};

#endif
