#include "motocam_api_l2.h"
#include "log.h"
#include "net.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>


// Use atomic for thread-safety and signal handling
// Keeping it static to the file (internal linkage)
static std::atomic<bool> g_running{true};

static void signal_handler(int sig) {
  // Note: LOG_INFO might not be "async-signal-safe".
  // In a production app, just set the flag and log after the loop breaks.
  g_running = false;
}

int main(int argc, char *argv[]) {
  bool log_enabled = false;

  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--log") {
      log_enabled = true;
      break;
    }
  }

  // RAII: Initialize Net inside main so it cleans up when it goes out of scope
  Net net;

  LOG_INFO("Starting embedded web server...");

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  init_motocam_configs();

  if (!net.init()) {
    LOG_ERROR("Failed to initialize web server");
    return 1;
  }

  LOG_INFO("Web server is running. Press Ctrl+C to stop.");

  while (g_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  LOG_INFO("Shutting down web server...");
  net.cleanup();

  return 0;
}