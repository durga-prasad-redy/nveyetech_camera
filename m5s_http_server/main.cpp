#include "include/motocam_api_l2.h"
#include "net.h"
#include "include/log.h"
#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>
#include <string.h>

// Global flag for graceful shutdown
static volatile bool g_running = true;
bool g_log_enabled = false;

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
  LOG_INFO("Received signal %d, shutting down...", sig);
  g_running = false;
}

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--log") == 0) {
      g_log_enabled = true;
      break;
    }
  }

  LOG_INFO("Starting embedded web server...");

  // Set up signal handlers for graceful shutdown
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  init_motocam_configs();

  // Initialize the web server
  if (!web_init()) {
    LOG_ERROR("Failed to initialize web server");
    return 1;
  }

  LOG_INFO("Web server is running. Press Ctrl+C to stop.");

  // Main loop - keep the server running
  while (g_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  LOG_INFO("Shutting down web server...");

  // Cleanup
  web_cleanup();

  LOG_INFO("Server stopped.");
  return 0;
}
