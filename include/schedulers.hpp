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
