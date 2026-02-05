# Motocam API Development Guide

This document provides a step-by-step guide on how to add new APIs to the Motocam codebase. The architecture follows a layered approach (L1 and L2) using a custom binary protocol.

## Architecture Overview

The API is structured around **Commands** and **SubCommands**.

1.  **Entry Point**: `do_processing` in `motocam_api_l1.cpp` receives the raw byte packet.
2.  **Dispatch**: It parses the **Header** (GET/SET) and **Command** (e.g., SYSTEM, NETWORK).
3.  **L1 Layer** (`src/l1/`): Responsible for:
    *   Protocol parsing and validation.
    *   Dispatching to the specific SubCommand handler.
    *   Input validation (checking data length, value ranges).
    *   Calling the L2 layer.
4.  **L2 Layer** (`src/l2/`): Responsible for:
    *   Business logic.
    *   Interacting with the system, hardware, or internal state.
    *   Returning success/error codes.

## Step-by-Step Guide

### 1. Define the Command
First, register your new Command or SubCommand in `include/motocam_command_enums.h`.

**If adding a new SubCommand to an existing Command (e.g., SYSTEM):**

```c
// include/motocam_command_enums.h

// ... inside enum SystemSetSubCommands or SystemGetSubCommands
enum SystemSetSubCommands{
    // ... existing
    MY_NEW_FEATURE = 10, // Add your new subcommand
};
```

**If adding a completely NEW Top-Level Command:**

```c
// include/motocam_command_enums.h

enum Commands{
    // ... existing
    MY_NEW_MODULE = 7, // Add your new command module
};

// Define your subcommands
enum MyNewModuleSubCommands {
    ACTION_ONE = 1,
    ACTION_TWO = 2
};
```

### 2. Update the Dispatcher (L1 Helper)
If you added a **NEW Top-Level Command**, you must teach the helper how to route it.

Modify `src/l1/motocam_helper_api_l1.cpp`:

```cpp
// src/l1/motocam_helper_api_l1.cpp

#include "motocam_my_new_module_api_l1.h" // Include your new header

int8_t set_command(...) {
    switch (command) {
        // ... existing cases
        case MY_NEW_MODULE:
            return set_my_new_module_command(sub_command, data_length, data);
    }
}

// Similarly for get_command if applicable
```

### 3. Implement L1 Layer
Create or update files in `src/l1/`.

**If adding to existing module (e.g., System):**
Open `src/l1/motocam_system_api_l1.cpp` and update `set_system_command` (or `get_system_command`) call your new handler.

```cpp
// src/l1/motocam_system_api_l1.cpp

int8_t set_system_command(...) {
    switch (sub_command) {
        // ...
        case MY_NEW_FEATURE:
            return set_system_my_new_feature_l1(data_length, data);
    }
}

// Implement the L1 handler
int8_t set_system_my_new_feature_l1(const uint8_t data_length, const uint8_t *data) {
    // 1. Validate Input
    if (data_length < 1) {
        return -5; // Invalid length
    }

    // 2. Call L2
    if (set_system_my_new_feature_l2(data_length, data) == 0) {
        return 0; // Success
    }
    return -1; // Error
}
```

**If creating a new module:**
Create `src/l1/motocam_my_new_module_api_l1.cpp` and `src/l1/motocam_my_new_module_api_l1.h`.

### 4. Implement L2 Layer
Create or update files in `src/l2/`. The L1 layer should call functions in L2.

**If adding to existing module:**
Open `src/l2/motocam_system_api_l2.cpp`.

```cpp
// src/l2/motocam_system_api_l2.cpp

int8_t set_system_my_new_feature_l2(const uint8_t data_length, const uint8_t *data) {
    // Perform the actual work here
    printf("Executing my new feature!\n");
    
    // Return 0 for success, other values for error
    return 0; 
}
```

**If creating a new module:**
Create `src/l2/motocam_my_new_module_api_l2.cpp` and `src/l2/motocam_my_new_module_api_l2.h`.

### 5. Build System
If you created **new files**, you must register them in `CMakeLists.txt`.

```cmake
# CMakeLists.txt

add_library(motocam_api_libs
    # ... existing files
    src/l1/motocam_my_new_module_api_l1.cpp
    src/l2/motocam_my_new_module_api_l2.cpp
)
```

## Summary of Responsibilities

| File / Location | Responsibility |
| :--- | :--- |
| `include/motocam_command_enums.h` | Define unique IDs for Commands and SubCommands. |
| `src/l1/motocam_helper_api_l1.cpp` | **Router**: Routes Top-Level Commands to their specific L1 handlers. |
| `src/l1/motocam_[module]_api_l1.cpp` | **Controller**: Validates input (length, type) and calls L2. Dispatches SubCommands. |
| `src/l2/motocam_[module]_api_l2.cpp` | **Service**: Executes the actual business logic. |
