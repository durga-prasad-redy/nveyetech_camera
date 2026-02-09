#include "fw/fw_network.h"
#define SET_WIFI "/mnt/flash/vienna/scripts/network/start_wifi.sh"
#define SET_ETHERNET_IPADDRESS \
  "/mnt/flash/vienna/motocam/network/outdu_set_eth0_ip.sh"
#define SET_ONVIF_INTERFACE_STATE "/mnt/flash/vienna/m5s_config/onvif_itrf"
#define SET_ONVIF_INTERFACE_ETHERNET "/mnt/flash/vienna/scripts/onvif/onvif_update.sh eth0"
#define SET_ONVIF_INTERFACE_WIFI "/mnt/flash/vienna/scripts/onvif/onvif_update.sh wlan0"
// Wifi Hotspot
#define GET_WIFI_HOTSPOT_SSID "cat " M5S_CONFIG_DIR "/hotspot_ssid"
#define GET_WIFI_HOTSPOT_PASSWORD "cat " M5S_CONFIG_DIR "/hotspot_password"
#define GET_WIFI_HOTSPOT_IPADDRESS "cat " M5S_CONFIG_DIR "/hotspot_ipaddress"
#define GET_WIFI_HOTSPOT_SUBNETMASK "cat " M5S_CONFIG_DIR "/hotspot_subnetmask"
#define GET_WIFI_HOTSPOT_ENCRYPTION_TYPE \
  "cat " M5S_CONFIG_DIR "/hotspot_encryption_type"
#define GET_WIFI_HOTSPOT_ENCRYPTION_KEY \
  "cat " M5S_CONFIG_DIR "/hotspot_encryption_key"
// Wifi Client
#define GET_WIFI_CLIENT_SSID "cat " M5S_CONFIG_DIR "/client_ssid"
#define GET_WIFI_CLIENT_PASSWORD "cat " M5S_CONFIG_DIR "/client_password"
#define GET_WIFI_CLIENT_IPADDRESS "cat " M5S_CONFIG_DIR "/client_ipaddress"
#define GET_WIFI_CLIENT_SUBNETMASK "cat " M5S_CONFIG_DIR "/client_subnetmask"
#define GET_WIFI_CLIENT_ENCRYPTION_TYPE \
  "cat " M5S_CONFIG_DIR "/client_encryption_type"
#define GET_WIFI_CLIENT_ENCRYPTION_KEY \
  "cat " M5S_CONFIG_DIR "/client_encryption_key"
#define GET_WIFI_STATE \
  "cat " M5S_CONFIG_DIR "/wifi_state" // 1:Hotspot, 2:Client
// #define GET_ONVIF_INTERFACE_STATE "cat " M5S_CONFIG_DIR "/onvif_itrf"
#define ONVIF_INTERFACE_STATE_FILE M5S_CONFIG_DIR "/onvif_itrf"

// Eth
#define GET_ETHERNET_IPADDRESS "cat " M5S_CONFIG_DIR "/current_eth_ip"

static int check_wifi_status_with_retry(void);

int8_t get_wifi_hotspot_config(char *ssid, uint8_t *encryption_type,
                               char *encryption_key, char *ip_address,
                               char *subnetmask)
{
  pthread_mutex_lock(&lock);
  std::string output_ssid = exec(GET_WIFI_HOTSPOT_SSID);
  std::string output_encryption_type = exec(GET_WIFI_HOTSPOT_ENCRYPTION_TYPE);
  std::string output_encryption_key = exec(GET_WIFI_HOTSPOT_ENCRYPTION_KEY);
  std::string output_ip_address = exec(GET_WIFI_HOTSPOT_IPADDRESS);
  std::string output_subnetmask = exec(GET_WIFI_HOTSPOT_SUBNETMASK);

snprintf(ssid, 32, "%s", output_ssid.c_str());
*encryption_type = (uint8_t)atoi(output_encryption_type.c_str());
  snprintf(encryption_key, 32, "%s", output_encryption_key.c_str());
  snprintf(ip_address, 16, "%s", output_ip_address.c_str());
  snprintf(subnetmask, 16, "%s", output_subnetmask.c_str());

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_wifi_client_config(const char *ssid, const uint8_t encryption_type,
                              const char *encryption_key,
                              const char *ip_address, const char *subnetmask)
{

  printf("set_WifiClient_l2 ssid=%s, encryption_type=%d, encryption_key=%s, "
         "ipaddress=%s, subnetmask=%s \n",
         ssid, encryption_type, encryption_key, ip_address, subnetmask);

  pthread_mutex_lock(&lock);
  set_uboot_env("wifi_runtime_result", 3); // 3 means in progress

  char cmd[500];
  sprintf(cmd, "%s runtime switch_to_device %s %s %s", SET_WIFI, ssid,
          encryption_key, ip_address);
  // exec(cmd);
  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);
  if (check_wifi_status_with_retry() != 1)
  {
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
                                   const char *encryption_key)
{

  printf("set_WifiClient_l2 ssid=%s, encryption_type=%d, encryption_key=%s \n",
         ssid, encryption_type, encryption_key);
  pthread_mutex_lock(&lock);
  set_uboot_env("wifi_runtime_result", 3); // 3 means in progress

  char cmd[500];
  sprintf(cmd, "%s runtime switch_to_device %s %s", SET_WIFI, ssid,
          encryption_key);
  printf("dhcp  command:%s\n", cmd);
  // exec(cmd);
  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  printf("dhcp  command:%s\n", cmd);
  system(background_cmd);
  if (check_wifi_status_with_retry() != 1)
  {
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
                              char *subnetmask)
{
  pthread_mutex_lock(&lock);
  std::string output_ssid = exec(GET_WIFI_CLIENT_SSID);
  std::string output_encryption_type = exec(GET_WIFI_CLIENT_ENCRYPTION_TYPE);
  std::string output_encryption_key = exec(GET_WIFI_CLIENT_ENCRYPTION_KEY);
  std::string output_ip_address = exec(GET_WIFI_CLIENT_IPADDRESS);
  std::string output_subnetmask = exec(GET_WIFI_CLIENT_SUBNETMASK);

snprintf(ssid, 32, "%s", output_ssid.c_str());
*encryption_type = (uint8_t)atoi(output_encryption_type.c_str());
  snprintf(encryption_key, 32, "%s", output_encryption_key.c_str());
  snprintf(ip_address, 16, "%s", output_ip_address.c_str());
  snprintf(subnetmask, 16, "%s", output_subnetmask.c_str());

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_wifi_state(uint8_t *state)
{
  // uint8_t *state;
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_WIFI_STATE);
  printf("get_wifi_state %s\n", output.c_str());

  *state = (uint8_t)atoi(output.c_str());
  pthread_mutex_unlock(&lock);
  printf("get_wifi_state %d\n", state[0]);
  return 0;
}

int8_t get_onvif_interface_state(uint8_t *interface)
{

  char cmd[500];
  pthread_mutex_lock(&lock);
  sprintf(cmd, "cat %s", ONVIF_INTERFACE_STATE_FILE);
  std::string output = exec(cmd);

  if (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
    output.pop_back();
  if (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
    output.pop_back(); // handle both \r\n cases

  LOG_DEBUG("fw onvif interface state %s\n", output.c_str());

  if (strcmp(output.c_str(), "eth") == 0)
  {

    LOG_DEBUG("changing onvif interface to eth");
    *interface = 0;
  }
  else if (strcmp(output.c_str(), "wifi") == 0)
  {
    LOG_DEBUG("changing onvif interface to wifi");
    *interface = 1;
  }
  else
  {
    LOG_DEBUG("invalid onvif interface option %d", *interface);
    pthread_mutex_unlock(&lock);
    return -1;
  }

  // LOG_DEBUG("changing onvif interface to wifalsdfjasl;dfji");

  pthread_mutex_unlock(&lock);
  return 0;
}


int8_t get_wifi_hotspot_ipaddress(char *ip_address)
{
  pthread_mutex_lock(&lock);
  std::string output_ip_address = exec(GET_WIFI_HOTSPOT_IPADDRESS);
  if (!output_ip_address.empty() && output_ip_address.back() == '\n')
  {
    output_ip_address.pop_back();
  }
  snprintf(ip_address, 16, "%s", output_ip_address.c_str());
  
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_wifi_client_ipaddress(char *ip_address)
{
  pthread_mutex_lock(&lock);
  std::string output_ip_address = exec(GET_WIFI_CLIENT_IPADDRESS);
  if (!output_ip_address.empty() && output_ip_address.back() == '\n')
  {
    output_ip_address.pop_back();
  }
  snprintf(ip_address, 16, "%s", output_ip_address.c_str());
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_eth_ipaddress(char *ip_address)
{
  pthread_mutex_lock(&lock);
  std::string output_ip_address = exec(GET_ETHERNET_IPADDRESS);
  if (!output_ip_address.empty() && output_ip_address.back() == '\n')
  {
    output_ip_address.pop_back();
  }

  snprintf(ip_address, 16, "%s", output_ip_address.c_str());

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ethernet_ip_address(const char *ip_address, const char *subnetmask)
{

  printf("fw set_ethernet_ip_address %s\n", ip_address);
  pthread_mutex_lock(&lock);
  char cmd[500];
  sprintf(cmd, "%s %s ", SET_ETHERNET_IPADDRESS, ip_address);
  (void)subnetmask;
  exec(cmd);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ethernet_dhcp_config()
{

  pthread_mutex_lock(&lock);
  char cmd[500];
  sprintf(cmd, "%s", SET_ETHERNET_IPADDRESS);
  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);
  // exec(cmd);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_onvif_interface(const uint8_t interface)
{
  LOG_DEBUG("fw set onvif interface %d\n", interface);
  pthread_mutex_lock(&lock);
  if (interface == 0)
  {
    // exec(SET_ONVIF_INTERFACE_ETHERNET);

    if (is_running(ONVIF_SERVER_PROCESS_NAME))
    {
      stop_process(ONVIF_SERVER_PROCESS_NAME);
      sleep(3);
    }
    set_uboot_env_chars("onvif_itrf", "eth");
    char background_cmd[600];
    snprintf(background_cmd, sizeof(background_cmd), "%s < /dev/null > /dev/null 2>&1 &", SET_ONVIF_INTERFACE_ETHERNET);
    system(background_cmd);
    /* if (check_onvif_interface_status(&interface) != 1)
    {
      LOG_DEBUG("Onvif interface change to ethernet failed\n");
      pthread_mutex_unlock(&lock);

      return -3;
    }
     */

    uint8_t count_onvif_server_process = 0;

    while (!is_running(ONVIF_SERVER_PROCESS_NAME))
    {
      LOG_DEBUG("waiting for onvif server to start with count:%d", count_onvif_server_process);
      if (count_onvif_server_process > 10)
      {
        break;
      }
      sleep(1);
      count_onvif_server_process++;
    }
    if (count_onvif_server_process > 10)
    {
      LOG_ERROR("NOT ABLE TO RESTART ONVIF SERVER");
      return -3;
    }
  }
  else if (interface == 1)
  {

    if (is_running(ONVIF_SERVER_PROCESS_NAME))
    {
      stop_process(ONVIF_SERVER_PROCESS_NAME);
      sleep(3);
    }

    // exec(SET_ONVIF_INTERFACE_WIFI);
    set_uboot_env_chars("onvif_itrf", "wifi");
    char background_cmd[600];
    snprintf(background_cmd, sizeof(background_cmd), "%s < /dev/null > /dev/null 2>&1 &", SET_ONVIF_INTERFACE_WIFI);
    system(background_cmd);
    /*  if (check_onvif_interface_status(&interface) != 1)
     {
       LOG_DEBUG("Onvif interface change to wifi failed\n");
       pthread_mutex_unlock(&lock);

       return -3;
     } */

    uint8_t count_onvif_server_process = 0;

    while (!is_running(ONVIF_SERVER_PROCESS_NAME))
    {
      LOG_DEBUG("waiting for onvif server to start with count:%d", count_onvif_server_process);
      if (count_onvif_server_process > 10)
      {
        break;
      }
      sleep(1);
      count_onvif_server_process++;
    }
    if (count_onvif_server_process > 10)
    {
      LOG_ERROR("NOT ABLE TO RESTART ONVIF SERVER");
      return -3;
    }
  }

  else
  {
    LOG_ERROR("fw unsupported onvif interface option %d", interface);
    pthread_mutex_unlock(&lock);
    return -4;
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_wifi_hotspot_config(const char *ssid, const uint8_t encryption_type,
                               const char *encryption_key,
                               const char *ip_address, const char *subnetmask)
{

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
  // exec(cmd);

  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);
  if (check_wifi_status_with_retry() != 1)
  {
    printf("WiFi AP mode setup failed\n");
    pthread_mutex_unlock(&lock);

    return -1;
  }
  pthread_mutex_unlock(&lock);
  return 0;
}


static int check_wifi_status_with_retry(void)
{
  int retries = 20;
  char status[256];

  printf("[INFO] Checking Wi-Fi status with %d retries...\n", retries);

  while (retries > 0)
  {
    printf("[DEBUG] Attempt %d...\n", 11 - retries);

    FILE *file = fopen(WIFI_RUNTIME_RESULT, "r");
    if (file)
    {
      printf("[DEBUG] Opened file: %s\n", WIFI_RUNTIME_RESULT);

      if (fgets(status, sizeof(status), file) != NULL)
      {
        // Remove trailing newline if present
        status[strcspn(status, "\n")] = 0;
        printf("[DEBUG] Read status: '%s'\n", status);

        fclose(file);

        // Check if status is empty (I think you intended strcmp(status, "") instead of 0)
        if (strcmp(status, "0") == 0)
        {
          printf("[INFO] Wi-Fi status indicates success.\n");
          return 1; // Success
        }
        else
        {
          printf("[INFO] Wi-Fi status is not empty: '%s'\n", status);
        }
      }
      else
      {
        printf("[WARN] File is empty or read failed.\n");
        fclose(file);
      }
    }
    else
    {
      printf("[ERROR] Failed to open file: %s\n", WIFI_RUNTIME_RESULT);
    }

    sleep(1); // Wait for 1 second before retry
    retries--;
  }

  printf("[ERROR] Wi-Fi status check failed after all retries.\n");
  return 0; // Failure after all retries
}

// TBD
#if 0
static int check_onvif_interface_status(const uint8_t *onvif_interface)
{
  int retries = 20;
  char status[256];

  LOG_DEBUG("checking onvif interface change status with %d retries...\n", retries);
  while (retries > 0)
  {
    LOG_DEBUG("Attempt %d...\n", 11 - retries);

    FILE *file = fopen(ONVIF_INTERFACE_STATE_FILE, "r");
    if (file)
    {
      LOG_DEBUG("Opened file: %s\n", ONVIF_INTERFACE_STATE_FILE);

      if (fgets(status, sizeof(status), file) != NULL)
      {
        // Remove trailing newline or carriage return if present
        size_t len = strlen(status);
        if (len > 0 && (status[len - 1] == '\n' || status[len - 1] == '\r'))
          status[len - 1] = '\0';
        len = strlen(status);
        if (len > 0 && (status[len - 1] == '\n' || status[len - 1] == '\r'))
          status[len - 1] = '\0';

        LOG_DEBUG("Read status: '%s'\n", status);

        fclose(file);

        uint8_t current_onvif_interface;
        if (strcmp(status, "eth") == 0)
        {
          current_onvif_interface = 0;
        }
        else if (strcmp(status, "wifi") == 0)
        {
          current_onvif_interface = 1;
        }
        else
        {
          LOG_DEBUG("Invalid interface status: '%s'\n", status);
          retries--;
          sleep(1);
          continue;
        }

        LOG_DEBUG("current_onvif_interface: %d, expected: %d\n", current_onvif_interface, *onvif_interface);

        if (current_onvif_interface == *onvif_interface)
        {
          LOG_DEBUG("Onvif interface change indicates success.\n");
          return 1; // Success
        }
      }
      else
      {
        LOG_DEBUG("File is empty or read failed.\n");
        fclose(file);
      }
    }
    else
    {
      LOG_ERROR("Failed to open file: %s\n", ONVIF_INTERFACE_STATE_FILE);
    }

    sleep(1); // Wait 1 second before retry
    retries--;
  }

  LOG_ERROR("Onvif interface change status check failed after all retries.\n");
  return 0; // Failure after all retries
}
#endif
