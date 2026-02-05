#ifndef FW_NETWORK_H
#define FW_NETWORK_H

#include "fw.h"

int8_t get_wifi_hotspot_ipaddress( char *ip_address);
int8_t get_wifi_client_ipaddress( char *ip_address);
int8_t get_eth_ipaddress( char *ip_address);
int8_t set_ethernet_ip_address(const char *ip_address, const char *subnetmask);
int8_t set_ethernet_dhcp_config();

int8_t set_wifi_hotspot_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key, const char *ip_address, const char *subnetmask);
int8_t get_wifi_hotspot_config(char *ssid, uint8_t *encryption_type, char *encryption_key, char *ip_address, char *subnetmask);
int8_t set_wifi_client_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key, const char *ip_address, const char *subnetmask);
int8_t set_wifi_dhcp_client_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key);

int8_t get_wifi_client_config(char *ssid, uint8_t *encryption_type, char *encryption_key, char *ip_address, char *subnetmask);


int8_t get_wifi_state(uint8_t *wifi_state); // 1:Hotspot, 2:Client


int8_t set_onvif_interface(const uint8_t interface);
int8_t get_onvif_interface_state(uint8_t *interface);

#endif
