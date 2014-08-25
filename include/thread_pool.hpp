#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <list>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>

#include "schedulers.hpp"

template <typename R>
bool is_ready(std::future<R>& f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template <class Task, class SchedulingPolicy>
class thread_pool {
 public:
  using task_type = Task;
  using scheduler_type = SchedulingPolicy<Task>;
  using pool_type = thread_pool<Task, SchedulingPolicy>;

  template <typename T_>
  using decay_t = typename std::decay<T_>::type;
  template <class T>
  using result_of_t = typename std::result_of<T>::type;

 public:
  thread_pool(size_t pool_size) {}

  std::future<Task::return_type> schedule_task(Task&& task) {}

template <typename F, typename A>
std::future<result_of_t<decay_t<F>(decay_t<A>)>> spawn_task(F &&f, A &&a) {
    using result_t = result_of_t<decay_t<F>(decay_t<A>)>;
    std::packaged_task< result_t(decay_t<A>)> task(std::forward<F>(f));
    auto res = task.get_future();
    std::thread t(std::move(task), std::forward<A>(a));
    t.detach();
    return res;
}

 private:
  std::atomic<int> active_;
  std::atomic<int> pending_;
  std::list<std::thread> workers_;
  scheduler_type scheduler_;
};

#endif
