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
#include <system_error>
#include <cassert>

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
  thread_pool(size_t pool_size)
      : counter_(0),
        active_tasks_(0),
        pending_tasks_(0),
        active_worker_count_(0),
        target_worker_count_(0) {
    this->resize(pool_size);
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
    std::unique_lock<std::mutex> lk(shutdown_mtx_);

    target_worker_count_ = 0;
    empty_condition_.notify_all();
    while (active_worker_count_ != 0) {
      shutdown_condition_.wait(lk);
    }

    for (auto&& w : workers_pending_destruction_) {
      w->join();
    }
    workers_pending_destruction_.clear();
    return true;
  }

  bool execute_task() {
    task_type task;
    {
      std::unique_lock<std::mutex> lk(mtx_);

      while (scheduler_.empty()) {
        if (active_worker_count_ > target_worker_count_) {
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

  bool resize(size_t target_worker_count) {
    std::unique_lock<std::mutex> lk(mtx_);
    target_worker_count_ = target_worker_count;

    if (active_worker_count_ >= target_worker_count_) {
      empty_condition_.notify_all();
    } else {
      size_t new_worker_count =
          target_worker_count_.load() - active_worker_count_.load();

      for (size_t w = 0; w < new_worker_count; ++w) {
        try {
          auto worker = std::make_shared<worker_type>(this);
          worker->set_thread_handle(std::make_shared<std::thread>(
              std::bind(&worker_type::run, worker)));
          ++active_worker_count_;
        }
        catch (std::system_error const& e) {
          return false;
        }
      }
      assert(active_worker_count_ == target_worker_count_);
    }
    return true;
  }

  bool empty() {
    std::lock_guard<std::mutex> lk(mtx_);
    return scheduler_.empty();
  }

  size_t pending() {
    std::lock_guard<std::mutex> lk(mtx_);
    return pending_tasks_.load();
  }

  size_t active() {
    std::lock_guard<std::mutex> lk(mtx_);
    return active_tasks_.load();
  }

  size_t size() {
    std::lock_guard<std::mutex> lk(mtx_);
    return active_worker_count_.load();
  }

 private:
  void add_worker_pending_destruction(std::shared_ptr<std::thread> w) {
    std::lock_guard<std::mutex> lk(mtx_);
    workers_pending_destruction_.emplace_back(w);
    --active_worker_count_;
    shutdown_condition_.notify_all();
  }

 private:
  scheduler_type scheduler_;

 private:
  int counter_;
  std::atomic<size_t> active_tasks_;
  std::atomic<size_t> pending_tasks_;

  std::atomic<size_t> active_worker_count_;
  std::atomic<size_t> target_worker_count_;

 private:
  std::vector<std::shared_ptr<std::thread>> workers_pending_destruction_;

 private:
  std::mutex mtx_;
  std::condition_variable empty_condition_;
  std::mutex shutdown_mtx_;
  std::condition_variable shutdown_condition_;
};

#endif
