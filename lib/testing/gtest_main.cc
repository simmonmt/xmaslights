#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {
  std::cerr << "installing failure signal handler\n";
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
