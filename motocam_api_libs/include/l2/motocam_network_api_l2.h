#include <stdint.h>

int8_t set_ipaddress_subnetmask_l2(const uint8_t *ipaddress_subnetmask);
int8_t set_WifiHotspot_l2(const uint8_t wifiHotspot_len,
                          const uint8_t *wifiHotspot);
int8_t set_ethernet_ip_address_l2(const uint8_t wifiHotspot_len,
                          const uint8_t *wifiHotspot);
int8_t set_ethernet_dhcp_config_l2();
                          
uint8_t set_onvif_interface_l2(const uint8_t onvif_interface_len, const uint8_t *onvif_interface);
int8_t get_ipaddress_subnetmask_l2(uint8_t **ipaddress_subnetmask,
                                   uint8_t *length);
int8_t get_WifiHotspot_l2(uint8_t **wifiHotspot, uint8_t *length);
int8_t set_WifiClient_l2(const uint8_t wifiClient_len,
                         const uint8_t *wifiClient);
int8_t get_WifiClient_l2(uint8_t **wifiClient, uint8_t *length);
int8_t get_wifi_state_l2(uint8_t **wifi_state,
                         uint8_t *length); // Hotspot=1, Client=2
int8_t get_ethernet_l2(uint8_t **ethernet, uint8_t *length);
int8_t get_onvif_interface_state_l2(uint8_t **interface,uint8_t *length);

