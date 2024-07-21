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
#include <string_view>

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

  constexpr static bool logging = false;
  static void log(std::string_view sv) {
    if constexpr(logging) {
      std::cerr << sv << std::endl;
    }
  }

  static void worker_function(ReusableThread* t) {
    while (!t->stop.load(std::memory_order_relaxed)) {
      log("worker: acquiring task_executable");
      t->task_executable.acquire();
      if (t->stop.load(std::memory_order_relaxed)) break;
      log("worker: running task");
      t->current_task();
    }
    log("worker: exiting");
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
        log("execute: task complete, but result not retrieved");
        return false;
      }
      if (!is_ready()) {
        log("execute: task not ready");
        return false;
      }
    } else {
      log("execute: creating new thread");
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
          log("task: setting result");
          promise.set_value();
        } else {
          auto result = func();
          log("task: setting result");
          promise.set_value(result);
        }
      } catch (...) {  //
        promise.set_exception(std::current_exception());
      }
      log("task: releasing result_ready");
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
          log("task: setting result");
          promise.set_value(result);
        }
        log("task: releasing result_ready");
      } catch (...) {  //
        promise.set_exception(std::current_exception());
      }
      result_ready.release();
      pool->notify_result_ready();
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
      log("get_result(void)");
      future.get();
      log("acquiring result_ready");
      result_ready.acquire();
      log("releasing task_ready");
      task_ready.release();
    } else {
      log("getting result");
      auto result = future.get();
      log("acquiring result_ready");
      result_ready.acquire();
      log("releasing task_ready");
      task_ready.release();
      return result;
    }
  }

  void wait_for_result() {
    if (is_ready()) {
      log("wait_for_completion: task is ready, returning immediately");
      return;
    }
    log("waiting for result...");
    result_ready.acquire();
    result_ready.release();
    log("done waiting for result");
  }
};

} // namespace wtf
