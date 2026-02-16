#include <cstdio>
#include <new>
#include "motocam_network_api_l2.h"
#include "fw/fw_network.h"



uint8_t current_wifi_state;

int8_t set_WifiHotspot_l2(const uint8_t wifiHotspot_len,
                          const uint8_t *wifiHotspot) {
  printf("set_WifiHotspot_l2 wifiHotspot_len=%d\n", wifiHotspot_len);
  uint8_t ssid_len_idx = 0;
  uint8_t ssid_len = wifiHotspot[ssid_len_idx];
  uint8_t ssid_idx = (uint8_t)(ssid_len_idx + 1);
  const uint8_t *ssid = &wifiHotspot[ssid_idx];
  if (wifiHotspot_len < ssid_idx + ssid_len + 1) // 1 for val
    return -1;

  uint8_t encryption_type_idx = (uint8_t)(ssid_idx + ssid_len);
  const uint8_t encryption_type = wifiHotspot[encryption_type_idx];
  if (wifiHotspot_len < encryption_type_idx + 2) // 2---1 for len 1 for val
    return -1;

  uint8_t encryption_key_len_idx = (uint8_t)(encryption_type_idx + 1);
  uint8_t encryption_key_len = wifiHotspot[encryption_key_len_idx];
  uint8_t encryption_key_idx = (uint8_t)(encryption_key_len_idx + 1);
  const uint8_t *encryption_key = &wifiHotspot[encryption_key_idx];
  if (wifiHotspot_len <
      encryption_key_idx + encryption_key_len + 2) // 2---1 for len 1 for val
    return -1;

  uint8_t ipaddress_len_idx =
      (uint8_t)(encryption_key_idx + encryption_key_len);
  uint8_t ipaddress_len = wifiHotspot[ipaddress_len_idx];
  uint8_t ipaddress_idx = (uint8_t)(ipaddress_len_idx + 1);
  const uint8_t *ipaddress = &wifiHotspot[ipaddress_idx];
  if (wifiHotspot_len <
      ipaddress_idx + ipaddress_len + 2) // 2-- 1 for len, 1 for val
    return -1;

  uint8_t subnetmask_len_idx = ipaddress_idx + ipaddress_len;
  uint8_t subnetmask_len = wifiHotspot[subnetmask_len_idx];
  uint8_t subnetmask_idx = (uint8_t)(subnetmask_len_idx + 1);
  const uint8_t *subnetmask = &wifiHotspot[subnetmask_idx];
  if (wifiHotspot_len < subnetmask_idx + subnetmask_len)
    return -1;

  char ssid_str[32];
  uint8_t i;
  for (i = 0; i < ssid_len; i++) {
    ssid_str[i] = ssid[i];
  }
  ssid_str[ssid_len] = '\0';

  if (ssid_len > 0 && ssid_str[ssid_len - 1] == '\n') {
    ssid_str[ssid_len - 1] = '\0';
  }

  char encryption_key_str[32];
  for (i = 0; i < encryption_key_len; i++) {
    encryption_key_str[i] = encryption_key[i];
  }
  encryption_key_str[encryption_key_len] = '\0';

  char ipaddress_str[32];
  for (i = 0; i < ipaddress_len; i++) {
    ipaddress_str[i] = ipaddress[i];
  }
  ipaddress_str[ipaddress_len] = '\0';

  char subnetmask_str[32];
  for (i = 0; i < subnetmask_len; i++) {
    subnetmask_str[i] = subnetmask[i];
  }
  subnetmask_str[subnetmask_len] = '\0';
  int8_t ret =
      set_wifi_hotspot_config(ssid_str, encryption_type, encryption_key_str,
                              ipaddress_str, subnetmask_str);
  printf("set_WifiHotspot_l2 ssid=%s, encryption_type=%d, encryption_key=%s, "
         "ipaddress=%s, subnetmask=%s ret=%d\n",
         ssid_str, encryption_type, encryption_key_str, ipaddress_str,
         subnetmask_str, ret);

  if (ret < 0)
    return ret;

  current_wifi_state = 1;
  // writeState(motocam_wifi_state_file, &current_wifi_state);
  return 0;
}

int8_t set_ethernet_ip_address_l2(const uint8_t ethernet_len,
                                  const uint8_t *ethernet) {
  printf("set_ethernet_ip_address_l2 ethernet_len=%d\n", ethernet_len);
  uint8_t ip_address_len_idx = 0;
  uint8_t ip_address_len = ethernet[ip_address_len_idx];
  uint8_t ip_address_idx = (uint8_t)(ip_address_len_idx + 1);
  const uint8_t *ip_address = &ethernet[ip_address_idx];

  printf("ethernet_len :%d  ip_address_idx: %d  ip_address_len: %d\n",
         ethernet_len, ip_address_idx, ip_address_len);
  if (ethernet_len < ip_address_idx + ip_address_len + 1) // 1 for val
    return -1;

  uint8_t subnetmask_len_idx = ip_address_idx + ip_address_len;
  uint8_t subnetmask_len = ethernet[subnetmask_len_idx];
  uint8_t subnetmask_idx = (uint8_t)(subnetmask_len_idx + 1);
  const uint8_t *subnetmask = &ethernet[subnetmask_idx];
  printf("ethernet_len :%d subnetmask_idx: %d subnetmask_len: %d\n",
         ethernet_len, subnetmask_idx, subnetmask_len);
  if (ethernet_len < subnetmask_idx + subnetmask_len)
    return -1;

  uint8_t i;

  char ipaddress_str[32];
  for (i = 0; i < ip_address_len; i++) {
    ipaddress_str[i] = ip_address[i];
  }
  ipaddress_str[ip_address_len] = '\0';

  char subnetmask_str[32];
  for (i = 0; i < subnetmask_len; i++) {
    subnetmask_str[i] = subnetmask[i];
  }
  subnetmask_str[subnetmask_len] = '\0';
  int8_t ret = set_ethernet_ip_address(ipaddress_str, subnetmask_str);
  printf("set_ethernet_ip_address  ipaddress=%s, subnetmask=%s ret=%d\n",
         ipaddress_str, subnetmask_str, ret);

  if (ret < 0)
    return ret;

  // current_wifi_state = 1;
  // writeState(motocam_wifi_state_file, &current_wifi_state);
  return 0;
}

int8_t set_ethernet_dhcp_config_l2() {
  printf("set_ethernet_dhcp_config_l2\n");
  int8_t ret = set_ethernet_dhcp_config();
  printf("set_ethernet_dhcp_config_l2 ret=%d\n", ret);
  return ret;
}




uint8_t set_onvif_interface_l2(const uint8_t onvif_interface_len, const uint8_t *onvif_interface)
{
  LOG_DEBUG("set_onvif_interface with interface",onvif_interface);
  uint8_t current_onvif_interface_state;
  int8_t ret=get_onvif_interface_state(&current_onvif_interface_state);
  (void)onvif_interface_len;

  if (ret<0)
  {
    /* code */
    return ret;
  }
  LOG_DEBUG("Current onvif interface is %d",current_onvif_interface_state);
  if(current_onvif_interface_state==*onvif_interface)
  {
    LOG_ERROR("current onvif interfact state matches with onvif interface to be changed ");
    return -2;
  }

  ret=set_onvif_interface(*onvif_interface);
  return ret;
}



int8_t get_WifiHotspot_l2(uint8_t **wifiHotspot, uint8_t *length) {
  printf("get_WifiHotspot_l2\n");

  char ssid[32];
  uint8_t encryption_type;
  char encryption_key[32];
  char ipaddress[32];
  char subnetmask[32];

  int8_t ret = get_wifi_hotspot_config(ssid, &encryption_type, encryption_key,
                                       ipaddress, subnetmask);
  if (ret < 0)
    return -1;
  ssid[31]='\0';
  uint8_t ssid_len = (uint8_t)strlen(ssid);
  encryption_key[31]='\0';
  uint8_t encryption_key_len = (uint8_t)strlen(encryption_key);
  
  ipaddress[16]='\0';
  uint8_t ipaddress_len = (uint8_t)strlen(ipaddress);
  subnetmask[16]='\0';
  uint8_t subnetmask_len = (uint8_t)strlen(subnetmask);

  *length = (uint8_t)(1 + ssid_len + 1 + 1 + encryption_key_len + 1 +
                      ipaddress_len + 1 + subnetmask_len);
  *wifiHotspot = new (std::nothrow) uint8_t[*length];
  if (!*wifiHotspot) return -1;
  uint8_t wifiHotspot_idx = 0;
  (*wifiHotspot)[wifiHotspot_idx] = ssid_len;
  uint8_t i;
  for (i = 0; i < ssid_len; i++) {
    (*wifiHotspot)[++wifiHotspot_idx] = (uint8_t)ssid[i];
  }
  (*wifiHotspot)[++wifiHotspot_idx] = encryption_type;
  (*wifiHotspot)[++wifiHotspot_idx] = encryption_key_len;
  for (i = 0; i < encryption_key_len; i++) {
    (*wifiHotspot)[++wifiHotspot_idx] = (uint8_t)encryption_key[i];
  }
  (*wifiHotspot)[++wifiHotspot_idx] = ipaddress_len;
  for (i = 0; i < ipaddress_len; i++) {
    (*wifiHotspot)[++wifiHotspot_idx] = (uint8_t)ipaddress[i];
  }
  (*wifiHotspot)[++wifiHotspot_idx] = subnetmask_len;
  for (i = 0; i < subnetmask_len; i++) {
    (*wifiHotspot)[++wifiHotspot_idx] = (uint8_t)subnetmask[i];
  }
  return 0;
}

int8_t set_WifiClient_l2(const uint8_t wifiClient_len,
                         const uint8_t *wifiClient) {
  printf("set_WifiClient_l2 wifiClient_len=%d\n", wifiClient_len);
  uint8_t ssid_len_idx = 0;
  uint8_t ssid_len = wifiClient[ssid_len_idx];
  uint8_t ssid_idx = (uint8_t)(ssid_len_idx + 1);
  const uint8_t *ssid = &wifiClient[ssid_idx];
  if (wifiClient_len < ssid_idx + ssid_len + 1) // 1 for val
  {
    return -1;
  }

  uint8_t encryption_type_idx = (uint8_t)(ssid_idx + ssid_len);
  const uint8_t encryption_type = wifiClient[encryption_type_idx];
  if (wifiClient_len < encryption_type_idx + 2) // 2---1 for len 1 for val
  {
    printf("set_WifiClient_l2 2 %d\n", encryption_type_idx);
    return -1;
  }

  uint8_t encryption_key_len_idx = (uint8_t)(encryption_type_idx + 1);
  uint8_t encryption_key_len = wifiClient[encryption_key_len_idx];
  uint8_t encryption_key_idx = (uint8_t)(encryption_key_len_idx + 1);
  const uint8_t *encryption_key = &wifiClient[encryption_key_idx];
  // printf("set_WifiClient_l2 3 %d,%d\n", encryption_key, encryption_key_len);
  if (wifiClient_len <
      encryption_key_idx + encryption_key_len + 2) // 2-- 1 for len, 1 for val
  {
    // printf("set_WifiClient_l2 3 %d,%d\n", encryption_key_idx,
    // encryption_key_len);
    return -1;
  }

  char ssid_str[32];
  uint8_t i;
  for (i = 0; i < ssid_len; i++) {
    ssid_str[i] = ssid[i];
  }
  ssid_str[ssid_len] = '\0';
  if (ssid_len > 0 && ssid_str[ssid_len - 1] == '\n') {
    ssid_str[ssid_len - 1] = '\0';
  }

  char encryption_key_str[32];
  for (i = 0; i < encryption_key_len; i++) {
    encryption_key_str[i] = encryption_key[i];
  }
  encryption_key_str[encryption_key_len] = '\0';

  uint8_t ipaddress_len_idx =
      (uint8_t)(encryption_key_idx + encryption_key_len);
  uint8_t ipaddress_len = wifiClient[ipaddress_len_idx];
  if (ipaddress_len <= 0) {
    int8_t ret = set_wifi_dhcp_client_config(ssid_str, encryption_type,
                                             encryption_key_str);

     uint8_t state;
    if (get_wifi_state(&state) < 0) {
      return -1;
    }
    if (state == 2) {
      current_wifi_state = 2;
      return 0;
    }
    if (state == 1) {
      current_wifi_state = 1;
      return -2;
    }

    printf("dhcp setup fw.cpp return %d", ret);
    if (ret < 0)
      return ret;

    current_wifi_state = 2;
    return 0;
  }
  uint8_t ipaddress_idx = (uint8_t)(ipaddress_len_idx + 1);
  const uint8_t *ipaddress = &wifiClient[ipaddress_idx];
  if (wifiClient_len <
      ipaddress_idx + ipaddress_len + 2) // 2-- 1 for len, 1 for val
  {
    return -1;
  }

  uint8_t subnetmask_len_idx = ipaddress_idx + ipaddress_len;
  uint8_t subnetmask_len = wifiClient[subnetmask_len_idx];
  uint8_t subnetmask_idx = (uint8_t)(subnetmask_len_idx + 1);
  const uint8_t *subnetmask = &wifiClient[subnetmask_idx];
  printf("set_WifiClient_l2 5 %p,%d\n",(void*) subnetmask, subnetmask_len);
  if (wifiClient_len < subnetmask_idx + subnetmask_len) {
    return -1;
  }

  char ipaddress_str[32];
  for (i = 0; i < ipaddress_len; i++) {
    ipaddress_str[i] = ipaddress[i];
  }
  ipaddress_str[ipaddress_len] = '\0';

  char subnetmask_str[32];
  for (i = 0; i < subnetmask_len; i++) {
    subnetmask_str[i] = subnetmask[i];
  }
  subnetmask_str[subnetmask_len] = '\0';

  int8_t ret =
      set_wifi_client_config(ssid_str, encryption_type, encryption_key_str,
                             ipaddress_str, subnetmask_str);

  printf("set_WifiClient_l2 ssid=%s, encryption_type=%d, encryption_key=%s, "
         "ipaddress=%s, subnetmask=%s ret=%d\n",
         ssid_str, encryption_type, encryption_key_str, ipaddress_str,
         subnetmask_str, ret);
  if (ret < 0)
    return ret;

  current_wifi_state = 2;
  return 0;
}

int8_t get_WifiClient_l2(uint8_t **wifiClient, uint8_t *length) {
  printf("get_WifiClient_l2\n");
  char ssid[32];
  uint8_t encryption_type;
  char encryption_key[32];
  char ipaddress[32];
  char subnetmask[32];

  int8_t ret = get_wifi_client_config(ssid, &encryption_type, encryption_key,
                                      ipaddress, subnetmask);
  if (ret < 0)
    return -1;

  uint8_t ssid_len = (uint8_t)strlen(ssid);
  uint8_t encryption_key_len = (uint8_t)strlen(encryption_key);
  uint8_t ipaddress_len = (uint8_t)strlen(ipaddress);
  uint8_t subnetmask_len = (uint8_t)strlen(subnetmask);

  *length = (uint8_t)(1 + ssid_len + 1 + 1 + encryption_key_len + 1 +
                      ipaddress_len + 1 + subnetmask_len);
  *wifiClient = new (std::nothrow) uint8_t[*length];
  if (!*wifiClient) return -1;
  uint8_t wificlient_idx = 0;
  (*wifiClient)[wificlient_idx] = ssid_len;
  uint8_t i;
  for (i = 0; i < ssid_len; i++) {
    (*wifiClient)[++wificlient_idx] = (uint8_t)ssid[i];
  }
  (*wifiClient)[++wificlient_idx] = encryption_type;
  (*wifiClient)[++wificlient_idx] = encryption_key_len;
  for (i = 0; i < encryption_key_len; i++) {
    (*wifiClient)[++wificlient_idx] = (uint8_t)encryption_key[i];
  }
  (*wifiClient)[++wificlient_idx] = ipaddress_len;
  for (i = 0; i < ipaddress_len; i++) {
    (*wifiClient)[++wificlient_idx] = (uint8_t)ipaddress[i];
  }
  (*wifiClient)[++wificlient_idx] = subnetmask_len;
  for (i = 0; i < subnetmask_len; i++) {
    (*wifiClient)[++wificlient_idx] = (uint8_t)subnetmask[i];
  }
  return 0;
}

int8_t get_wifi_state_l2(uint8_t **wifi_state, uint8_t *length) {
  printf("get_wifi_state_l2\n");
  // enum ON_OFF ir_cutfilter;
  uint8_t state;
  if (get_wifi_state(&state) < 0) {
    return -1;
  }
  *length = 1;
  *wifi_state = new (std::nothrow) uint8_t[*length];
  if (!*wifi_state) return -1;
  printf("get_wifi_state_l2 state=%d\n", state);
  (*wifi_state)[0] = state;
  return 0;
}

int8_t get_ethernet_l2(uint8_t **ethernet, uint8_t *length) {
  printf("get_ethernet_l2\n");
  char ip_address[32];
  if (get_eth_ipaddress(ip_address) < 0) {
    return -1;
  }

  uint8_t ip_address_len = (uint8_t)strlen(ip_address);
  if (ip_address_len <= 0)
    return -1;
  printf("ip_address %s %d\n", ip_address, ip_address_len);

  *length = ip_address_len + 1;
  *ethernet = new (std::nothrow) uint8_t[*length];

  if (*ethernet == nullptr) {
    return -1;
  }

  // Fix: Use parentheses to ensure correct precedence
  (*ethernet)[0] = ip_address_len;
  for (int i = 0; i < ip_address_len; i++) {
    (*ethernet)[i + 1] = (uint8_t)ip_address[i]; // Also simplified the indexing
  }

  return 0;
}

int8_t get_onvif_interface_state_l2(uint8_t **interface,uint8_t *length){

  printf("get_onvif_interface_state_l2\n");
  uint8_t onvif_interface;
  int8_t ret=get_onvif_interface_state(&onvif_interface); 
  if(ret<0){
    return ret;
  }
  printf("interface value=%d",onvif_interface);

  *length=1;
  **interface=onvif_interface;
  return 0;

}
