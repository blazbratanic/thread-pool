#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <list>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>
#include <exception>
#include <type_traits>
#include <iostream>

#include "schedulers.hpp"
#include "worker_thread.hpp"

template <typename R>
bool is_ready(std::future<R>& f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template <class Task, template <class> class SchedulingPolicy = fifo_scheduler>
class thread_pool {
 public:
  using return_type = decltype(Task()());
  using task_type = std::tuple<int, int, std::promise<return_type>, Task>;
  using scheduler_type = SchedulingPolicy<task_type>;
  using pool_type = thread_pool<Task, SchedulingPolicy>;
  using worker_type = worker_thread<pool_type>;

 public:
  thread_pool(size_t pool_size) : counter_(0), target_workers_(pool_size) {
    for (size_t i = 0; i < pool_size; ++i) {
      auto worker = std::make_shared<worker_type>(this);
      workers_.emplace_back(std::thread(std::bind(&worker_type::run, worker)));
    }
  }

  std::future<return_type> schedule_task(Task&& task) {
    std::unique_lock<std::mutex> lk(mtx_);
    std::promise<return_type> result_promise;
    auto result_future = result_promise.get_future();

    scheduler_.push(std::make_tuple(0, ++counter_, std::move(result_promise),
                                    std::forward<Task>(task)));
    ++pending_tasks_;
    empty_condition_.notify_all();

    return result_future;
  }

  bool shutdown() {
    target_workers_ = 0;
    empty_condition_.notify_all();
    for (auto&& w : workers_) {
      w.join();
    }
    return true;
  }

  bool execute_task() {
    task_type task;
    {
      std::unique_lock<std::mutex> lk(mtx_);

      while (scheduler_.empty()) {
        if (workers_.size() > target_workers_) {
          return false;
        }

        empty_condition_.wait(lk);
      }

      scheduler_.top(task);
      --pending_tasks_;
      scheduler_.pop();
    }

    ++active_tasks_;
    try {
      std::get<2>(task).set_value(std::get<3>(task)());
      --active_tasks_;
    }
    catch (...) {
      std::get<2>(task).set_exception(std::current_exception());
      --active_tasks_;
    }
    return true;
  }

  bool empty() {
    // std::unique_lock<std::mutex> lk(mtx_);
    return scheduler_.size();
  }

 private:
  int counter_;
  std::atomic<int> active_tasks_;
  std::atomic<int> pending_tasks_;
  std::vector<std::thread> workers_;
  scheduler_type scheduler_;

  size_t target_workers_;

  std::mutex mtx_;
  std::condition_variable empty_condition_;
};

#endif
