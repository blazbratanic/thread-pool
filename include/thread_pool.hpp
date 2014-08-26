// Copyright (c) 2014, Blaz Bratanic
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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

  friend class worker_thread<pool_type>;

 public:
  thread_pool(size_t pool_size) : counter_(0), target_workers_(pool_size) {
    for (size_t i = 0; i < pool_size; ++i) {
      add_worker(mtx_);
    }
  }

  ~thread_pool() { shutdown(); }

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
    workers_.clear();
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

  bool resize(size_t target_workers) {
    std::unique_lock<std::mutex> lk(mtx_);

    if (target_workers < target_workers_) {
      target_workers_ = target_workers;
      empty_condition_.notify_all();
    } else {
      size_t new_workers = target_workers - workers_.size();

      for (size_t w = 0; w < new_workers; ++w) {
        add_worker(mtx_);
      }
      target_workers_ = target_workers;
    }
    return true;
  }

  bool empty() {
    std::lock_guard<std::mutex>(mtx_);
    return scheduler_.empty();
  }
  size_t pending() {
    std::lock_guard<std::mutex>(mtx_);
    return pending_tasks_.load();
  }
  size_t active() {
    std::lock_guard<std::mutex>(mtx_);
    return active_tasks_.load();
  }
  size_t size() {
    std::lock_guard<std::mutex>(mtx_);
    return workers_.size();
  }

 private:
  void add_worker(std::mutex mtx_) {
    auto worker = std::make_shared<worker_type>(this);
    worker->set_thread_handle(
        std::make_shared<std::thread>(std::bind(&worker_type::run, worker)));
  }

  void add_worker_pending_destruction(std::shared_ptr<std::thread> w) {
    std::lock_guard<std::mutex>(mtx_);
    workers_pending_destruction_.emplace_back(w);
  }


 private:
  scheduler_type scheduler_;

 private:
  int counter_;
  std::atomic<int> active_tasks_;
  std::atomic<int> pending_tasks_;

 private:
  std::vector<std::shared_ptr<std::thread>> workers_pending_destruction_;
  size_t workers_;
  size_t target_workers_;

 private:
  std::mutex mtx_;
  std::condition_variable empty_condition_;
};

#endif
