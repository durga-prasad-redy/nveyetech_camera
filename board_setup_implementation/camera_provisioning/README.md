# Camera Provisioning & Device Setup

Embedded utilities for initial camera device provisioning and configuration during manufacturing. Handles MAC address assignment, device identification, factory backup creation, and time synchronization.

## Overview

This module provides two main utilities for device initialization and provisioning:

1. **device_setup** - Comprehensive device provisioning and initialization
2. **set_mac_uboot** - U-Boot MAC address configuration utility

## Components

### 1. device_setup

The primary provisioning tool that configures all essential device parameters during manufacturing.

#### Purpose

Initializes a new camera device with unique identifiers and configuration while capturing factory defaults for future recovery.

#### Features

- **MAC Address Configuration**: Sets unique Ethernet MAC address
- **Device Identification**: Assigns serial number and manufacturing date
- **Hotspot Configuration**: Auto-generates WiFi hotspot SSID based on MAC
- **Time Synchronization**: Sets device system time via Unix timestamp
- **Factory Backup**: Creates comprehensive backup of factory default configurations
- **Default Snapshot**: Captures initial configuration state
- **Self-Cleanup**: Removes binary after successful execution
- **Comprehensive Validation**: Input format and value validation

#### Usage

```bash
./device_setup <MAC> <SERIAL> <MFG_DATE> <EPOCH_TIME>
```

#### Arguments

| Argument | Format | Example | Description |
|----------|--------|---------|-------------|
| MAC | XX:XX:XX:XX:XX:XX | 02:1A:2B:3C:4D:5E | Ethernet MAC address (no all-00 or all-FF) |
| SERIAL | String | SN20251027001 | Device serial number |
| MFG_DATE | YYYY-MM-DD | 2025-10-27 | Manufacturing date |
| EPOCH_TIME | 10-digit number | 1767619974 | Unix timestamp for system time |

#### Example

```bash
./device_setup 02:1A:2B:3C:4D:5E SN20251027001 2025-10-27 1767619974
```

#### Output

```
Successfully set ethaddr to 02:1A:2B:3C:4D:5E
Serial and Mfg date written successfully.
Hotspot SSID updated successfully.
Successfully set Epoch time to 1767619974
Default configuration snapshot captured.
Factory backup complete.
Deleted binary: /mnt/flash/vienna/firmware/board_setup/device_setup
```

#### Execution Workflow

The device_setup tool performs the following steps:

```
Step 1: Validate MAC address format (XX:XX:XX:XX:XX:XX)
           ↓
Step 2: Reject invalid MACs (all-00, all-FF, multicast)
           ↓
Step 3: Validate manufacturing date (YYYY-MM-DD)
           ↓
Step 4: Set ethaddr in U-Boot using fw_setenv
           ↓
Step 5: Write serial number to /mnt/flash/vienna/m5s_config/serial_number
           ↓
Step 6: Write manufacturing date to /mnt/flash/vienna/m5s_config/mfg_date
           ↓
Step 7: Update hotspot SSID with last 4 MAC address characters
           ↓
Step 8: Set system time via /mnt/flash/vienna/scripts/time_manager.sh
           ↓
Step 9: Capture configuration snapshot to /mnt/flash/vienna/default_snapshot.txt
           ↓
Step 10: Create factory backup tarballs (vienna.tar, etc.tar)
           ↓
Step 11: Delete binary for security (one-time use)
```

#### Configuration Files

| File Path | Purpose |
|-----------|---------|
| /mnt/flash/vienna/m5s_config/serial_number | Device serial number |
| /mnt/flash/vienna/m5s_config/mfg_date | Manufacturing date (YYYY-MM-DD) |
| /mnt/flash/vienna/m5s_config/hotspot_ssid | WiFi hotspot SSID (auto-updated) |
| /mnt/flash/vienna/default_snapshot.txt | Configuration defaults snapshot |
| /mnt/flash/vienna/factory_backup/ | Factory backup directory |

#### Backup Contents

**vienna.tar** - Vienna directory backup excluding:
- vienna/lib
- vienna/firmware
- vienna/scripts
- vienna/bin
- vienna/factory_backup

**etc.tar** - System /etc directory backup

#### Functions

```c
void capture_defaults(void);
// Captures current configuration file values to snapshot

void capture_factory_backup(void);
// Creates tar archives of factory default state

int validate_mac(const char *mac);
// Validates MAC format: XX:XX:XX:XX:XX:XX
// Returns: 1 if valid, 0 if invalid

int validate_mac_value(const char *mac);
// Rejects all-00, all-FF, and multicast addresses
// Returns: 1 if valid, 0 if invalid

int validate_date(const char *date);
// Validates date format: YYYY-MM-DD with range checking
// Returns: 1 if valid, 0 if invalid

int update_hotspot_ssid(const char *mac);
// Updates hotspot SSID with base_ssid + last 4 MAC chars
// Example: "MyCamera_4D5E" (from MAC ...4D:5E)
// Returns: 0 on success, -1 on error

int write_to_file(const char *path, const char *value);
// Safely writes configuration value to file
// Returns: 0 on success, -1 on error

int run_cmd(const char *cmd);
// Executes system command and checks return status
// Returns: 0 on success, -1 on error
```

#### Validation Rules

**MAC Address:**
- Format: `XX:XX:XX:XX:XX:XX` (colon-separated hex pairs)
- Case-insensitive (A-F or a-f)
- Rejects: `00:00:00:00:00:00` (all zeros)
- Rejects: `FF:FF:FF:FF:FF:FF` (all ones)
- Rejects: Multicast addresses (first octet LSB = 1)
- Allows: Locally administered addresses (second nibble of first octet = 2,6,A,E)

**Date:**
- Format: `YYYY-MM-DD`
- Month: 01-12
- Day: 01-31
- No leap year validation (allows Feb 30th)

**Serial Number:**
- Any string (no validation)
- Recommended format: `SNYYYYMMDDnnn` (e.g., SN20251027001)

**Epoch Time:**
- 10-digit Unix timestamp
- Example: 1767619974 = Oct 27, 2025, 02:32:54 UTC

#### Hotspot SSID Update

The hotspot SSID is updated by:
1. Reading base SSID from `/mnt/flash/vienna/m5s_config/hotspot_ssid`
2. Extracting last 4 hex digits from MAC address (colon characters removed)
3. Appending with underscore separator
4. Writing back to hotspot_ssid file

**Example:**
```
Base SSID: "MyCamera"
MAC: 02:1A:2B:3C:4D:5E
Last 4 chars: 4D5E
Result: "MyCamera_4D5E"
```

### 2. set_mac_uboot

A simple, single-purpose utility for setting MAC address in U-Boot environment.

#### Purpose

Sets the Ethernet MAC address (ethaddr) in U-Boot environment variables using the `fw_setenv` utility.

#### Features

- **MAC Validation**: Validates address format before writing
- **U-Boot Integration**: Uses fw_setenv for proper environment storage
- **Self-Deletion**: Removes binary after successful execution
- **Simple Interface**: Single argument, clear success/failure

#### Usage

```bash
./set_mac_uboot <MAC_ADDRESS>
```

#### Arguments

| Argument | Format | Example |
|----------|--------|---------|
| MAC_ADDRESS | XX:XX:XX:XX:XX:XX | 02:1A:2B:3C:4D:5E |

#### Example

```bash
./set_mac_uboot 02:1A:2B:3C:4D:5E
```

#### Output

```
Successfully set ethaddr to 02:1A:2B:3C:4D:5E
Deleted binary: /mnt/flash/vienna/firmware/board_setup/set_mac_uboot
```

#### Functions

```c
int validate_mac(const char *mac);
// Validates MAC format: XX:XX:XX:XX:XX:XX
// Returns: 1 if valid, 0 if invalid
```

#### Implementation Details

- Calls `fw_setenv ethaddr <MAC>`
- Deletes itself after successful execution
- Returns error if fw_setenv fails
- Does not validate MAC value (only format)

#### Error Handling

| Scenario | Message | Exit Code |
|----------|---------|-----------|
| Wrong argument count | "Usage: ./set_mac_uboot <MAC_ADDRESS>" | 1 |
| Invalid format | "Invalid MAC address format." | 1 |
| fw_setenv fails | "Failed to set ethaddr." | 1 |
| Success | "Successfully set ethaddr to..." | 0 |

## Building

### Prerequisites

- ARM cross-compiler: `/opt/vtcs_toolchain/vienna/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc`
- OpenSSL development headers (for some optional tools)
- C99 compatible compiler
- POSIX system utilities (mkdir, tar, etc.)

### Build Commands

**Build all utilities:**
```bash
make
```

**Build specific target:**
```bash
make device_setup
make set_mac_uboot
```

**Cross-compile for x86 (testing):**
```bash
make ARCH=x86
```

**Clean build artifacts:**
```bash
make clean
```

### Build Output

```
device_setup      - Device provisioning utility
set_mac_uboot     - MAC address U-Boot setter (optional)
```

## Manufacturing Workflow

### Typical Device Setup Process

```
1. Build firmware and load on device
2. Boot device to production U-Boot
3. Prepare unique provisioning parameters:
   - MAC address (assigned from pool)
   - Serial number (auto-generated)
   - Manufacturing date (current date)
   - Current Unix timestamp
4. Execute: ./device_setup <MAC> <SERIAL> <DATE> <EPOCH>
5. Binary self-deletes after success
6. Device reboots with new configuration
7. Verify configuration: cat /mnt/flash/vienna/m5s_config/serial_number
```

### Batch Provisioning Script

```bash
#!/bin/bash
# provision_devices.sh

DEVICE_IP=$1
MAC=$2
SERIAL=$3
MFG_DATE=$4

EPOCH=$(date +%s)

# Copy device_setup to device
scp device_setup root@${DEVICE_IP}:/mnt/flash/vienna/firmware/board_setup/

# Execute provisioning
ssh root@${DEVICE_IP} \
  "/mnt/flash/vienna/firmware/board_setup/device_setup \
   ${MAC} ${SERIAL} ${MFG_DATE} ${EPOCH}"

if [ $? -eq 0 ]; then
    echo "Successfully provisioned ${SERIAL}"
else
    echo "Failed to provision ${SERIAL}"
    exit 1
fi
```

## File System Layout

```
/mnt/flash/
├── vienna/
│   ├── m5s_config/
│   │   ├── serial_number
│   │   ├── mfg_date
│   │   ├── hotspot_ssid
│   │   └── onvif_itrf
│   ├── firmware/
│   │   └── board_setup/
│   │       ├── device_setup (executable)
│   │       └── set_mac_uboot (executable)
│   ├── scripts/
│   │   └── time_manager.sh
│   └── factory_backup/
│       ├── vienna.tar
│       └── etc.tar
└── etc/
```

## Environment Variables

### fw_setenv/fw_getenv

U-Boot environment variable management:
- **ethaddr**: MAC address (set by set_mac_uboot or device_setup)
- **bootargs**: Kernel boot arguments
- **bootcmd**: Boot command

Example:
```bash
fw_getenv ethaddr          # Read current MAC
fw_setenv ethaddr 02:1A:2B:3C:4D:5E  # Set MAC
```

## Troubleshooting

### device_setup Failures

**"Invalid MAC address format"**
- Check format is XX:XX:XX:XX:XX:XX
- Verify all 6 pairs are present
- Ensure hex characters are valid (0-9, A-F)

**"Invalid manufacturing date format"**
- Use YYYY-MM-DD format
- Valid month: 01-12
- Valid day: 01-31

**"Failed to set ethaddr"**
- Verify fw_setenv is installed: `which fw_setenv`
- Check U-Boot environment is writable
- Confirm device has root privileges

**"Failed to write serial_number"**
- Verify directory exists: `/mnt/flash/vienna/m5s_config/`
- Check write permissions on directory
- Ensure sufficient disk space

**"Failed to update hotspot_ssid"**
- Verify `/mnt/flash/vienna/m5s_config/hotspot_ssid` exists
- File must be readable and writable
- Check for disk space

### set_mac_uboot Failures

**"Failed to set ethaddr"**
- Verify `fw_setenv` is installed
- Check device has root privileges
- Confirm U-Boot partition is accessible

### Factory Backup Issues

**"Capturing full factory backup..." hangs**
- Check available disk space: `df -h /mnt/flash/`
- Verify tar utility is present: `which tar`
- May take several minutes on slow storage

**Backup files too large**
- Backup includes all of /vienna and /etc directories
- Consider deleting old logs before provisioning
- Pre-clean temporary files

## Security Considerations

1. **Binary Self-Deletion**: Both utilities delete themselves after successful execution for manufacturing security
2. **Sensitive Data**: Configuration files contain device identifiers - protect access
3. **Root Privileges**: Utilities require root to modify U-Boot environment and system time
4. **Factory Backup**: Contains sensitive configuration - restrict access to authorized personnel
5. **Serial Numbers**: Should be unique and traceable for warranty/support

## Performance Characteristics

- **device_setup**: ~30-60 seconds (includes factory backup creation)
- **set_mac_uboot**: <1 second
- **Factory backup creation**: Depends on filesystem size (typically 30-60 seconds)

## Limitations

- One-time use utilities (self-delete after success)
- Requires root/sudo privileges
- Requires fw_setenv for U-Boot interaction
- No rollback capability (backup tarballs created after device_setup completes)
- MAC address cannot be changed after initial provisioning without manual intervention

## Related Files

- U-Boot environment: `/dev/mtd* or /mnt/mtd*`
- Time manager script: `/mnt/flash/vienna/scripts/time_manager.sh`
- Configuration directory: `/mnt/flash/vienna/m5s_config/`
- Firmware directory: `/mnt/flash/vienna/firmware/`

## Author

Rikin Shah  
Date: 2025-10-27 (device_setup), 2025-06-24 (set_mac_uboot)

