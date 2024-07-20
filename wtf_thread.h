#pragma once
#include <thread>
#include <functional>
#include <future>
#include <iostream>
#include <atomic>
#include <optional>
#include <semaphore>
#include <thread>
#include <functional>
#include <future>
#include <semaphore>

namespace wtf {

template <typename R, int Size> class ThreadPool;

template <typename R>
class ReusableThread {
private:
  std::optional<std::jthread> thread{};
  std::move_only_function<void()> current_task;
  mutable std::binary_semaphore task_ready{1};
  mutable std::binary_semaphore task_executable{0};
  mutable std::binary_semaphore result_ready{0};
  std::future<R> future;
  std::atomic<bool> stop{false};

  static void worker_function(ReusableThread* t) {
    while (!t->stop.load(std::memory_order_relaxed)) {
      std::cout << "worker: acquiring task_executable\n";
      t->task_executable.acquire();
      if (t->stop.load(std::memory_order_relaxed)) break;
      std::cout << "worker: running task\n";
      t->current_task();
    }
    std::cout << "worker: exiting\n";
  }

public:
  ReusableThread() : thread(std::nullopt) {}
  ~ReusableThread() {
    stop.store(true, std::memory_order_relaxed);
    task_executable.release();
  }

  bool pre_execute_common() {
    if (thread.has_value()) {
      if (has_result()) {
        std::cout << "execute: task complete, but result not retrieved\n";
        return false;
      }
      if (!is_ready()) {
        std::cout << "execute: task not ready\n";
        return false;
      }
    } else {
      std::cout << "execute: creating new thread\n";
      thread = std::jthread{worker_function, this};
    }
    task_ready.acquire();
    return true;
  }

  template <typename F>
  // requires is_same_v(decltype(F()), R>
  // or is_same_v<invoke_result_t<F, ??>, R>
  bool execute(F func) {
    if (!pre_execute_common()) return false;
    std::promise<R> promise;
    future = promise.get_future();
    current_task = [this, func = std::move(func),
     promise = std::move(promise)]() mutable {
      try {
        if constexpr (std::is_void_v<R>) {
          func();
          promise.set_value();
        } else {
          auto result = func();
          std::cout << "task: setting result\n";
          promise.set_value(result);
        }
        std::cout << "task: releasing result_ready\n";
      } catch (...) {  //
        promise.set_exception(std::current_exception());
      }
      result_ready.release();
    };
    task_executable.release();
    return true;
  }

  template <typename F, int PoolSize>
  // requires is_same_v(decltype(F()), R>
  // or is_same_v<invoke_result_t<F, ??>, R>
  bool execute(F func, ThreadPool<R, PoolSize>* pool = nullptr) {
    if (!pre_execute_common()) return false;
    std::promise<R> promise;
    future = promise.get_future();
    current_task = [this, pool, func = std::move(func),
     promise = std::move(promise)]() mutable {
      try {
        if constexpr (std::is_void_v<R>) {
          func();
          promise.set_value();
        } else {
          auto result = func();
          std::cout << "task: setting result\n";
          promise.set_value(result);
        }
        std::cout << "task: releasing result_ready\n";
      } catch (...) {  //
        promise.set_exception(std::current_exception());
      }
      result_ready.release();
      pool->result_ready();
    };
    task_executable.release();
    return true;
  }

  bool is_ready() const {
    if (task_ready.try_acquire()) {
      task_ready.release();
      return true;
    }
    return false;
  }

  bool has_result() const {
    if (result_ready.try_acquire()) {
      result_ready.release();
      return true;
    }
    return false;
    /*
    return future.valid()
           && future.wait_for(std::chrono::seconds(0))
                  == std::future_status::ready;
    */
  }

  R get_result() {
    if (!has_result()) {
      throw std::runtime_error("Task has no result");
    }
    if constexpr (std::is_void_v<R>) {
      std::cout << "get_result(void)\n";
      future.get();
      std::cout << "acquiring result_ready\n";
      result_ready.acquire();
      std::cout << "releasing task_ready\n";
      task_ready.release();
    } else {
      std::cout << "getting result\n";
      auto result = future.get();
      std::cout << "acquiring result_ready\n";
      result_ready.acquire();
      std::cout << "releasing task_ready\n";
      task_ready.release();
      return result;
    }
  }

  void wait_for_completion() {
    if (is_ready()) {
      std::cout << "wait_for_completion: task is ready, returning immediately\n";
      return;
    }
    std::cout << "waiting for completion...\n";
    result_ready.acquire();
    result_ready.release();
    std::cout << "done waiting.\n";
  }
};

} // namespace wtf
