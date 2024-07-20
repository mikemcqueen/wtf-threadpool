#include <iostream>
#include "wtf_pool.h"

using namespace wtf;

// Usage example
int main() {
  ThreadPool<int, 5> pool;  // Create a pool with 5 threads, tasks return int

  auto process_task = [](int result) {
    std::cout << "Task result: " << result << std::endl;
  };

  // Submit tasks and process results concurrently
  for (int i = 0; i < 20; ++i) {
    while (!pool.execute([i]() {
      std::cout << "Task " << i << " executing\n";
      std::this_thread::sleep_for(std::chrono::seconds( i % 3 + 1));
      std::cout << "Task " << i << " completed\n";
      return i * 10;
    })) {
      // If submission fails, wait for a task to complete and process it
      pool.wait_for_any_result();
      pool.process_one_result(process_task);
    }
  }
  pool.process_all_results(process_task);
  return 0;
}
