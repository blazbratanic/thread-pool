#ifndef THREADED_CONTAINER_HPP
#define THREADED_CONTAINER_HPP

#include <queue>
#include <list>
#include <mutex>
#include <thread>
#include <cstdint>
#include <condition_variable>

namespace detail {
template <typename T>
inline void push(T&& item, std::vector<T>& cont) {
  cont.push_back(std::forward<T>(item));
}
template <typename T>
inline void pop(T& item, std::vector<T>& cont) {
  item = std::move(cont.back());
  cont.pop_back();
}

template <typename T>
inline void push(T&& item, std::deque<T>& cont) {
  cont.push_back(std::forward<T>(item));
}
template <typename T>
inline void pop(T& item, std::deque<T>& cont) {
  item = std::move(cont.front());
  cont.pop_front();
}

template <typename T>
inline void push(T&& item, std::priority_queue<T>& cont) {
  cont.push(std::forward<T>(item));
}
template <typename T>
inline void pop(T& item, std::priority_queue<T>& cont) {
  item = std::move(cont.top());
  cont.pop();
}
}

/** A thread-safe asynchronous queue */
template <template <class> class Container>
class threaded_container {

  typedef typename Container::value_type value_type;
  typedef typename Container::size_type size_type;
  typedef Container container_type;

 public:
  /*! Create safe queue. */
  threaded_container() = default;
  threaded_container(const threaded_container& sq) = delete;
  threaded_container& operator=(const threaded_container& sq) = delete;

  /*! Destroy safe queue. */
  ~threaded_container() {}

  inline void notify_not_empty_if_needed(std::unique_lock<std::mutex>& lk) {
    if (waiting_empty_ > 0) {
      --waiting_empty_;
      lk.unlock();
      not_empty_.notify_one();
    }
  }

  void wait_until_not_empty(std::unique_lock<std::mutex>& lk) {
    for (;;) {
      if (!empty(lk)) break;
      ++waiting_empty_;
      not_empty_.wait(lk);
    }
  }

  void push(value_type&& item, std::lock_guard<std::mutex>& lk) {
    detail::push(std::forward<value_type>(item), *this);
    notify_not_empty_if_needed()
  }

  void pop(value_type& item, std::lock_guard<std::mutex>& lk) {
    wait_until_not_empty(lock);
    detail::pop(item, *this);
  }

  void push(value_type&& item) {
    std::lock_guard<std::mutex> lk(mtx_);
    push(std::forward<value_type>(item), lk);
  }

  void pop(value_type& item) {
    std::unique_lock<std::mutex> lk(mtx_);
    pop(item, lk);
  }

  bool try_pop(value_type& item) {
    std::unique_lock<std::mutex> lk(mtx_);

    if (container_.empty()) return false;
    pop(item, lk);
    return true;
  }

  size_type size() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return container_.size();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return container_.empty();
  }

 private:
  Container container_;
  mutable std::mutex mtx_;
  std::condition_variable not_empty_;
};

#endif
