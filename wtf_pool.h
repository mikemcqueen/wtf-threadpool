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
  bool execute(std::function<R()> f) {
    for (auto& thread : threads) {
      if (thread.is_ready()) {
        thread.execute(std::move(f), this);
        return true;
      }
    }
    return false;  // All threads are busy
  }

  void wait_for_any_result() {
    results_ready.acquire();
    results_ready.release();
  }

  template <class Func>
  bool process_one_result(Func& processor) {
    for (auto& thread : threads) {
      if (thread.has_result()) {
        if constexpr (std::is_void_v<R>) {
          thread.get_result();
          results_ready.acquire();
          processor();
        } else {
          auto result = thread.get_result();
          results_ready.acquire();
          processor(std::move(result));
        }
        return true;
      }
    }
    return false;  // No completed task found
  }

  template <class Func>
  void process_all_results(Func& processor) {
    while (has_pending_tasks()) {
      wait_for_any_result();
      process_one_result(processor);
    }
  }

  bool has_pending_tasks() const {
    return std::any_of(threads.begin(), threads.end(),
        [](const ReusableThread<R>& t) { return !t.is_ready(); });
  }

  friend class ReusableThread<R>;

  private:
    void result_ready() { results_ready.release(); }

    std::array<ReusableThread<R>, Size> threads;
    std::counting_semaphore<Size> results_ready{0};
  };
}
