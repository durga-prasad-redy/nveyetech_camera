#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include "motocam_network_api_l2.h"
#include "fw/fw_network.h"



static uint8_t current_wifi_state;

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

  auto ssid_str = std::string(ssid, ssid + ssid_len);
  if (ssid_len > 0 && ssid_str[ssid_len - 1] == '\n') {
    ssid_str.resize(ssid_len - 1);
  }

  auto encryption_key_str = std::string(encryption_key, encryption_key + encryption_key_len);
  auto ipaddress_str = std::string(ipaddress, ipaddress + ipaddress_len);
  auto subnetmask_str = std::string(subnetmask, subnetmask + subnetmask_len);

  auto ret =
      set_wifi_hotspot_config(ssid_str.c_str(), encryption_type, encryption_key_str.c_str(),
                              ipaddress_str.c_str(), subnetmask_str.c_str());
  printf("set_WifiHotspot_l2 ssid=%s, encryption_type=%d, encryption_key=%s, "
         "ipaddress=%s, subnetmask=%s ret=%d\n",
         ssid_str.c_str(), encryption_type, encryption_key_str.c_str(), ipaddress_str.c_str(),
         subnetmask_str.c_str(), ret);

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

  auto ipaddress_str = std::string(ip_address, ip_address + ip_address_len);
  auto subnetmask_str = std::string(subnetmask, subnetmask + subnetmask_len);
  auto ret = set_ethernet_ip_address(ipaddress_str.c_str(), subnetmask_str.c_str());
  printf("set_ethernet_ip_address  ipaddress=%s, subnetmask=%s ret=%d\n",
         ipaddress_str.c_str(), subnetmask_str.c_str(), ret);

  if (ret < 0)
    return ret;

  // current_wifi_state = 1;
  // writeState(motocam_wifi_state_file, &current_wifi_state);
  return 0;
}

int8_t set_ethernet_dhcp_config_l2() {
  printf("set_ethernet_dhcp_config_l2\n");
  auto ret = set_ethernet_dhcp_config();
  printf("set_ethernet_dhcp_config_l2 ret=%d\n", ret);
  return ret;
}




uint8_t set_onvif_interface_l2(const uint8_t onvif_interface_len, const uint8_t *onvif_interface)
{
  LOG_DEBUG("set_onvif_interface with interface",onvif_interface);
  uint8_t current_onvif_interface_state;
  auto ret=get_onvif_interface_state(&current_onvif_interface_state);
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

  std::string ssid(32, '\0');
  uint8_t encryption_type;
  std::string encryption_key(32, '\0');
  std::string ipaddress(32, '\0');
  std::string subnetmask(32, '\0');

  int8_t ret = get_wifi_hotspot_config(&ssid[0], &encryption_type, &encryption_key[0],
                                       &ipaddress[0], &subnetmask[0]);
  if (ret < 0)
    return -1;
  ssid.resize(strlen(ssid.c_str()));
  auto ssid_len = (uint8_t)ssid.size();
  encryption_key.resize(strlen(encryption_key.c_str()));
  auto encryption_key_len = (uint8_t)encryption_key.size();
  ipaddress.resize(strlen(ipaddress.c_str()));
  auto ipaddress_len = (uint8_t)ipaddress.size();
  subnetmask.resize(strlen(subnetmask.c_str()));
  auto subnetmask_len = (uint8_t)subnetmask.size();

  *length = (uint8_t)(1 + ssid_len + 1 + 1 + encryption_key_len + 1 +
                      ipaddress_len + 1 + subnetmask_len);
  auto buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  uint8_t wifiHotspot_idx = 0;
  buf[wifiHotspot_idx] = ssid_len;
  uint8_t i;
  for (i = 0; i < ssid_len; i++) {
    buf[++wifiHotspot_idx] = (uint8_t)ssid[i];
  }
  buf[++wifiHotspot_idx] = encryption_type;
  buf[++wifiHotspot_idx] = encryption_key_len;
  for (i = 0; i < encryption_key_len; i++) {
    buf[++wifiHotspot_idx] = (uint8_t)encryption_key[i];
  }
  buf[++wifiHotspot_idx] = ipaddress_len;
  for (i = 0; i < ipaddress_len; i++) {
    buf[++wifiHotspot_idx] = (uint8_t)ipaddress[i];
  }
  buf[++wifiHotspot_idx] = subnetmask_len;
  for (i = 0; i < subnetmask_len; i++) {
    buf[++wifiHotspot_idx] = (uint8_t)subnetmask[i];
  }
  *wifiHotspot = buf.release();
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

  auto ssid_str = std::string(ssid, ssid + ssid_len);
  if (ssid_len > 0 && ssid_str[ssid_len - 1] == '\n') {
    ssid_str.resize(ssid_len - 1);
  }

  auto encryption_key_str = std::string(encryption_key, encryption_key + encryption_key_len);

  uint8_t ipaddress_len_idx =
      (uint8_t)(encryption_key_idx + encryption_key_len);
  uint8_t ipaddress_len = wifiClient[ipaddress_len_idx];
  if (ipaddress_len <= 0) {
    auto ret = set_wifi_dhcp_client_config(ssid_str.c_str(), encryption_type,
                                             encryption_key_str.c_str());

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

  auto ipaddress_str = std::string(ipaddress, ipaddress + ipaddress_len);
  auto subnetmask_str = std::string(subnetmask, subnetmask + subnetmask_len);

  auto ret =
      set_wifi_client_config(ssid_str.c_str(), encryption_type, encryption_key_str.c_str(),
                             ipaddress_str.c_str(), subnetmask_str.c_str());

  printf("set_WifiClient_l2 ssid=%s, encryption_type=%d, encryption_key=%s, "
         "ipaddress=%s, subnetmask=%s ret=%d\n",
         ssid_str.c_str(), encryption_type, encryption_key_str.c_str(), ipaddress_str.c_str(),
         subnetmask_str.c_str(), ret);
  if (ret < 0)
    return ret;

  current_wifi_state = 2;
  return 0;
}

int8_t get_WifiClient_l2(uint8_t **wifiClient, uint8_t *length) {
  printf("get_WifiClient_l2\n");
  std::string ssid(32, '\0');
  uint8_t encryption_type;
  std::string encryption_key(32, '\0');
  std::string ipaddress(32, '\0');
  std::string subnetmask(32, '\0');

  int8_t ret = get_wifi_client_config(&ssid[0], &encryption_type, &encryption_key[0],
                                      &ipaddress[0], &subnetmask[0]);
  if (ret < 0)
    return -1;

  ssid.resize(strlen(ssid.c_str()));
  encryption_key.resize(strlen(encryption_key.c_str()));
  ipaddress.resize(strlen(ipaddress.c_str()));
  subnetmask.resize(strlen(subnetmask.c_str()));
  auto ssid_len = (uint8_t)ssid.size();
  auto encryption_key_len = (uint8_t)encryption_key.size();
  auto ipaddress_len = (uint8_t)ipaddress.size();
  auto subnetmask_len = (uint8_t)subnetmask.size();

  *length = (uint8_t)(1 + ssid_len + 1 + 1 + encryption_key_len + 1 +
                      ipaddress_len + 1 + subnetmask_len);
  auto buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  uint8_t wificlient_idx = 0;
  buf[wificlient_idx] = ssid_len;
  uint8_t i;
  for (i = 0; i < ssid_len; i++) {
    buf[++wificlient_idx] = (uint8_t)ssid[i];
  }
  buf[++wificlient_idx] = encryption_type;
  buf[++wificlient_idx] = encryption_key_len;
  for (i = 0; i < encryption_key_len; i++) {
    buf[++wificlient_idx] = (uint8_t)encryption_key[i];
  }
  buf[++wificlient_idx] = ipaddress_len;
  for (i = 0; i < ipaddress_len; i++) {
    buf[++wificlient_idx] = (uint8_t)ipaddress[i];
  }
  buf[++wificlient_idx] = subnetmask_len;
  for (i = 0; i < subnetmask_len; i++) {
    buf[++wificlient_idx] = (uint8_t)subnetmask[i];
  }
  *wifiClient = buf.release();
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
  auto buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  printf("get_wifi_state_l2 state=%d\n", state);
  buf[0] = state;
  *wifi_state = buf.release();
  return 0;
}

int8_t get_ethernet_l2(uint8_t **ethernet, uint8_t *length) {
  printf("get_ethernet_l2\n");
  std::string ip_address(32, '\0');
  if (get_eth_ipaddress(&ip_address[0]) < 0) {
    return -1;
  }

  ip_address.resize(strlen(ip_address.c_str()));
  auto ip_address_len = (uint8_t)ip_address.size();
  if (ip_address_len <= 0)
    return -1;
  printf("ip_address %s %d\n", ip_address.c_str(), ip_address_len);

  *length = ip_address_len + 1;
  auto buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[*length]);
  if (!buf) {
    return -1;
  }

  buf[0] = ip_address_len;
  for (int i = 0; i < ip_address_len; i++) {
    buf[i + 1] = (uint8_t)ip_address[i];
  }
  *ethernet = buf.release();

  return 0;
}

int8_t get_onvif_interface_state_l2(uint8_t **interface,uint8_t *length){

  printf("get_onvif_interface_state_l2\n");
  uint8_t onvif_interface;
  auto ret=get_onvif_interface_state(&onvif_interface); 
  if(ret<0){
    return ret;
  }
  printf("interface value=%d",onvif_interface);

  *length=1;
  **interface=onvif_interface;
  return 0;

}
