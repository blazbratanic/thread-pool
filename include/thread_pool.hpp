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
  template <typename T_>
  using decay_t = typename std::decay<T_>::type;
  template <class T>
  using result_of_t = typename std::result_of<T>::type;

 public:
  using return_type = decay_t<result_of_<Task>>;
  using task_type = std::tuple<int, int, std::promise<return_type>, Task>;
  using scheduler_type = SchedulingPolicy<Task>;
  using pool_type = thread_pool<Task, SchedulingPolicy>;

 public:
  thread_pool(size_t pool_size) : counter_(0) {}

  std::future<Task::return_type> schedule_task(Task&& task) {
    std::unique_lock<std::mutex> lk(mtx_);
    std::promise<Task::return_type> result_promise;
    auto result_future = result_promise.get_future();

    scheduler_.push(std::make_tuple(0, ++counter_, std::move(result_promise),
                                    std::forward<Task>(task)));
    ++pending_tasks_;
    empty_condition_.notify_all();

    return result_future;
  }

  void execute_task() {
    task_type task;
    {
      std::unique_lock<std::mutex> lk(pool->mtx_);

      while (pool->scheduler_.empty()) {
        if (workers_.size() > pool->wanted_workers_) {
          return;
        }

        pool->empty_condition_.wait(lk);
      }
      auto task = scheduler_.top();
      --pending_tasks_;
      scheduler_.pop();
    }

    ++active_tasks_;
    std::get<3>(task).set_value(std::get<4>(task)());
    --active_tasks_;
  }

  bool empty() {
    std::unique_lock<std::mutex> lk(mtx_);
    return scheduler_.size();
  }

 private:
  int counter_;
  std::atomic<int> active_tasks_;
  std::atomic<int> pending_tasks_;
  std::vector<std::thread> workers_;
  scheduler_type scheduler_;

  int wanted_workers_;

  std::mutex mtx_;
  std::condition_variable empty_condition_;
};

#endif
