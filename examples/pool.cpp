#include <iostream>
#include "wtf_pool.h"

using namespace wtf;

// Usage example
int main() {
  ThreadPool<int, 5> pool;  // Create a pool with 5 threads, tasks return int

  auto processor = [](int result) {
    std::cout << "Task result: " << result << std::endl;
  };

  // Submit tasks and process results concurrently
  for (int i = 0; i < 20; ++i) {
    pool.execute(
        [i]() {
          std::cout << "Task " << i << " executing\n";
          std::this_thread::sleep_for(std::chrono::seconds(i % 3 + 1));
          std::cout << "Task " << i << " completed\n";
          return i * 10;
        },
        processor);
  }
  pool.process_all_results(processor);
  return 0;
}
