# OTA Package Verification and Installation

A comprehensive Over-The-Air (OTA) update system for firmware deployment on embedded camera devices. This module handles the complete lifecycle of OTA updates including secure package creation, verification, installation, and rollback capabilities.

## Overview

This system is divided into two main components:

- **Web-Side**: Creates and packages OTA updates with cryptographic signing
- **Camera-Side**: Verifies, validates, and applies OTA updates on the device

## System Architecture

### Web-Side (Server/Desktop)

The web-side component prepares firmware packages for distribution:

```
ota_packager → Computes SHA256 hash → Creates manifest → Signs with RSA private key
```

**Key Files:**
- `ota_packager.c` - Main packager executable that creates secure OTA packages
- `ota_packager` - Compiled binary
- `certs/` - Certificate management (includes certificate generation scripts)

**Features:**
- SHA256 integrity hashing of firmware packages
- RSA-2048 cryptographic signing with OpenSSL
- JSON manifest generation for version control and metadata
- Full package creation (combines fw_package.tar.gz and manifest.json into ota.tar.gz)

### Camera-Side (Device/Embedded)

The camera-side handles the installation and verification workflow:

**Key Files:**
- `ota_handler.c/h` - Core OTA management and orchestration
- `ota_service.c` - Service for managing OTA updates
- `verify_ota.c` - Package integrity and authenticity verification
- `post_ota_support.c` - Post-installation operations and cleanup
- `openssl/` - OpenSSL libraries and headers (ARM-compatible)

**Workflow:**
```
Download Package → Verify Signature → Verify Hash → Extract Files → 
Backup Original Files → Apply Updates → Verify Installation → Cleanup
```

## Error Code System

The system uses a structured error code scheme for diagnostics:

| Range | Purpose |
|-------|---------|
| 10-19 | Download errors |
| 20-29 | Verification errors |
| 30-39 | Installation errors |
| 80-89 | Rollback errors |
| 90 | Success (e_OTA_SUCCESSFULLY_DONE) |
| 91-99 | In-progress/internal states |

### Common Error Codes

| Code | Meaning |
|------|---------|
| 0 | Success (e_OTA_SUCCESS) |
| 20 | Verification failed |
| 21 | Package size exceeds limit (2.2 MB) |
| 22 | Manifest parsing error |
| 23 | Manifest file not found |
| 24 | Version mismatch with current firmware |
| 30 | File list extraction error |
| 31 | File backup failed |
| 32 | Package extraction failed |
| 33 | Update failed |
| 35 | Hash format invalid or extraction rollback failed |
| 36 | File rename failed |
| 37 | File not found |
| 38 | Hash mismatch (integrity check failed) |
| 39 | Hash computation failed |
| 80 | Rollback failed |
| 91 | OTA in progress |
| 92 | OTA already in progress (duplicate request) |
| 99 | Internal error |

## Key Features

### Security
- **RSA-2048 Signing**: Cryptographic signature verification of packages
- **SHA-256 Hashing**: Integrity verification of firmware files
- **Manifest Validation**: Ensures package authenticity and compatibility
- **Public Key Infrastructure**: Device stores public key for signature verification

### Reliability
- **File Backup**: Original files backed up before installation
- **Partial Rollback**: Can revert to previous state if installation fails
- **Duplicate Prevention**: Prevents multiple concurrent OTA processes
- **Size Limiting**: Rejects packages exceeding ~2.2 MB

### Versioning
- **Semantic Versioning**: Supports major.minor.patch version format
- **Version Checking**: Validates compatibility before installation
- **Custom Versioning**: Supports cust_XXX format for custom versions

## Configuration

### Directory Structure
```
OTA_DIR = /mnt/flash/vienna/firmware/ota
VIENNA_DIR = /mnt/flash
BACKUP_DIR = /mnt/flash/backup
```

### Key Paths
```c
#define OTA_TAR              OTA_DIR "/fw_package.tar.gz"      // Firmware package
#define MANIFEST_FILE        OTA_DIR "/manifest.json"          // Update metadata
#define FULL_PACKAGE_TAR     OTA_DIR "/ota.tar.gz"             // Complete OTA package
#define PUBLIC_KEY_FILE      OTA_DIR "/public.pem"             // Public key for verification
```

### Limits
- **Max OTA Size**: 2,300,000 bytes (~2.2 MB)
- **Max Retries**: 3 attempts for critical operations
- **Retry Delay**: 1 second between retry attempts

## Building

### Camera-Side (ARM)

```bash
cd camera-side
make ARCH=arm
```

This creates:
- `verify_ota` - Verification binary
- `ota_service` - Main update service
- `post_ota_support` - Post-installation support

### Camera-Side (x86 for Testing)

```bash
cd camera-side
make ARCH=x86
```

### Web-Side

```bash
cd web-side
make
```

This creates:
- `ota_packager` - Package creation utility

## Usage

### Preparing an OTA Package (Web-Side)

```bash
cd web-side
./ota_packager
```

This process:
1. Reads `fw_package.tar.gz` (firmware archive)
2. Computes SHA256 hash of the firmware
3. Reads manifest template or creates manifest.json
4. Signs the package with private key
5. Creates final OTA package: `ota.tar.gz`

**Input Files:**
- `fw_package.tar.gz` - Compiled firmware binary
- `manifest.json` - Package metadata (version, files, etc.)
- `certs/private.pem` - Private key for signing

**Output:**
- `ota.tar.gz` - Complete, signed OTA package for distribution

### Installing an OTA Package (Camera-Side)

```bash
ota_service
```

The service automatically:
1. Extracts the package to OTA_DIR
2. Verifies the digital signature
3. Validates file hashes against manifest
4. Backs up original files
5. Extracts and applies updates
6. Runs any post-installation scripts
7. Cleans up temporary files

### Checking OTA Status

```bash
./verify_ota           # Verify a package
ota_service           # Check/apply updates
post_ota_support      # Perform post-update operations
```

## API Reference

### Core Functions (ota_handler.c)

```c
int verify_package(void);                   // Verify OTA package signature and integrity
int extract_file_list_from_tar(void);      // Extract list of files from package
int backup_files_from_list(void);          // Create backups of files to be updated
int extract_tar_package(void);              // Extract package to target location
int run_updated_component(void);            // Execute installed components
int rollback_partial(void);                 // Revert to backed-up files
int clean_ota_temp_files(void);            // Remove temporary OTA files
int reject_large_update(void);             // Validate package size
int is_ota_in_progress(void);              // Check if update is currently running
int set_config_file_var(const char *var, const char *val); // Update config
void log_msg(const char *msg);             // Log timestamped message
int run_command(const char *desc, const char *cmd); // Execute system command
```

### Verification Functions (verify_ota.c)

```c
int sha256sum(const char *filename, char *out_hex);  // Compute file SHA256
int sign_with_private_key(...);                       // Create RSA signature
int verify_signature(...);                            // Verify RSA signature
```

## Manifest Format

The manifest.json file contains package metadata:

```json
{
    "version": "1.0.0",
    "timestamp": 1234567890,
    "files": [
        {
            "path": "lib/libfirmware.so",
            "hash": "sha256_hex_value",
            "size": 12345
        }
    ],
    "signature": "base64_encoded_signature"
}
```

## OpenSSL Dependencies

The system uses OpenSSL 1.1.1 for cryptographic operations:

- **Library**: `openssl/lib/armeabi-v7a/libssl.so.1.1` and `libcrypto.so.1.1`
- **Headers**: Located in `openssl/include/openssl/`
- **Supported Algorithms**:
  - SHA-256 for hashing
  - RSA-2048 for signing/verification
  - AES for encryption (if needed)

## Testing

### Unit Tests

```bash
cd camera-side/unit_test
gcc -o version_check_test version_check_test.c
./version_check_test
```

### Integration Testing

1. Create a test package: `cd web-side && ./ota_packager`
2. Copy to device: `scp ota.tar.gz device:/mnt/flash/vienna/firmware/ota/`
3. Run service: `ssh device ./ota_service`
4. Verify installation: `ssh device ./verify_ota`

## Rollback Mechanism

If an update fails at any point:

1. **Partial Installation Failure**: System rolls back extracted files using backups
2. **After Completion**: Manual intervention required with backup files in `BACKUP_DIR`
3. **Verification Failure**: Package is rejected before any modifications

Backed-up files are stored in:
```
/mnt/flash/backup/[timestamp]/[file_path]
```

## Security Considerations

1. **Key Management**: Private key must be kept secure on web-side; public key distributed with device
2. **Signature Verification**: Always verify signature before extraction
3. **Size Limits**: Enforced to prevent denial-of-service attacks
4. **File Permissions**: Restored from manifest after extraction
5. **Audit Logging**: All operations logged with timestamps

## Limitations

- Maximum package size: ~2.2 MB
- Package must be valid tar.gz format
- Version must follow semantic versioning or cust_XXX format
- Device must have sufficient storage for backups
- Requires OpenSSL 1.1.1 or compatible

## Author

Rikin Shah  
Date: 2025-06-27

