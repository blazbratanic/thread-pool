#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <deque>
#include <priority_queue>
#include <list>
#include <thread>
#include <atomic>

#include "threaded_container.hpp"

template <class T>
class lifo_scheduler {
  typedef threaded_container<T, std::vector<T>> container_type;
};

template <class T>
class fifo_scheduler {
  typedef threaded_container<T, std::deque<T>> container_type;
};

template <class T>
class priority_scheduler {
  typedef threaded_container<T, std::priority_queue<T>> container_type;
};

class thread_pool {
  template <class Task, class SchedulingPolicy = fifo_sc
  thread_pool(size_t pool_size) {}

 public:
  using task_type = Task;
  using scheduler_type = SchedulingPolicy<Task>;

 private:
  std::atomic<int> active_;
  std::atomic<int> pending_;
  std::list<std::thread> workers_;
  scheduler_type scheduler_;
};

#endif
