# DOB Validation Enhancement Guide

## Overview

This document describes the DOB (Date of Birth) validation feature added to the CONFIG_RESET, FACTORY_RESET, and SET_LOGIN APIs.

## Functional Requirements

### 1. DOB as Mandatory Input
- All three APIs (CONFIG_RESET, FACTORY_RESET, SET_LOGIN) now require DOB as a mandatory input parameter
- DOB format must strictly be `dd-mm-yyyy` (10 characters)
- Example: `15-01-1990`

### 2. DOB Existence Check
Before processing any API call, the system checks if a DOB is already stored:
- If DOB is **not set**: API aborts and returns error code `-6`
- If DOB is **set**: API proceeds to validation

### 3. DOB Validation
The input DOB must:
- Match the format `dd-mm-yyyy`
- Be a valid date (e.g., no 32-13-2023, 00-00-0000)
- Not be a future date (validation can be enabled in code if needed)
- Match the stored DOB exactly

### 4. Error Codes

| Error Code | Description |
|------------|-------------|
| `0` | Success |
| `-1` | General error / DOB format validation failed |
| `-2` | Invalid date (e.g., invalid month or day) |
| `-5` | Invalid data length |
| `-6` | DOB not set in system |
| `-7` | DOB validation failed (mismatch) |

## API Changes

### 1. FACTORY_RESET API

**Previous Format:**
```
Data Length: 0 bytes
Data: None
```

**New Format:**
```
Data Length: 10 bytes
Data: DOB in dd-mm-yyyy format
```

**Example:**
```c
// Input data
uint8_t data[] = "15-01-1990";
uint8_t data_length = 10;

// API Call
int8_t result = set_system_factory_reset_l1(data_length, data);

// Result codes:
// 0  = Success
// -5 = Invalid data length
// -6 = DOB not set in system
// -7 = DOB validation failed
```

### 2. CONFIG_RESET API

**Previous Format:**
```
Data Length: 0 bytes
Data: None
```

**New Format:**
```
Data Length: 10 bytes
Data: DOB in dd-mm-yyyy format
```

**Example:**
```c
// Input data
uint8_t data[] = "15-01-1990";
uint8_t data_length = 10;

// API Call
int8_t result = set_system_config_reset_l1(data_length, data);

// Result codes:
// 0  = Success
// -5 = Invalid data length
// -6 = DOB not set in system
// -7 = DOB validation failed
```

### 3. SET_LOGIN API

**Previous Format:**
```
Data Length: 1-32 bytes
Data: login_pin
```

**New Format:**
```
Data Length: 12-43 bytes
Data: [login_pin_length (1 byte)] + [login_pin (1-32 bytes)] + [DOB (10 bytes)]
```

**Example:**
```c
// For a 4-character PIN "1234" and DOB "15-01-1990"
uint8_t data[] = {
    4,                              // login_pin_length
    '1', '2', '3', '4',             // login_pin
    '1', '5', '-', '0', '1', '-', '1', '9', '9', '0'  // DOB
};
uint8_t data_length = 15;  // 1 + 4 + 10

// API Call
int8_t result = set_system_login_l1(data_length, data);

// Result codes:
// 0  = Success
// -5 = Invalid data length or format
// -6 = DOB not set in system
// -7 = DOB validation failed
```

## Setting DOB (First Time Setup)

Before using the protected APIs, you must set the DOB in the system:

**API:** `SET_USER_DOB`

**Format:**
```
Data Length: 10 bytes
Data: DOB in dd-mm-yyyy format
```

**Example:**
```c
uint8_t dob[] = "15-01-1990";
uint8_t dob_length = 10;

int8_t result = set_system_user_dob_l1(dob_length, dob);

// Result codes:
// 0  = Success
// -1 = Invalid format
// -2 = Invalid date
// -5 = Invalid data length
```

## Getting DOB

To retrieve the stored DOB:

**API:** `GET_USER_DOB`

**Format:**
```
Data Length: 0 bytes
Response: DOB in dd-mm-yyyy format (10 bytes)
```

**Example:**
```c
uint8_t *dob_response;
uint8_t dob_length;

int8_t result = get_system_user_dob_l1(0, NULL, &dob_response, &dob_length);

if (result == 0) {
    // dob_response contains the DOB string
    // dob_length should be 10
    printf("Stored DOB: %.*s\n", dob_length, dob_response);
    free(dob_response);
} else {
    // DOB not found or error
}
```

## Implementation Architecture

### Layer Structure

```
┌─────────────────────────────────────┐
│  L1 Layer (motocam_system_api_l1)  │
│  - Protocol parsing                 │
│  - Extract DOB from binary data     │
│  - Data length validation           │
└───────────────┬─────────────────────┘
                │
                ▼
┌─────────────────────────────────────┐
│  L2 Layer (motocam_system_api_l2)  │
│  - DOB format validation            │
│  - Convert binary to string         │
│  - Business logic                   │
└───────────────┬─────────────────────┘
                │
                ▼
┌─────────────────────────────────────┐
│  FW Layer (fw_system)               │
│  - DOB existence check              │
│  - DOB comparison validation        │
│  - Execute reset/login operations   │
└─────────────────────────────────────┘
```

### Key Functions

**FW Layer:**
- `validate_user_dob(const char *input_dob)` - Validates DOB exists and matches
- `set_factory_reset(const char *dob)` - Factory reset with DOB validation
- `set_config_reset(const char *dob)` - Config reset with DOB validation
- `set_login(const char *login_pin, const char *dob)` - Set login with DOB validation
- `set_user_dob(const char *dob)` - Store DOB
- `get_user_dob(char *dob)` - Retrieve stored DOB

**L2 Layer:**
- `validate_dob_string(const char* date_str)` - Validates DOB format and date validity
- `set_system_factory_reset_l2(dobLength, dob)` - Factory reset L2
- `set_system_config_reset_l2(dobLength, dob)` - Config reset L2
- `set_system_login_l2(loginLength, loginPin, dobLength, dob)` - Set login L2
- `set_system_user_dob_l2(length, dob)` - Set DOB L2
- `get_system_user_dob_l2(dob, length)` - Get DOB L2

**L1 Layer:**
- `set_system_factory_reset_l1(data_length, data)` - Factory reset L1
- `set_system_config_reset_l1(data_length, data)` - Config reset L1
- `set_system_login_l1(data_length, data)` - Set login L1
- `set_system_user_dob_l1(data_length, data)` - Set DOB L1
- `get_system_user_dob_l1(data_length, data, res_data_bytes, res_data_bytes_size)` - Get DOB L1

## Validation Flow

```
┌─────────────────────────────────────┐
│  API Call with DOB                  │
└───────────────┬─────────────────────┘
                │
                ▼
┌─────────────────────────────────────┐
│  Check DOB Length = 10 bytes?       │
│  NO ──> Return -5 (Invalid Length)  │
└───────────────┬─────────────────────┘
                │ YES
                ▼
┌─────────────────────────────────────┐
│  Validate DOB Format (dd-mm-yyyy)?  │
│  NO ──> Return -1/-2 (Invalid Date) │
└───────────────┬─────────────────────┘
                │ YES
                ▼
┌─────────────────────────────────────┐
│  Check if DOB exists in system?     │
│  NO ──> Return -6 (DOB Not Set)     │
└───────────────┬─────────────────────┘
                │ YES
                ▼
┌─────────────────────────────────────┐
│  Compare input DOB with stored DOB? │
│  NO ──> Return -7 (DOB Mismatch)    │
└───────────────┬─────────────────────┘
                │ YES (Match)
                ▼
┌─────────────────────────────────────┐
│  Execute API Operation              │
│  Return 0 (Success)                 │
└─────────────────────────────────────┘
```

## DOB Storage

- DOB is stored in a file: `/mnt/flash/vienna/m5s_config/user_dob`
- File format: Plain text, 10 characters (dd-mm-yyyy)
- No encryption (if security is needed, implement encryption in `set_user_dob` and `get_user_dob`)

## Testing

### Test Case 1: DOB Not Set
```c
// Without setting DOB first
uint8_t data[] = "15-01-1990";
int8_t result = set_system_factory_reset_l1(10, data);
// Expected: result == -6 (DOB not set)
```

### Test Case 2: Invalid DOB Format
```c
// Set valid DOB first
set_system_user_dob_l1(10, (uint8_t*)"15-01-1990");

// Try with invalid format
uint8_t data[] = "15/01/1990";  // Wrong separator
int8_t result = set_system_factory_reset_l1(10, data);
// Expected: result == -1 or -2 (Invalid format)
```

### Test Case 3: DOB Mismatch
```c
// Set valid DOB
set_system_user_dob_l1(10, (uint8_t*)"15-01-1990");

// Try with different DOB
uint8_t data[] = "20-05-1985";
int8_t result = set_system_factory_reset_l1(10, data);
// Expected: result == -7 (DOB validation failed)
```

### Test Case 4: Successful Validation
```c
// Set valid DOB
set_system_user_dob_l1(10, (uint8_t*)"15-01-1990");

// Call with matching DOB
uint8_t data[] = "15-01-1990";
int8_t result = set_system_factory_reset_l1(10, data);
// Expected: result == 0 (Success)
```

### Test Case 5: SET_LOGIN with DOB
```c
// Set valid DOB first
set_system_user_dob_l1(10, (uint8_t*)"15-01-1990");

// Set login with PIN "1234" and matching DOB
uint8_t data[] = {
    4,                              // length
    '1', '2', '3', '4',             // PIN
    '1', '5', '-', '0', '1', '-', '1', '9', '9', '0'  // DOB
};
int8_t result = set_system_login_l1(15, data);
// Expected: result == 0 (Success)
```

## Security Considerations

1. **DOB Storage**: Consider encrypting the DOB file to prevent unauthorized access
2. **Brute Force Protection**: Consider adding rate limiting or lockout after failed attempts
3. **Logging**: Add audit logs for all DOB validation attempts
4. **Transmission**: Ensure DOB is transmitted securely (e.g., over TLS)
5. **Backup**: Consider backing up the DOB securely

## Migration Guide

### For Existing Systems

If your system is already deployed without DOB:

1. **First**: Set the DOB using `SET_USER_DOB` API
2. **Then**: Update clients to include DOB in all FACTORY_RESET, CONFIG_RESET, and SET_LOGIN calls

### For New Systems

1. Set DOB during initial provisioning
2. All subsequent protected operations will require DOB validation

## Troubleshooting

### Error: -6 (DOB Not Set)
**Solution**: Use `SET_USER_DOB` to set the DOB first

### Error: -7 (DOB Validation Failed)
**Solution**: Verify the input DOB matches the stored DOB exactly

### Error: -5 (Invalid Data Length)
**Solution**: Ensure DOB is exactly 10 bytes and follows the expected format for each API

### Error: -1 or -2 (Invalid Format/Date)
**Solution**: 
- Check DOB format is `dd-mm-yyyy`
- Verify date is valid (month 1-12, day appropriate for month)
- Check for valid date components

## File Modifications

The following files were modified to implement this feature:

### Header Files
- `include/fw/fw_system.h` - Added DOB parameter to reset/login functions
- `include/l2/motocam_system_api_l2.h` - Updated L2 function signatures
- `include/l1/motocam_system_api_l1.h` - No changes (maintains backward compatible signatures)

### Implementation Files
- `src/fw/fw_system.cpp` - Added DOB validation and updated functions
- `src/l2/motocam_system_api_l2.cpp` - Added DOB parsing and validation
- `src/l1/motocam_system_api_l1.cpp` - Updated to parse DOB from binary data

## Enums
- `include/motocam_command_enums.h` - No changes (existing enums used)

---

**Version**: 1.0  
**Date**: 2026-01-19  
**Status**: Implemented and Ready for Testing
