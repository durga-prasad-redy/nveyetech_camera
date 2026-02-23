#!/bin/sh

# M5S Configuration Files Creator
# This script creates all M5S config files from a default values source file

# Configuration
M5S_CONFIG_DIR="/mnt/flash/vienna/m5s_config"
SOURCE_FILE="$M5S_CONFIG_DIR/m5s_config_files_default"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}[$(date '+%Y-%m-%d %H:%M:%S')] ${message}${NC}"
}

# Function to create config directory
create_config_directory() {
    print_status $BLUE "Creating config directory: $M5S_CONFIG_DIR"
    
    if mkdir -p "$M5S_CONFIG_DIR" 2>/dev/null; then
        print_status $GREEN "✓ Config directory created successfully"
    else
        print_status $YELLOW "! Config directory already exists or cannot be created"
    fi
}

# Function to create a single config file
create_config_file() {
    local key=$1
    local value=$2
    local file_path="$M5S_CONFIG_DIR/$key"
    
    # Create the file with the default value
    if echo "$value" > "$file_path" 2>/dev/null; then
        print_status $GREEN "✓ Created: $key = $value"
        return 0
    else
        print_status $RED "✗ Failed to create: $key"
        return 1
    fi
}

# Function to read and process the source file
process_source_file() {
    local source_file=$1
    
    if [[ ! -f "$source_file" ]]; then
        print_status $RED "✗ Source file not found: $source_file"
        print_status $YELLOW "Creating sample source file..."
        create_sample_source_file
        return 1
    fi
    
    print_status $BLUE "Reading source file: $source_file"
    
    local line_number=0
    local success_count=0
    local error_count=0
    
    while IFS='=' read -r key value || [[ -n "$key" ]]; do
        line_number=$((line_number + 1))
        
        # Skip empty lines and comments
        if [[ -z "$key" ]]; then
            continue
        fi
        # Skip comment lines (lines starting with #)
        comment_check=$(echo "$key" | sed 's/^[[:space:]]*//' | cut -c1)
        if [[ "$comment_check" == "#" ]]; then
            continue
        fi
        
        # Remove leading/trailing whitespace
        key=$(echo "$key" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        value=$(echo "$value" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        
        # Skip if key is empty after trimming
        if [[ -z "$key" ]]; then
            print_status $YELLOW "! Skipping empty key at line $line_number"
            continue
        fi
        
        # Create the config file
        if create_config_file "$key" "$value"; then
            success_count=$((success_count + 1))
        else
            error_count=$((error_count + 1))
        fi
        
    done < "$source_file"
    
    print_status $BLUE "Processing complete:"
    print_status $GREEN "  ✓ Successfully created: $success_count files"
    if [[ $error_count -gt 0 ]]; then
        print_status $RED "  ✗ Failed to create: $error_count files"
    fi
}

# Function to create a sample source file
create_sample_source_file() {
    cat > "$SOURCE_FILE" << 'EOF'
ir_led_brightness=0
resolution=1
day_mode=1
gyro_reader=0
wdr=0
eis=0
flip=0
mirror=0
image_zoom=1
tilt=0
misc=1
ircut_filter=0
hotspot_ssid=M5S_Camera
hotspot_encryption_type=2
hotspot_encryption_key=password123
hotspot_ipaddress=192.168.1.100
hotspot_subnetmask=255.255.255.0
client_ssid=MyWiFi
client_encryption_type=2
client_encryption_key=wifipassword
client_ipaddress=192.168.1.101
client_subnetmask=255.255.255.0
wifi_state=1
camera_name=M5S_Camera_001
firmware_version=1.0.0
ethaddr=00:11:22:33:44:55
login_pin=1234
stream_state=0
ota_status=idle
ipaddr=192.168.1.102
stream1_resolution=1
stream1_fps=30
stream1_bitrate=4
stream1_encoder=0
stream2_resolution=1
stream2_fps=15
stream2_bitrate=1
stream2_encoder=0
heater=0
indication_led=1
temperature=25
motion_x=0
motion_y=0
motion_z=0
EOF

    print_status $GREEN "✓ Sample source file created: $SOURCE_FILE"
    print_status $YELLOW "Please review and modify the file, then run this script again."
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -d, --dir DIR     Set config directory (default: $M5S_CONFIG_DIR)"
    echo "  -s, --source FILE Set source file (default: $SOURCE_FILE)"
    echo "  -h, --help        Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Use default settings"
    echo "  $0 -d /tmp/m5s_config                # Use custom directory"
    echo "  $0 -s my_defaults.txt                # Use custom source file"
    echo ""
}

# Function to list existing config files
list_config_files() {
    if [[ -d "$M5S_CONFIG_DIR" ]]; then
        print_status $BLUE "Existing config files in $M5S_CONFIG_DIR:"
        if ls -1 "$M5S_CONFIG_DIR"/* 2>/dev/null | head -20; then
            local count=$(ls -1 "$M5S_CONFIG_DIR"/* 2>/dev/null | wc -l)
            print_status $GREEN "Total: $count files"
        else
            print_status $YELLOW "No config files found"
        fi
    else
        print_status $YELLOW "Config directory does not exist"
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--dir)
            M5S_CONFIG_DIR="$2"
            shift 2
            ;;
        -s|--source)
            SOURCE_FILE="$2"
            shift 2
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_status $RED "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Main execution
print_status $BLUE "=== M5S Configuration Files Creator ==="
print_status $BLUE "Config directory: $M5S_CONFIG_DIR"
print_status $BLUE "Source file: $SOURCE_FILE"
echo ""

# Create config directory
create_config_directory
echo ""

# Process the source file
process_source_file "$SOURCE_FILE"
echo ""

# List existing config files
list_config_files
echo ""

print_status $GREEN "=== Script completed ===" 