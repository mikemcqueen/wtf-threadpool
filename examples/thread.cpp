#include <iostream>
#include "wtf_thread.h"

using namespace wtf;

// Usage example
int main() {
  ReusableThread<int> thread;

  // Execute first task
  auto t1 = thread.execute([]() {
    std::cout << "Task 1 executing\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Task 1 completed\n";
    return 42;
  });

  if (!t1) {
    std::cout << "Failed to execute task 1!\n";
    std::abort();
  }

  // Try to execute while the thread is busy (should fail)
  auto t_fail =
    thread.execute([]() { std::cout << "This should not execute\n"; return 0; });
  if (t_fail) {
    std::cout << "Successfully executed task which should fail!\n";
    std::abort();
  }
  std::cout << "Correctly failed to execute task while thread was busy\n";

  thread.wait_for_result();
  if (!thread.has_result()) {
    std::cout << "No result for task 1!\n";
    std::abort();
  }

  auto result = thread.get_result();
  std::cout << "Task 1 result: " << result << std::endl;

  // Execute second task
  auto t2 = thread.execute([]() {
    std::cout << "Task 2 executing\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Task 2 completed\n";
    return 43;
  });
  if (!t2) {
    std::cout << "Failed to execute t2!\n";
    std::abort();
  }

  thread.wait_for_result();
  if (!thread.has_result()) {
    std::cout << "No result for task 2!\n";
    std::abort();
  }

  result = thread.get_result();
  std::cout << "Task 2 result: " << result << std::endl;

  return 0;
}
