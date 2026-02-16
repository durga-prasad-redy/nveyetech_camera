#include <stdint.h>

int8_t set_network_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t set_network_WifiHotspot(const uint8_t data_length, const uint8_t *data);
int8_t set_network_WifiClient(const uint8_t data_length, const uint8_t *data);
int8_t get_network_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_network_WifiHotspot(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_network_WifiClient(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_wifi_state(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_ethernet_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_onvif_interface_l1(const uint8_t data_length,const uint8_t *data,uint8_t **res_data_bytes,uint8_t *res_data_bytes_size);

int8_t set_ethernet_ip_address_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_ethernet_dhcp_config_l1();

int8_t set_onvif_interface_l1(const uint8_t data_length, const uint8_t *data);

