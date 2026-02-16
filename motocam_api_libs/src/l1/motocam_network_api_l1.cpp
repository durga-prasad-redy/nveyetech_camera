#include <cstdlib>               
#include <cstdio>
#include "fw/fw_network.h"
#include "motocam_network_api_l1.h"
#include "motocam_network_api_l2.h"
#include "motocam_command_enums.h"


/*
 *
 * NETWORK
 * */

int8_t set_network_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data)
{
    switch (sub_command)
    {

    case WifiHotspot:
        return set_network_WifiHotspot(data_length, data);
    case WifiClient:
        return set_network_WifiClient(data_length, data);
    case ETHERNET:
        return set_ethernet_ip_address_l1(data_length, data);

    case Onvif:
        return set_onvif_interface_l1(data_length, data);
    case ETHERNET_DHCP:
        return set_ethernet_dhcp_config_l1();
    default:
        printf("invalid sub command\n");
        return -4;
    }
}



int8_t set_network_WifiHotspot(const uint8_t data_length, const uint8_t *data)
{
    if (data_length >= 13 && data_length <= 75)
    {
        if (set_WifiHotspot_l2(data_length, data) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_network_WifiClient(const uint8_t data_length, const uint8_t *data)
{
    if (data_length >= 13 && data_length <= 75)
    {

        int8_t ret = set_WifiClient_l2(data_length, data);
        if (ret==0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        if(ret==-2)
        {
            printf("IP Address assignment timeout\n");
        }
        return ret;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_ethernet_ip_address_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length >= 13 && data_length <= 40)
    {

        int8_t ret = set_ethernet_ip_address_l2(data_length, data);
        if (ret==0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return ret;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_ethernet_dhcp_config_l1() {
  printf("set_ethernet_dhcp_config_l1\n");
  int8_t ret = set_ethernet_dhcp_config_l2();
  printf("set_ethernet_dhcp_config_l1 ret=%d\n", ret);
  return ret;
}


int8_t set_onvif_interface_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {

        int8_t ret = set_onvif_interface_l2(data_length, data);
        if (ret==0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return ret;
    }
    printf("invalid data/data length\n");
    return -5;
}


/*
 *
 * NETWORK GET COMMAND
 * */
int8_t get_network_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                           uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    switch (sub_command)
    {

    case WifiHotspot:
        return get_network_WifiHotspot(data_length, data, res_data_bytes, res_data_bytes_size);
    case WifiClient:
        return get_network_WifiClient(data_length, data, res_data_bytes, res_data_bytes_size);
    case WifiState:
        return get_wifi_state(data_length, data, res_data_bytes, res_data_bytes_size);
    case ETHERNET:
        return get_ethernet_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case Onvif:
        return get_onvif_interface_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    default:
        printf("invalid sub command\n");
        return -4;
    }
}


int8_t get_network_WifiHotspot(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_WifiHotspot_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t get_network_WifiClient(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_WifiClient_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}



int8_t get_wifi_state(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t wifi_state = get_wifi_state_l2(res_data_bytes, res_data_bytes_size);
        printf("wifi_state=%d\n", wifi_state);
        if (wifi_state >= 0)
        {
            // The res_data_bytes and res_data_bytes_size should be set by get_wifi_state_l2
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}


int8_t get_ethernet_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ethernet = get_ethernet_l2(res_data_bytes, res_data_bytes_size);
        printf("ethernet=%d\n", ethernet);
        if (ethernet >= 0)
        {
            // The res_data_bytes and res_data_bytes_size should be set by get_wifi_state_l2
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t get_onvif_interface_l1(const uint8_t data_length,const uint8_t *data,uint8_t **res_data_bytes,uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t onvif_interface_state = get_onvif_interface_state_l2(res_data_bytes, res_data_bytes_size);
        printf("onvif_interface_state=%d\n", onvif_interface_state);
        if (onvif_interface_state >= 0)
        {
            // The res_data_bytes and res_data_bytes_size should be set by get_wifi_state_l2
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;

}
