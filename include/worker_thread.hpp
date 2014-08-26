#ifndef WORKER_THREAD_HPP
#define WORKER_THREAD_HPP

#include <memory>

template <class Pool>
class worker_thread {
  worker_thread(worker_thread const &) = delete;
  worker_thread &operator=(worker_thread const &) = delete;

 public:
  using pool_type = Pool;

  worker_thread(pool_type* pool) : pool_(pool) {}

  void run() {
    while (pool_->execute_task()) {}
  }

 private:
  pool_type* pool_;
};


#endif
