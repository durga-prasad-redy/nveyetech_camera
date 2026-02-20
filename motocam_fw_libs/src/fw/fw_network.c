#include "fw/fw_network.h"
#include "fw/fw_state_machine.h"
#define SET_WIFI "/mnt/flash/vienna/scripts/network/start_wifi.sh"
#define SET_ETHERNET_IPADDRESS                                                 \
  "/mnt/flash/vienna/motocam/network/outdu_set_eth0_ip.sh"
#define SET_ONVIF_INTERFACE_STATE "/mnt/flash/vienna/m5s_config/onvif_itrf"
#define SET_ONVIF_INTERFACE_ETHERNET                                           \
  "/mnt/flash/vienna/scripts/onvif/onvif_update.sh eth0"
#define SET_ONVIF_INTERFACE_WIFI                                               \
  "/mnt/flash/vienna/scripts/onvif/onvif_update.sh wlan0"
// Wifi Hotspot
#define GET_WIFI_HOTSPOT_SSID "cat " M5S_CONFIG_DIR "/hotspot_ssid"
#define GET_WIFI_HOTSPOT_PASSWORD "cat " M5S_CONFIG_DIR "/hotspot_password"
#define GET_WIFI_HOTSPOT_IPADDRESS "cat " M5S_CONFIG_DIR "/hotspot_ipaddress"
#define GET_WIFI_HOTSPOT_SUBNETMASK "cat " M5S_CONFIG_DIR "/hotspot_subnetmask"
#define GET_WIFI_HOTSPOT_ENCRYPTION_TYPE                                       \
  "cat " M5S_CONFIG_DIR "/hotspot_encryption_type"
#define GET_WIFI_HOTSPOT_ENCRYPTION_KEY                                        \
  "cat " M5S_CONFIG_DIR "/hotspot_encryption_key"
// Wifi Client
#define GET_WIFI_CLIENT_SSID "cat " M5S_CONFIG_DIR "/client_ssid"
#define GET_WIFI_CLIENT_PASSWORD "cat " M5S_CONFIG_DIR "/client_password"
#define GET_WIFI_CLIENT_IPADDRESS "cat " M5S_CONFIG_DIR "/client_ipaddress"
#define GET_WIFI_CLIENT_SUBNETMASK "cat " M5S_CONFIG_DIR "/client_subnetmask"
#define GET_WIFI_CLIENT_ENCRYPTION_TYPE                                        \
  "cat " M5S_CONFIG_DIR "/client_encryption_type"
#define GET_WIFI_CLIENT_ENCRYPTION_KEY                                         \
  "cat " M5S_CONFIG_DIR "/client_encryption_key"
#define GET_WIFI_STATE                                                         \
  "cat " M5S_CONFIG_DIR "/wifi_state" // 1:Hotspot, 2:Client
// #define GET_ONVIF_INTERFACE_STATE "cat " M5S_CONFIG_DIR "/onvif_itrf"
#define ONVIF_INTERFACE_STATE_FILE M5S_CONFIG_DIR "/onvif_itrf"

// Eth
#define GET_ETHERNET_IPADDRESS "cat " M5S_CONFIG_DIR "/current_eth_ip"

static int check_wifi_status_with_retry(void);

int8_t get_wifi_hotspot_config(char *ssid, uint8_t *encryption_type,
                               char *encryption_key, char *ip_address,
                               char *subnetmask) {
  char output_ssid[64];
  char output_encryption_type[64];
  char output_encryption_key[64];
  char output_ip_address[64];
  char output_subnetmask[64];

  pthread_mutex_lock(&lock);

  if (exec_cmd(GET_WIFI_HOTSPOT_SSID, output_ssid, sizeof(output_ssid)) == 0) {
    snprintf(ssid, 32, "%s", output_ssid);
  }

  if (exec_cmd(GET_WIFI_HOTSPOT_ENCRYPTION_TYPE, output_encryption_type,
               sizeof(output_encryption_type)) == 0) {
    *encryption_type = (uint8_t)atoi(output_encryption_type);
  }

  if (exec_cmd(GET_WIFI_HOTSPOT_ENCRYPTION_KEY, output_encryption_key,
               sizeof(output_encryption_key)) == 0) {
    snprintf(encryption_key, 32, "%s", output_encryption_key);
  }

  if (exec_cmd(GET_WIFI_HOTSPOT_IPADDRESS, output_ip_address,
               sizeof(output_ip_address)) == 0) {
    snprintf(ip_address, 16, "%s", output_ip_address);
  }

  if (exec_cmd(GET_WIFI_HOTSPOT_SUBNETMASK, output_subnetmask,
               sizeof(output_subnetmask)) == 0) {
    snprintf(subnetmask, 16, "%s", output_subnetmask);
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_wifi_client_config(const char *ssid, const uint8_t encryption_type,
                              const char *encryption_key,
                              const char *ip_address, const char *subnetmask) {

  printf("set_WifiClient_l2 ssid=%s, encryption_type=%d, encryption_key=%s, "
         "ipaddress=%s, subnetmask=%s \n",
         ssid, encryption_type, encryption_key, ip_address, subnetmask);

  pthread_mutex_lock(&lock);
  set_uboot_env("wifi_runtime_result", 3); // 3 means in progress

  char cmd[500];
  sprintf(cmd, "%s runtime switch_to_device %s %s %s", SET_WIFI, ssid,
          encryption_key, ip_address);

  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);
  if (check_wifi_status_with_retry() != 1) {
    printf("WiFi Client mode setup failed\n");
    pthread_mutex_unlock(&lock);

    return -1;
  }

  set_uboot_env_chars("client_ssid", ssid);
  set_uboot_env_chars("client_encryption_key", encryption_key);
  set_uboot_env_chars("client_ipaddress", ip_address);
  set_uboot_env_chars("client_subnetmask", subnetmask);

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_wifi_dhcp_client_config(const char *ssid,
                                   const uint8_t encryption_type,
                                   const char *encryption_key) {

  printf("set_WifiClient_l2 ssid=%s, encryption_type=%d, encryption_key=%s \n",
         ssid, encryption_type, encryption_key);
  pthread_mutex_lock(&lock);
  set_uboot_env("wifi_runtime_result", 3); // 3 means in progress

  char cmd[500];
  sprintf(cmd, "%s runtime switch_to_device %s %s", SET_WIFI, ssid,
          encryption_key);
  printf("dhcp  command:%s\n", cmd);

  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  printf("dhcp  command:%s\n", cmd);
  system(background_cmd);
  if (check_wifi_status_with_retry() != 1) {
    printf("WiFi AP mode setup failed\n");
    pthread_mutex_unlock(&lock);

    return -1;
  }
  set_uboot_env_chars("client_ssid", ssid);
  set_uboot_env_chars("client_encryption_key", encryption_key);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_wifi_client_config(char *ssid, uint8_t *encryption_type,
                              char *encryption_key, char *ip_address,
                              char *subnetmask) {
  char output_ssid[64];
  char output_encryption_type[64];
  char output_encryption_key[64];
  char output_ip_address[64];
  char output_subnetmask[64];

  pthread_mutex_lock(&lock);

  if (exec_cmd(GET_WIFI_CLIENT_SSID, output_ssid, sizeof(output_ssid)) == 0) {
    snprintf(ssid, 32, "%s", output_ssid);
  }

  if (exec_cmd(GET_WIFI_CLIENT_ENCRYPTION_TYPE, output_encryption_type,
               sizeof(output_encryption_type)) == 0) {
    *encryption_type = (uint8_t)atoi(output_encryption_type);
  }

  if (exec_cmd(GET_WIFI_CLIENT_ENCRYPTION_KEY, output_encryption_key,
               sizeof(output_encryption_key)) == 0) {
    snprintf(encryption_key, 32, "%s", output_encryption_key);
  }

  if (exec_cmd(GET_WIFI_CLIENT_IPADDRESS, output_ip_address,
               sizeof(output_ip_address)) == 0) {
    snprintf(ip_address, 16, "%s", output_ip_address);
  }

  if (exec_cmd(GET_WIFI_CLIENT_SUBNETMASK, output_subnetmask,
               sizeof(output_subnetmask)) == 0) {
    snprintf(subnetmask, 16, "%s", output_subnetmask);
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_wifi_state(uint8_t *state) {
  char output[64];

  pthread_mutex_lock(&lock);

  if (exec_cmd(GET_WIFI_STATE, output, sizeof(output)) == 0) {
    printf("get_wifi_state %s\n", output);
    *state = (uint8_t)atoi(output);
  }

  pthread_mutex_unlock(&lock);

  printf("get_wifi_state %d\n", state[0]);
  return 0;
}

int8_t get_onvif_interface_state(uint8_t *interface) {
  char cmd[500];
  char output[64];

  pthread_mutex_lock(&lock);

  snprintf(cmd, sizeof(cmd), "cat %s", ONVIF_INTERFACE_STATE_FILE);

  if (exec_cmd(cmd, output, sizeof(output)) != 0) {
    pthread_mutex_unlock(&lock);
    return -1;
  }

  /* trim trailing \n / \r (handle \r\n too) */
  size_t len = strlen(output);
  while (len > 0 && (output[len - 1] == '\n' || output[len - 1] == '\r')) {
    output[--len] = '\0';
  }

  LOG_DEBUG("fw onvif interface state %s\n", output);

  if (strcmp(output, "eth") == 0) {
    LOG_DEBUG("changing onvif interface to eth");
    *interface = 0;
  } else if (strcmp(output, "wifi") == 0) {
    LOG_DEBUG("changing onvif interface to wifi");
    *interface = 1;
  } else {
    LOG_DEBUG("invalid onvif interface option %d", *interface);
    pthread_mutex_unlock(&lock);
    return -1;
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_wifi_hotspot_ipaddress(char *ip_address) {
  char output[64];

  pthread_mutex_lock(&lock);

  if (exec_cmd(GET_WIFI_HOTSPOT_IPADDRESS, output, sizeof(output)) == 0) {
    /* trim trailing newline */
    size_t len = strlen(output);
    if (len > 0 && output[len - 1] == '\n')
      output[len - 1] = '\0';

    snprintf(ip_address, 16, "%s", output);
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_wifi_client_ipaddress(char *ip_address) {
  char output[64];

  pthread_mutex_lock(&lock);

  if (exec_cmd(GET_WIFI_CLIENT_IPADDRESS, output, sizeof(output)) == 0) {
    /* trim trailing newline */
    size_t len = strlen(output);
    if (len > 0 && output[len - 1] == '\n')
      output[len - 1] = '\0';

    snprintf(ip_address, 16, "%s", output);
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_eth_ipaddress(char *ip_address) {
  char output[64];

  pthread_mutex_lock(&lock);

  if (exec_cmd(GET_ETHERNET_IPADDRESS, output, sizeof(output)) == 0) {
    /* trim trailing newline */
    size_t len = strlen(output);
    if (len > 0 && output[len - 1] == '\n')
      output[len - 1] = '\0';

    snprintf(ip_address, 16, "%s", output);
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ethernet_ip_address(const char *ip_address, const char *subnetmask) {
  char cmd[500];
  char dummy[8];

  printf("fw set_ethernet_ip_address %s\n", ip_address);

  pthread_mutex_lock(&lock);

  snprintf(cmd, sizeof(cmd), "%s %s ", SET_ETHERNET_IPADDRESS, ip_address);

  (void)subnetmask; /* unused, kept for API compatibility */
  exec_cmd(cmd, dummy, sizeof(dummy));

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ethernet_dhcp_config() {

  pthread_mutex_lock(&lock);
  char cmd[500];
  sprintf(cmd, "%s", SET_ETHERNET_IPADDRESS);
  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_onvif_interface(const uint8_t interface) {
  LOG_DEBUG("fw set onvif interface %d\n", interface);

  if (interface != 0 && interface != 1) {
    LOG_ERROR("fw unsupported onvif interface option %d", interface);
    return -4;
  }

  pthread_mutex_lock(&lock);

  onvif_sm_ctx_t sm;
  onvif_sm_init(&sm, interface);

  fw_sm_status_t status;
  do {
    status = onvif_sm_step(&sm);
    if (status == FW_SM_RUNNING)
      fw_sm_yield();
  } while (status == FW_SM_RUNNING);

  pthread_mutex_unlock(&lock);

  if (status == FW_SM_DONE_FAIL)
    return -3;

  return 0;
}

int8_t set_wifi_hotspot_config(const char *ssid, const uint8_t encryption_type,
                               const char *encryption_key,
                               const char *ip_address, const char *subnetmask) {

  printf("fw set_wifi_hotspot %s %s\n", ssid, ip_address);
  pthread_mutex_lock(&lock);

  set_uboot_env("wifi_runtime_result", 3); // 3 means in progress

  char cmd[500];
  (void)encryption_type;
  sprintf(cmd, "%s runtime switch_to_ap %s %s %s", SET_WIFI, ssid,
          encryption_key, ip_address);

  printf("------------------ssid %s\n", ssid);

  printf("---------cmd: %s\n", cmd);

  set_uboot_env_chars("hotspot_ssid", ssid);
  set_uboot_env_chars("hotspot_encryption_key", encryption_key);
  set_uboot_env_chars("hotspot_ipaddress", ip_address);
  set_uboot_env_chars("hotspot_subnetmask", subnetmask);

  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);
  if (check_wifi_status_with_retry() != 1) {
    printf("WiFi AP mode setup failed\n");
    pthread_mutex_unlock(&lock);

    return -1;
  }
  pthread_mutex_unlock(&lock);
  return 0;
}

int read_wifi_status_once(void) {
  char status[256];

  FILE *file = fopen(WIFI_RUNTIME_RESULT, "r");
  if (!file) {
    printf("[ERROR] Failed to open file: %s\n", WIFI_RUNTIME_RESULT);
    return 0;
  }

  if (!fgets(status, sizeof(status), file)) {
    printf("[WARN] File is empty or read failed.\n");
    fclose(file);
    return 0;
  }

  fclose(file);

  size_t len = strcspn(status, "\n");
  if (len < sizeof(status)) {
    status[len] = '\0';
  }
  printf("[DEBUG] Read status: '%s'\n", status);

  if (strcmp(status, "0") == 0) {
    printf("[INFO] Wi-Fi status indicates success.\n");
    return 1;
  }

  printf("[INFO] Wi-Fi status not ready yet: '%s'\n", status);
  return 0;
}

static int check_wifi_status_with_retry(void) {
  wifi_sm_ctx_t sm;
  wifi_sm_init(&sm, 20);

  fw_sm_status_t status;
  do {
    status = wifi_sm_step(&sm);
    if (status == FW_SM_RUNNING)
      fw_sm_yield();
  } while (status == FW_SM_RUNNING);

  return (status == FW_SM_DONE_OK) ? 1 : 0;
}

// TBD
#if 0

typedef enum {
    ONVIF_ITRF_SM_IDLE,
    ONVIF_ITRF_SM_POLLING,
    ONVIF_ITRF_SM_DONE_OK,
    ONVIF_ITRF_SM_DONE_FAIL
} onvif_itrf_sm_state_t;

typedef struct {
    onvif_itrf_sm_state_t state;
    fw_sm_timer_t         timer;
    const uint8_t        *expected;
    int                   retries;
} onvif_itrf_sm_ctx_t;

static int read_onvif_interface_once(const uint8_t *expected)
{
    char status[256];
    FILE *file = fopen(ONVIF_INTERFACE_STATE_FILE, "r");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s\n", ONVIF_INTERFACE_STATE_FILE);
        return -1;
    }

    if (fgets(status, sizeof(status), file) == NULL)
    {
        LOG_DEBUG("File is empty or read failed.\n");
        fclose(file);
        return -1;
    }
    fclose(file);

    /* Trim trailing newline / carriage return */
    size_t len = strlen(status);
    while (len > 0 && (status[len - 1] == '\n' || status[len - 1] == '\r'))
        status[--len] = '\0';

    LOG_DEBUG("Read status: '%s'\n", status);

    uint8_t current;
    if (strcmp(status, "eth") == 0)
        current = 0;
    else if (strcmp(status, "wifi") == 0)
        current = 1;
    else
    {
        LOG_DEBUG("Invalid interface status: '%s'\n", status);
        return 0;
    }

    LOG_DEBUG("current_onvif_interface: %d, expected: %d\n",
              current, *expected);
    return (current == *expected) ? 1 : 0;
}

static int check_onvif_interface_status(const uint8_t *onvif_interface)
{
    onvif_itrf_sm_ctx_t sm;
    sm.state    = ONVIF_ITRF_SM_IDLE;
    sm.expected = onvif_interface;
    sm.retries  = 0;

    LOG_DEBUG("checking onvif interface change status with 20 retries...\n");

    while (1)
    {
        switch (sm.state)
        {
        case ONVIF_ITRF_SM_IDLE:
        {
            int r = read_onvif_interface_once(sm.expected);
            if (r == 1)
                return 1;
            sm.retries = 1;
            fw_sm_timer_start(&sm.timer, 1000);
            sm.state = ONVIF_ITRF_SM_POLLING;
            break;
        }
        case ONVIF_ITRF_SM_POLLING:
            if (!fw_sm_timer_expired(&sm.timer))
            {
                fw_sm_yield();
                break;
            }
            LOG_DEBUG("Attempt %d...\n", sm.retries);
            {
                int r = read_onvif_interface_once(sm.expected);
                if (r == 1)
                    return 1;
            }
            sm.retries++;
            if (sm.retries > 20)
            {
                LOG_ERROR("Onvif interface change status check failed after all retries.\n");
                return 0;
            }
            fw_sm_timer_start(&sm.timer, 1000);
            break;
        default:
            return 0;
        }
    }
}
#endif
