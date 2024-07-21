#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <future>
#include <optional>
#include <semaphore>
#include <vector>
#include "wtf_thread.h"

namespace wtf {

template<typename R, int Size>
class ThreadPool {
public:
  // Execute on any thread that is ready, or return false if no threads are
  // ready.
  //
  bool try_execute(std::function<R()> func) {
    for (auto& thread : threads) {
      if (thread.is_ready()) {
        thread.execute(std::move(func), this);
        return true;
      }
    }
    return false;  // All threads are busy
  }

  // Execute on any thread that is ready, or, if no thread is ready, wait for
  // one to complete, process its result, and run on that now-ready thread.
  //
  // If process_first is true, check for a ready-result prior to attempting to
  // run on a ready-thread. This will delay the creation of new threads.
  //
  template <class Func>
  void execute(std::function<R()> func, const Func& processor,
      bool process_first = true) {
    bool result_processed{false};
    if (process_first && any_result_ready()) {
      process_one_result(processor);
    }
    if (result_processed || any_thread_ready()) {
      try_execute(std::move(func));
      return;
    }
    wait_for_any_result();
    process_one_result(processor);
    try_execute(std::move(func));
  }

  auto any_result_ready() const {
    if (result_ready.try_acquire()) {
      result_ready.release();
      return true;
    }
    return false;
  }

  void wait_for_any_result() {
    result_ready.acquire();
    result_ready.release();
  }

  template <class Func>
  auto process_one_result(const Func& processor) {
    for (auto& thread : threads) {
      if (thread.has_result()) {
        if constexpr (std::is_void_v<R>) {
          thread.get_result();
          result_ready.acquire();
          processor();
        } else {
          auto result = thread.get_result();
          result_ready.acquire();
          processor(std::move(result));
        }
        return true;
      }
    }
    return false;  // No completed task found
  }

  template <class Func>
  void process_all_results(const Func& processor) {
    while (has_pending_tasks()) {
      wait_for_any_result();
      process_one_result(processor);
    }
  }

  auto has_pending_tasks() const {
    return std::any_of(threads.begin(), threads.end(),
        [](const ReusableThread<R>& t) { return !t.is_ready(); });
  }

  friend class ReusableThread<R>;

private:
  auto any_thread_ready() const {
    for (auto& thread : threads) {
      if (thread.is_ready()) return true;
    }
    return false;
  }

  void notify_result_ready() { result_ready.release(); }

  std::array<ReusableThread<R>, Size> threads;
  mutable std::counting_semaphore<Size> result_ready{0};
};

} // namespace wtf
