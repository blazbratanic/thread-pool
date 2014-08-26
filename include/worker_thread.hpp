#ifndef WORKER_THREAD_HPP
#define WORKER_THREAD_HPP

#include <memory>

template <class Pool>
class worker_thread {
  worker_thread(worker_thread const &) = delete;
  worker_thread &operator=(worker_thread const &) = delete;

 public:
  using pool_type = Pool;

  worker_thread(std::shared_ptr<pool_type> pool) : pool_(pool) {}

  void start() {
    while (true) {
      pool->execute_task();
    }
  }

 private:
  std::shared_ptr<pool_type> pool_;
};

#endif
