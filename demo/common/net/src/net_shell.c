/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <ctype.h>

#include "net_shell.h"
#include "if_ethersubr.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "ip_addr.h"
#include "netifapi.h"
#include "data_path.h"
#if NT_FN_DHCPS_V4
#include "lwip/apps/nt_dhcps.h"
#include "dns.h"
#include "ip_addr.h"
#endif /* NT_FN_DHCPS_V4 */
#if NT_FN_DHCP6
#include "dhcp6.h"
#endif
#if LWIP_AUTOIP
#include "autoip.h"
#endif
#if LWIP_DNS
#include "dns.h"
#endif

#include "ping.h"
#include "prefix.h"
#include "iperf.h"
#include "pmtud_demo.h"
#include "safeAPI.h"
#include "ssl_demo.h"
#include "httpc_demo.h"
#include "httpd_demo.h"

#ifdef CONFIG_MQTT_CLIENT_DEMO
#include "mqtt_client_demo.h"
#endif

#ifdef CONFIG_SUPPORT_LWIP_RAW_SOCKET
#include "sockets.h"
#include "ethernet.h"
#include "data_path.h"
#include "netif.h"
#include "qapi_wlan_misc.h"
#endif /*CONFIG_SUPPORT_LWIP_RAW_SOCKET*/

#ifdef CONFIG_SNTP_CLIENT_DEMO
#include "lwip/apps/sntp.h"
#endif

extern int lwip_socket_count(void);
extern uint8_t iperf_stream_count(void);

static ip_addr_t default_ip_address[MAX_ROLE];
static ip_addr_t default_netmask[MAX_ROLE];
static ip_addr_t default_gw[MAX_ROLE];

#ifdef CONFIG_SUPPORT_LWIP_RAW_SOCKET
#define ETH_RAW_RX_BUFFER_SIZE  512
#define HEX_BYTES_PER_LINE 16
#define HEXDUMP(inbuf, inlen, ascii, addr)  app_hexdump_raw(inbuf, inlen, ascii, addr)
static const char hexchar_net_shell[] = "0123456789ABCDEF";
static uint16_t eth_raw_rx_protocol = 0x888e;
static uint8_t eth_rx_quit = 1;
static void eth_help(void);
uint8_t net_ascii_to_hex(char val);
int32_t net_ether_aton(const char *orig, uint8_t *eth);
#endif /*CONFIG_SUPPORT_LWIP_RAW_SOCKET*/

#if NT_FN_DHCPS_V4 && LWIP_DHCP
/**
 * API to enable/disable DHCP.
 *
 * @param pointer to netif on which dhcp server/client need to be enabled.
 * @param To select server/client :: if is_server = 1 start as server else if is_server = 0 start as client.
 * @param state of the DHCP (TURN_ON_DHCP/TURN_OFF_DHCP).
 * @param To set the value of dhcp_enable from CLI or from dev_config.
 *
 * @return QAPI_OK on success.
 *
 */
static qapi_Status_t net_enable_disable_dhcp(struct netif *netif, NT_BOOL is_server, uint8_t state)
{
    qapi_Status_t err = QAPI_ERROR;
    uint8_t dhcp_enable = TURN_OFF_DHCP;
    uint8_t role;  // 0 for sta, 1 for AP

    if (netif == NULL) {
        info_printf("netif is NULL\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    role = is_server ? 1 : 0;
    dhcp_enable = state;

    if (is_server) {
        NT_BOOL status = FALSE;

        /* IP address for the device in AP mode before starting DHCP */
        ip_addr_t apipaddr = IPADDR4_INIT_BYTES(192, 168, 0, 1);       // IPV4_adderss for netif (AP)
        ip_addr_t net_mask_ap = IPADDR4_INIT_BYTES(255, 255, 255, 0);  // IPV4_netmask adderss for netif (AP)

        if (dhcp_enable == TURN_ON_DHCP) {
            netif_set_addr(netif, (const ip4_addr_t *)ip_2_ip4(&apipaddr), (const ip4_addr_t *)ip_2_ip4(&net_mask_ap),
                           (const ip4_addr_t *)ip_2_ip4(&apipaddr));
            status = nt_ap_dhcps_start(netif);
            if (status != TRUE) {
                info_printf("DHCP server start failed\n");
                err = QAPI_NET_ERR_OPERATION_FAILED;
                goto Default;
            }
            err = QAPI_OK;
            info_printf("DHCP server start success\n");
        } else if (dhcp_enable == TURN_OFF_DHCP) {
            if (nt_dhcps_netif_status(netif) == DHCP_STARTED) {
                status = nt_ap_dhcps_stop(netif);
                if (status != TRUE) {
                    info_printf("DHCP server stop failed\n");
                } else {
                    err = QAPI_OK;
                    info_printf("DHCP server stop success\n");
                    goto Default;
                }
            }
        } else {
            info_printf("Invalid command for DHCP Server\n");
        }
    } else if (!is_server) {
        err_t status;

        if (dhcp_enable == TURN_ON_DHCP) {
            netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
            status = dhcp_start(netif);
            if (status != ERR_OK) {
                info_printf("DHCP client start failed\n");
                goto Default;
            }
            err = NT_OK;
            info_printf("DHCP client start success\r\n", 0, 0, 0);
        } else if (dhcp_enable == TURN_OFF_DHCP) {
            status = dhcp_release(netif);
            if (status != ERR_OK) {
                info_printf("DHCP client release failed\n");
            } else {
                err = NT_OK;
                info_printf("DHCP client release success\n", 0, 0, 0);
                goto Default;
            }
        } else {
            info_printf("Invalid command for DHCP Server\n", 0, 0, 0);
        }
    } else {
        info_printf("DHCP Start/stop Failed as mode of operation is not correct\n", 0, 0, 0);
    }

    return err;

Default:
    netif_set_addr(netif, (const ip4_addr_t *)ip_2_ip4(&default_ip_address[role]),
                   (const ip4_addr_t *)ip_2_ip4(&default_netmask[role]),
                   (const ip4_addr_t *)ip_2_ip4(&default_gw[role]));

    return err;
}
#endif

static uint8_t nt_get_netifidx_by_devmode(u8_t devid)
{
    struct netif *netif;

    if (devid <= AP_DEVICE) {
        NETIF_FOREACH(netif)
        {
            if (devid == ((device_t *)netif->state)->role) {
                return netif->num + 1; /* found! */
            }
        }
    }

    return 0;
}

static struct netif *get_netif_by_device(int devid)
{
    uint8_t netid;

    netid = nt_get_netifidx_by_devmode(devid);
    return netif_get_by_index(netid);
}
static qapi_Status_t dhcpv4c(uint32_t __attribute__((__unused__)) Parameter_Count,
                             QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
#if NT_FN_DHCPS_V4 && LWIP_DHCP
    char *action = NULL;
    struct netif *netif = NULL;
    uint8_t is_server = 0, state = TURN_OFF_DHCP;
    char *interface_name = NULL;

    if (Parameter_Count < 2 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    interface_name = Parameter_List[0].String_Value;
    if (strncmp(interface_name, "wlan1", 5) != 0) {
        info_printf("Just wlan1 support DHCP client mode currently\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    action = Parameter_List[1].String_Value;
    is_server = 0;

    netif = get_netif_by_device(STA_DEVICE);

    if ((!memcmp(action, "new", 3))) {
        state = TURN_ON_DHCP;
    } else if (!memcmp(action, "release", 7)) {
        state = TURN_OFF_DHCP;
    } else {
        info_printf("DHCP Command Failed due to invalid option i.e. start/stop.\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    net_enable_disable_dhcp(netif, is_server, state);
    return QAPI_OK;
#else
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_OK;
#endif
}

#if LWIP_DNS
void dnsc_found_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    (void)arg;
    if (ipaddr)
        info_printf("%s IP is  %s\r\n", name, ipaddr_ntoa(ipaddr));
    else
        info_printf("get %s IP failed\n", name);
}
#endif

static qapi_Status_t dnsc(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
#if LWIP_DNS
    uint32_t indx;
    const ip_addr_t *server_addr;
    char *cmd, *hostname;
    ip_addr_t ip_addr;

    if (Parameter_Count == 0) {
        // get DNS list
        for (indx = 0; indx < DNS_MAX_SERVERS; indx++) {
            server_addr = (ip_addr_t *)dns_getserver(indx);
            if (!ip_addr_isany_val(*server_addr)) {
                info_printf("DNS Server[%d] : %s\r\n", indx, ipaddr_ntoa(server_addr));
            }
        }
        return QAPI_OK;
    }
    cmd = Parameter_List[0].String_Value;
    if (strncmp(cmd, "addsvr", 6) == 0) {
        if (Parameter_Count < 2) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        if (!ipaddr_aton(Parameter_List[1].String_Value, &ip_addr)) {
            info_printf("Invalid IP Address. Please try again \n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        int insert_indx = -1;
        for (indx = 0; indx < DNS_MAX_SERVERS; indx++) {
            server_addr = (ip_addr_t *)dns_getserver(indx);
            if (ip_addr_cmp(server_addr, &ip_addr)) {
                info_printf("this IP Address already exists.\n");
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }

            if ((insert_indx == -1) && ip_addr_isany_val(*server_addr)) {
                insert_indx = indx;
            }
        }

        if (insert_indx >= 0 && insert_indx < DNS_MAX_SERVERS) {
            dns_setserver(insert_indx, &ip_addr);
            info_printf("add DNS server OK.\n");
        } else {
            info_printf("add DNS server failed, the array is full now.\n");
        }
    } else if (strncmp(cmd, "delsvr", 6) == 0) {
        if (Parameter_Count < 2) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        if (!ipaddr_aton(Parameter_List[1].String_Value, &ip_addr)) {
            info_printf("Invalid IP Address. Please try again \n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        for (indx = 0; indx < DNS_MAX_SERVERS; indx++) {
            server_addr = (ip_addr_t *)dns_getserver(indx);
            if ((!ip_addr_isany_val(*server_addr)) && ip_addr_cmp_zoneless(server_addr, &ip_addr)) {
                dns_setserver(indx, NULL);
                info_printf("del DNS server OK.\n");
            }
        }
    } else if (strcmp(cmd, "gethostbyname") == 0) {
        err_t result;
        if (Parameter_Count < 2) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        hostname = Parameter_List[1].String_Value;
        result = dns_gethostbyname(hostname, &ip_addr, dnsc_found_callback, NULL);

        if (result == ERR_OK) {
            info_printf("%s IP is  %s[cache]\n", hostname, ipaddr_ntoa(&ip_addr));
        } else if (result != ERR_INPROGRESS) {
            info_printf("gethostbyname err:%d.\n", result);
        }
    } else if (strcmp(cmd, "gethostbyname2") == 0) {
        err_t result;
        char *type;
        uint8_t addr_type = 0;
        if (Parameter_Count < 3) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        hostname = Parameter_List[1].String_Value;
        type = Parameter_List[2].String_Value;

        if (strcmp(type, "v4") == 0)
            addr_type = LWIP_DNS_ADDRTYPE_IPV4;
        else if (strcmp(type, "v6") == 0)
            addr_type = LWIP_DNS_ADDRTYPE_IPV6;
        else if (strcmp(type, "v4v6") == 0)
            addr_type = LWIP_DNS_ADDRTYPE_IPV4_IPV6;
        else if (strcmp(type, "v6v4") == 0)
            addr_type = LWIP_DNS_ADDRTYPE_IPV6_IPV4;
        else {
            info_printf("invalid type.\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        result = dns_gethostbyname_addrtype(hostname, &ip_addr, dnsc_found_callback, NULL, addr_type);

        if (result == ERR_OK) {
            info_printf("%s IP is  %s[cache]\n", hostname, ipaddr_ntoa(&ip_addr));
        } else if (result != ERR_INPROGRESS) {
            info_printf("gethostbyname2 err:%d.\n", result);
        }
    } else {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
#else
    PRINT_ERR_NOT_SUPPORTED;
#endif
    return QAPI_OK;
}

/**
 * Fetch IP address from network interface in network order.
 *
 * @param type IP version type to be fetch IPADDR_TYPE_V6 or IPADDR_TYPE_V4.
 * @param idx Index to select IP for IPv6 idx = 0 (link_local), idx = 1 (site_local), idx = 2 (global_uincast)
 *                                  for IPV4 idx = 0 (ipv4 address), idx = 1 (subnet mask), idx = 2 (default gateway)
 * @return pointer to IP address else NULL on failure.
 */
static const ip_addr_t *net_get_ip(struct netif *netif, u8_t type, s8_t idx)
{
    if (netif == NULL) {
        info_printf("Default network interface not initialized");
        return NULL;
    }

    if (!((type == IPADDR_TYPE_V4) || (type == IPADDR_TYPE_V6)) && ((idx < 0) && (idx > 2))) {
        info_printf("Invalid type  or Invalid index ");
        return NULL;
    }

#if LWIP_IPV6
    if (type == IPADDR_TYPE_V6) {
        return &netif->ip6_addr[idx];  // ipv6_address
    }
#endif

#if LWIP_IPV4
    if (type == IPADDR_TYPE_V4) {
        if (idx == IPv4_IP_IDX) {  // ipv4 address
            return netif_ip_addr4(netif);
        } else if (idx == IPv4_NETMASK_IDX) {  // ipv4 subnet mask
            return netif_ip_netmask4(netif);
        } else if (idx == IPv4_GATEWAY_IDX) {  // ipv4 default gateway
            return netif_ip_gw4(netif);
        }
    }
#endif
    return NULL;
}

/*
 * print IP address/netmask/gateway
 */
typedef enum {
    NETIF_IP_VER_V4 = 0U,
    NETIF_IP_VER_V6 = 1U,
} netif_ipaddr_type_t;

void show_net_info_by_id(uint8_t id, uint8_t ip_ver)
{
    ip_addr_t *ip_addr = NULL;
    char *addr = NULL;
    uint8_t ip_type = 0;
    struct netif *netif = NULL;

    netif = netif_get_by_index(id);

    if (netif == NULL) {
        info_printf("netif is NULL\n");
        return;
    }
    if (ip_ver == NETIF_IP_VER_V4) {
        info_printf("DHCPv4c: ");
        for (int i = 0; i < 3; i++) {
            ip_addr = (ip_addr_t *)net_get_ip(netif, ip_type, i);
            if (i == IPv4_IP_IDX) {
                printf("IP=");
            } else if (i == IPv4_NETMASK_IDX) {
                printf(" Subnet Mask=");
            } else if (i == IPv4_GATEWAY_IDX) {
                printf(" Gateway=");
            }
            if (!ip_addr_isany_val(*ip_addr)) {
                addr = ipaddr_ntoa(ip_addr);
                if (addr != NULL) {
                    printf("%s", addr);
                    addr = NULL;
                }
            }
        }
        printf("\n");
    }
}

/*
 * Show the network interface info, include mac address, IP address/netmask/gateway
 */
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR     "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

static void net_show_info(struct netif *netif)
{
    char pbuf[100];
    NT_BOOL state = FALSE;
    ip_addr_t *ip_addr = NULL;
    char addr_type[16] = {0};
    char *addr = NULL;
    uint8_t ip_type = 0;
    char *padding = ". . . . . . . . . . . . . . . . . :";
#if ((defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4))
    ip_addr_t *server_addr = NULL;
#endif  // #if  ((defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4))
    char interface_name[6] = "wlan0";

    if (strncmp(netif->name, "st", 2) == 0)
        memscpy(interface_name, 5, "wlan1", 5);
    if (strncmp(netif->name, "ap", 2) == 0)
        memscpy(interface_name, 5, "wlan0", 5);
    if (strncmp(netif->name, "lo", 2) == 0)
        memscpy(interface_name, 5, "local", 5);

    info_printf("%s:%s\n", interface_name, netif->flags & NETIF_FLAG_UP ? "UP" : "DOWN");

#if NT_FN_DHCPS_V4
    struct dhcp *dhcp = netif_dhcp_data(netif);
    state = (dhcp != NULL && dhcp->state != DHCP_STATE_OFF) ? TRUE : FALSE;
#endif /* NT_FN_DHCPS_V4 */

    info_printf("**************************************************************\n");
    info_printf("IP Configuration(interface %d)\n", netif->num);
    info_printf("Phy address:" MACSTR, MAC2STR(netif->hwaddr));

    printf("\n");
    for (int j = 0; j < 2; j++) {
        ip_type = (j == 0) ? IPADDR_TYPE_V6 : IPADDR_TYPE_V4;
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES || i < 3; i++) {
            memset(addr_type, 0, sizeof(addr_type));
            ip_addr = (ip_addr_t *)net_get_ip(netif, ip_type, i);
            if (ip_addr != NULL) {
#if LWIP_IPV6
                if (ip_type == IPADDR_TYPE_V6) {
                    if (ip6_addr_isglobal(ip_2_ip6(ip_addr))) {
                        strlcpy(addr_type, "Global-local", strlen("Global-local") + 1);
                    } else if (ip6_addr_islinklocal(ip_2_ip6(ip_addr))) {
                        strlcpy(addr_type, "Link-local", strlen("Link-local") + 1);
                    } else if (ip6_addr_issitelocal(ip_2_ip6(ip_addr))) {
                        strlcpy(addr_type, "Site-local", strlen("Site-local") + 1);
                    } else if (ip6_addr_isuniquelocal(ip_2_ip6(ip_addr))) {
                        strlcpy(addr_type, "Unique-local", strlen("Unique-local") + 1);
                    } else if (ip6_addr_isipv4mappedipv6(ip_2_ip6(ip_addr))) {
                        strlcpy(addr_type, "v4mapped-v6", strlen("v4mapped-v6") + 1);
                    }
                    if (!ip_addr_isany_val(*ip_addr)) {
                        addr = ipaddr_ntoa(ip_addr);
                        if (addr != NULL) {
                            snprintf(pbuf, sizeof(pbuf), "%s %s\r\n", padding, addr);
                            if (ip_type == IPADDR_TYPE_V6) {
                                snprintf(pbuf, sizeof(pbuf), "%s IPv6 Address", addr_type);
                            } else {
                                snprintf(pbuf, sizeof(pbuf), "%s", addr_type);
                            }
                            pbuf[strlen(pbuf)] = (strlen(pbuf) % 2 == 0) ? '.' : ' ';
                            info_printf("%s", pbuf);
                            addr = NULL;
                        }
                    }
                } else
#endif
                {
                    if (ip_type == IPADDR_TYPE_V4) {
                        if (i == IPv4_IP_IDX) {
                            info_printf("IPv4: %s ", ipaddr_ntoa(ip_addr));
                        } else if (i == IPv4_NETMASK_IDX) {
                            printf("Subnet Mask: %s ", ipaddr_ntoa(ip_addr));
                        } else if (i == IPv4_GATEWAY_IDX) {
                            printf("Default Gateway: %s\n", ipaddr_ntoa(ip_addr));
                        }
                    }
                }
            }
        }
    }
#if LWIP_IPV4
    info_printf("DHCP Enabled : %s\r\n", (state ? "yes" : "no"));
#endif
#if ((defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4))
    server_addr = dhcp_dns_getserver();

    if (!ip_addr_isany_val(*server_addr)) {
        info_printf("DNS configured in DHCP server IP address . . . . . : %s\r\n", ipaddr_ntoa(server_addr));
    }

    for (int indx = 0; indx < 2; indx++) {
        server_addr = (ip_addr_t *)dns_getserver(indx);
        if (!ip_addr_isany_val(*server_addr)) {
            info_printf("DNS Server : %s\r\n", ipaddr_ntoa(server_addr));
        }
    }
#endif  // (defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4)
    info_printf("**************************************************************\n");
}

/**
 * set IP address from network interface.
 *
 * @param ip IP address pointer having IP stored in network order.
 * @param idx Index to select IP for IPv6 idx = 0 (link_local), idx = 1 (site_local), idx = 2 (multicast)
 *                                  for IPV4 idx = 0 (ipv4 address), idx = 1 (subnet mask), idx = 2 (default gateway)
 * @return (void) set IP address on netif_default.
 *
 */
static qapi_Status_t net_set_ip(struct netif *netif, ip_addr_t *ip, s8_t idx)
{
    if (netif == NULL) {
        netif = netif_get_by_index(DEFAULT_NETIF_IDX);
    }

    if (netif == NULL && ip_addr_isany(ip) && ((idx < 0) && (idx > 2))) {
        info_printf("Default network interface not initialized");
        return QAPI_NET_ERR_CANNOT_GET_SCOPEID;
    }

#if LWIP_IPV6
    if (IP_IS_V6_VAL(*ip)) {
        netif_ip6_addr_set(netif, idx, (const ip6_addr_t *)ip_2_ip6(ip));
        netif_ip6_addr_set_state(netif, idx, IP6_ADDR_TENTATIVE);
    }
#endif
#if LWIP_IPV4
    if (IP_IS_V4_VAL(*ip)) {
        if (idx == IPv4_IP_IDX)
            netif_set_ipaddr(netif, (const ip4_addr_t *)ip_2_ip4(ip));
        else if (idx == IPv4_NETMASK_IDX)
            netif_set_netmask(netif, (const ip4_addr_t *)ip_2_ip4(ip));
        else if (idx == IPv4_GATEWAY_IDX)
            netif_set_gw(netif, (const ip4_addr_t *)ip_2_ip4(ip));
    }
#endif
    return QAPI_OK;
}

#if LWIP_IPV6
static int is_valid_prefix(const char *str);
/**
 *Set or config prefix of IPv6 address for  the AP interface
 */
static qapi_Status_t prefix_set(uint32_t __attribute__((__unused__)) Parameter_Count,
                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    ip6_addr_t ip6_addr;
    struct netif *netif = NULL;
    uint8_t parts[8];
    char str[8][6];
    char buf[40];
    char *ptr = NULL;
    int i;

    netif = get_netif_by_device(AP_DEVICE);
    if (NULL == netif)
        info_printf("prefix_set can only be run under AP role");

    switch (Parameter_Count) {
        case 0:
            // show the prefix
            parts[0] = (netif->ip6_addr[1].u_addr.ip6.addr[0]) & 0xFF;
            parts[1] = (netif->ip6_addr[1].u_addr.ip6.addr[0] >> 8) & 0xFF;
            parts[2] = (netif->ip6_addr[1].u_addr.ip6.addr[0] >> 16) & 0xFF;
            parts[3] = (netif->ip6_addr[1].u_addr.ip6.addr[0] >> 24) & 0xFF;

            parts[4] = (netif->ip6_addr[1].u_addr.ip6.addr[1]) & 0xFF;
            parts[5] = (netif->ip6_addr[1].u_addr.ip6.addr[1] >> 8) & 0xFF;
            parts[6] = (netif->ip6_addr[1].u_addr.ip6.addr[1] >> 16) & 0xFF;
            parts[7] = (netif->ip6_addr[1].u_addr.ip6.addr[1] >> 24) & 0xFF;

            for (i = 0; i < 8; i++) {
                snprintf(str[i], sizeof(str[i]), "%02x", parts[i]);
            }

            snprintf(buf, sizeof(buf), "%s%s:%s%s:%s%s:%s%s\r\n", str[0], str[1], str[2], str[3], str[4], str[5],
                     str[6], str[7]);
            info_printf("Prefix is: %s\r\n", buf);

            break;
        case 1:
            // check valid prefix
            ptr = Parameter_List[0].String_Value;
            if (!is_valid_prefix(ptr)) {
                info_printf("Prefix format error\r\n");
                break;
            }
            memset(buf, 0, sizeof(buf));
            strlcpy(buf, ptr, sizeof(buf));
            strlcat(buf, "::1", sizeof(buf));
            if (ip6addr_aton(buf, &ip6_addr) == 0) {
                info_printf("Prefix format error\r\n");
                break;
            } else {
                // check global or not
                if (ip6_addr_isglobal(&ip6_addr)) {
                    netif_ip6_addr_set(netif, 1, &ip6_addr);
                    netif->ip6_autoconfig_enabled = 1;
                    netif_ip6_addr_set_state(netif, 1, IP6_ADDR_TENTATIVE);
                    break;
                } else {
                    info_printf("Prefix format error\r\n");
                    break;
                }
            }
            break;
        default:
            info_printf("Wrong params\r\n");
    }
}

static int is_valid_prefix(const char *str)
{
    if (strlen(str) > 19) {
        return 0;
    }

    int colon_count = 0;
    int is_valid = 1;
    int last_colon_pos = -1;

    if (str[0] == ':' || str[strlen(str) - 1] == ':') {
        return 0;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (c == ':') {
            colon_count++;
            if (last_colon_pos == i - 1) {
                is_valid = 0;
                break;
            }
            last_colon_pos = i;

        } else {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                is_valid = 0;
                break;
            }
        }
    }

    if (colon_count != 3) {
        return 0;
    }

    return is_valid;
}
#endif /* LWIP_IPV6 */

/**
 * Display IP address for network interface.
 */
static qapi_Status_t ifconfig(uint32_t __attribute__((__unused__)) Parameter_Count,
                              QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    char *interface_name = NULL;
    struct netif *netif = NULL;
    ip_addr_t ip_addr;
    ip_addr_t netmask;
    uint8_t is_empty = 1;

    switch (Parameter_Count) {
        case 0:
            NETIF_FOREACH(netif)
            {
                net_show_info(netif);
                is_empty = 0;
            }
            if (is_empty) {
                info_printf("Default network interface not initialized");
                return QAPI_ERROR;
            }
            break;
        case 1:
            interface_name = Parameter_List[0].String_Value;
            info_printf("%s\n", interface_name);
            if (strncmp(interface_name, "wlan0", 5) == 0) {
                /* There is a mapping: wlan0-AP mode */
                netif = get_netif_by_device(AP_DEVICE);
                if (netif)
                    net_show_info(netif);
                else {
                    info_printf("network interface not initialized\n");
                    return QAPI_ERROR;
                }
            } else if (strncmp(interface_name, "wlan1", 5) == 0) {
                /* There is a mapping: wlan11-Station mode */
                netif = get_netif_by_device(STA_DEVICE);
                if (netif)
                    net_show_info(netif);
                else {
                    info_printf("network interface not initialized\n");
                    return QAPI_ERROR;
                }
            } else {
                info_printf("Error: not support the interface:%s\n", interface_name);
            }
            break;
        /* Set IPv4 address for WLAN interfaces */
        case 2: /* ifconfig wlan0 0.0.0.0 */
        case 3: /* ifconfig wlan0 192.168.2.100 255.0.0.0 */
        case 4: /* ifconfig wlan0 192.168.2.100 255.0.0.0 192.168.2.1 */
            interface_name = Parameter_List[0].String_Value;
            if (strncmp(interface_name, "wlan0", 5) == 0) {
                netif = get_netif_by_device(AP_DEVICE);
            } else if (strncmp(interface_name, "wlan1", 5) == 0) {
                netif = get_netif_by_device(STA_DEVICE);
            } else {
                info_printf("Error: not support the interface:%s\n", interface_name);
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }

            if (netif == NULL) {
                info_printf("network interface not initialized\n");
                return QAPI_ERROR;
            }
            if (!ipaddr_aton(Parameter_List[1].String_Value, &ip_addr)) {
                info_printf("Please try again invalid IP Address \r\n");
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }

#if LWIP_IPV4
            if (IP_IS_V4_VAL(ip_addr)) {
                ip_addr_t gw_addr;
                info_printf("Received IP address: %s\r\n", ipaddr_ntoa(&ip_addr));
                net_set_ip(netif, &ip_addr, IPv4_IP_IDX);

                if (Parameter_Count >= 3) {
                    if (!ipaddr_aton(Parameter_List[2].String_Value, &netmask)) {
                        info_printf("Please try again invalid subnet mask \r\n");
                        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
                    }
                    if (!ip_addr_netmask_valid(ip_2_ip4(&netmask))) {
                        IP_ADDR4(&netmask, 255, 255, 0, 0);
                        info_printf("Netmask is invalid setting to default 255.255.0.0 \r\n");
                    }
                    info_printf("Received subnet mask: %s\r\n", ipaddr_ntoa(&netmask));
                    net_set_ip(netif, &netmask, IPv4_NETMASK_IDX);
                }

                if (Parameter_Count >= 4) {
                    if (!ipaddr_aton(Parameter_List[3].String_Value, &gw_addr)) {
                        info_printf("Please try again invalid gateway IP Address \r\n");
                        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
                    }
                    info_printf("Received gateway IP address: %s\r\n", ipaddr_ntoa(&gw_addr));
                    net_set_ip(netif, &gw_addr, IPv4_GATEWAY_IDX);
                }
            }
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
            else if (IP_IS_V6_VAL(ip_addr)) {
                int idx;
                idx = atoi(Parameter_List[2].String_Value);
                if ((idx < 1) && (idx > 2)) {
                    info_printf("Invalid index selected for ipv6 address index set to default \r\n");
                    idx = 1;
                }
                net_set_ip(netif, &ip_addr, idx);  // setting ipv6 address
            }
#endif /* LWIP_IPV6 */
            else {
                return QAPI_NET_ERR_INVALID_IPADDR;
            }
            break;
        default:
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    return QAPI_OK;
}

#define CFG_PING_MAX_TX  10000  // increase the max_size from 1470 to 10000 to pass WFA 11N-5.2.35/36
#define CFG_PING6_MAX_TX 1450
static qapi_Status_t pingv4(uint32_t __attribute__((__unused__)) Parameter_Count,
                            QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    ip_addr_t ip_addr;
    uint32_t size = 64;
    uint32_t count = 4;
    uint32_t delay = 1000;
    char *ptr = NULL;
    struct netif *netif = NULL;
    uint8_t is_empty = 1;
    uint8_t i;

    if (Parameter_Count < 1 || Parameter_List == NULL) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    NETIF_FOREACH(netif)
    {
        is_empty = 0;
        break;
    }
    if (is_empty) {
        info_printf("Default network interface not initialized");
        return QAPI_ERROR;
    }

    if (Parameter_Count >= 1) {
        ptr = Parameter_List[0].String_Value;
        if (!ipaddr_aton(ptr, &ip_addr)) {
            info_printf("error :invalid IP Addres. Please try again\n");
            return QAPI_ERROR;
        }
        if (ip6_addr_islinklocal(ip_2_ip6(&ip_addr))) {
            if (get_netif_by_device(AP_DEVICE)) {
                ip_addr.u_addr.ip6.zone = nt_get_netifidx_by_devmode(AP_DEVICE);
            } else if (get_netif_by_device(STA_DEVICE)) {
                ip_addr.u_addr.ip6.zone = nt_get_netifidx_by_devmode(STA_DEVICE);
            } else {
                ;
            }
        }
    }

    for (i = 1; i < Parameter_Count; i++) {
        if (strcmp(Parameter_List[i].String_Value, "-c") == 0) {
            ++i;
            if (Parameter_List[i].Integer_Is_Valid) {
                count = Parameter_List[i].Integer_Value;
            }
        } else if (strcmp(Parameter_List[i].String_Value, "-d") == 0) {
            ++i;
            if (Parameter_List[i].Integer_Is_Valid) {
                delay = Parameter_List[i].Integer_Value;
            }
        } else if (strcmp(Parameter_List[i].String_Value, "-s") == 0) {
            ++i;
            if (Parameter_List[i].Integer_Is_Valid) {
                size = Parameter_List[i].Integer_Value;
                /* if repsonse client does not support IP fragment when size > 1538( data is1472)Bytes,
                will not get response with some AP */
                if (IP_IS_V6(&ip_addr) && size > CFG_PING6_MAX_TX) {
                    info_printf("Size should be <= %d\n", CFG_PING6_MAX_TX);
                    return QAPI_ERROR;
                }
                if (IP_IS_V4(&ip_addr) && size > CFG_PING_MAX_TX) {
                    info_printf("Size should be <= %d\n", CFG_PING_MAX_TX);
                    return QAPI_ERROR;
                }
            }
        }
    } /* for loop */
    info_printf("Pinging %s with %u bytes of data:\r\n", ipaddr_ntoa(&ip_addr), (unsigned int)size);
    ping(&ip_addr, size, count, delay);

    return QAPI_OK;
}

/*****************************************************************************
 *              [0]          [1]      [2]             [3]               [4]
 * prefix <interface_name> <v6addr> <prefixlen> <prefix_lifetime> <valid_lifetime>
 ****************************************************************************/

static qapi_Status_t prefix_v6(uint32_t __attribute__((__unused__)) Parameter_Count,
                               QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    ip_addr_t v6addr;
    uint32_t prefixlen = 0;
    uint32_t prefix_lifetime = 0;
    uint32_t valid_lifetime = 0;
    char *interface_name;
    char *ptr = NULL;
    uint8_t netid = 0;

    if (Parameter_Count < 5) {
        return QAPI_ERROR;
    }

    interface_name = Parameter_List[0].String_Value;
    prefixlen = Parameter_List[2].Integer_Value;
    prefix_lifetime = Parameter_List[3].Integer_Value;
    valid_lifetime = Parameter_List[4].Integer_Value;
    ptr = Parameter_List[1].String_Value;

    if (!ipaddr_aton(ptr, &v6addr)) {
        info_printf("error :invalid IP Addres. Please try again\n");
        return QAPI_ERROR;
    }

    if (strncmp(interface_name, "wlan0", 5) == 0) {
        netid = nt_get_netifidx_by_devmode(AP_DEVICE);
    }

    prefix_send(&v6addr, prefixlen, prefix_lifetime, valid_lifetime, netid);
    return QAPI_OK;
}

/*****************************************************************************
 * DHCPv4 Server: Set up and configure Dynamic Host Configuration Protocol v4 server
 *           [0]   [1] [2]           [3]           [4]
 * dhcpv4s wlan0  pool 192.168.1.10  192.168.1.50  3600
 * dhcpv4s wlan0  start
 * dhcpv4s wlan0  stop
 *****************************************************************************/
static qapi_Status_t dhcpv4s(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
#if NT_FN_DHCPS_V4 && LWIP_DHCP
    struct netif *netif = NULL;
    char *interface_name, *cmd;
    char *start_ip_addr_string;
    char *end_ip_addr_string;
    struct dhcps_lease lease;

    if (Parameter_Count < 2 || Parameter_Count > 5 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    interface_name = Parameter_List[0].String_Value;
    cmd = Parameter_List[1].String_Value;

    if (strncmp(interface_name, "wlan0", 5) != 0) {
        info_printf("Just wlan0 support DHCP server mode currently\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    netif = get_netif_by_device(AP_DEVICE);
    if (!netif) {
        info_printf("SoftAP is not started.\n");
        return QAPI_ERROR;
    }

    if (strncmp(cmd, "pool", 4) == 0) {
        start_ip_addr_string = Parameter_List[2].String_Value;
        end_ip_addr_string = Parameter_List[3].String_Value;

        if (!ipaddr_aton(start_ip_addr_string, &(lease.start_ip))) {
            info_printf("Please try again invalid IP Address \r\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        if (!ipaddr_aton(end_ip_addr_string, &(lease.end_ip))) {
            info_printf("Please try again invalid IP Address \r\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        lease.lease_time = 0;
        if (Parameter_Count == 5 && Parameter_List[4].Integer_Is_Valid) {
            lease.lease_time = Parameter_List[4].Integer_Value;
        }

        lease.enable = TRUE;
        if (!nt_set_dhcps_lease(&lease)) {
            info_printf("configure pool address fail \r\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
        }
    } else if (strncmp(cmd, "start", 5) == 0) {
        if (!nt_ap_dhcps_start(netif)) {
            info_printf("DHCP server start failed \r\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
        }
        info_printf("DHCP server start success \r\n");
    } else if (strncmp(cmd, "stop", 4) == 0) {
        if (nt_dhcps_netif_status(netif) == DHCP_STARTED) {
            if (!nt_ap_dhcps_stop(netif)) {
                info_printf("DHCP server stop failed \r\n");
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
            }
            info_printf("DHCP server stop success \r\n");
        } else {
            info_printf("DHCP server not start \r\n");
        }

    } else {
        info_printf("Invalid command: %s\n", cmd);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    return QAPI_OK;
#else
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_OK
#endif
}

#ifdef CONFIG_SNTP_CLIENT_DEMO
static qapi_Status_t sntpc(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t id = 0;
    char msg_name_too_long[] = "Domain name or address cannot be more then 64 bytes\n";
    char msg_op_class[] = "the operating class for SNTP client should be 0 or 1";
    char *cmd = NULL;
    char *ptr = NULL;
    ip_addr_t ip_addr;

    /* sntpc */
    if (Parameter_Count == 0) {
        uint8_t i = 0;
        uint8_t started;
        const char *name = NULL;
        const ip_addr_t *addr = NULL;

        started = sntp_enabled();
        info_printf("SNTP client is %s.\n", started ? "started" : "stopped");

        for (i = 0; i < SNTP_MAX_SERVERS; i++) {
            name = sntp_getservername(i);
            addr = sntp_getserver(i);
            info_printf("%d; %s		%s", i, name != NULL ? name : "****", addr != NULL ? ipaddr_ntoa(addr) : "****");
            if (sntp_getkodreceived(i) != 0) {
                printf("	KOD");
            }
            printf("\n");
        }

        /* If not started, we want to display cmd syntax */
        if (!started) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        return QAPI_OK;
    }

    cmd = Parameter_List[0].String_Value;
    /* Sntpc  start */
    if (strncmp(cmd, "start", 5) == 0) {
        sntp_init();
    }
    /* Sntpc  stop */
    else if (strncmp(cmd, "stop", 4) == 0) {
        sntp_stop();
    }
    /* Sntpc  set operating class */
    else if (strncmp(cmd, "setOpMode", 9) == 0) {
        uint8_t opMode = 0;
        if (Parameter_Count >= 2 && Parameter_List[1].Integer_Is_Valid) {
            opMode = Parameter_List[1].Integer_Value;
            if (opMode < 2) {
                sntp_setoperatingmode(opMode);
                return QAPI_OK;
            }
        }
        info_printf("\n%s\n", msg_op_class);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    /* Sntpc  setServer <IP addr | name> [id] */
    else if (strncmp(cmd, "setServer", 8) == 0) {
        if (Parameter_Count >= 3 && Parameter_List[2].Integer_Is_Valid) {
            id = Parameter_List[2].Integer_Value;
            if (id >= SNTP_MAX_SERVERS) {
                info_printf("id exceed the max number of SNTP servers");
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }
        }

        if (Parameter_Count >= 2) {
            ptr = Parameter_List[1].String_Value;
            if (strlen(ptr) > 64) {
                info_printf("\n%s\n", msg_name_too_long);
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }
            if (ipaddr_aton(ptr, &ip_addr)) {
                sntp_setserver(id, &ip_addr);
            } else {
                sntp_setservername(id, ptr);
            }
            return QAPI_OK;
        }
    }
    return QAPI_OK;
}
#endif

static qapi_Status_t socketstat(uint32_t __attribute__((__unused__)) Parameter_Count,
                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    lwip_socket_count();
    iperf_stream_count();
    return QAPI_OK;
}

#ifdef CONFIG_SUPPORT_LWIP_RAW_SOCKET
void app_hexdump_raw(void *inbuf, uint32_t inlen, int ascii, int addr)
{
    uint8_t *cp = (uint8_t *)inbuf;
    uint8_t *ap = (uint8_t *)inbuf;
    int len = (int)inlen;
    int clen, alen, i;
    char outbuf[96];
    char *outp = &outbuf[0];
    int  line = 0;

    memset(outbuf, 0, sizeof(outbuf));
    while (len > 0)
    {
        if (addr)
            outp += snprintf(outp, sizeof(outbuf), "[%p] ", cp);

        clen = alen = min(HEX_BYTES_PER_LINE, len);

        /* display data in hex */
        for (i = 0; i < HEX_BYTES_PER_LINE; i++)
        {

            if (--clen >= 0)
            {
                uint8_t uc = *cp++;

                *outp++ = hexchar_net_shell[(uc >> 4) & 0x0f];
                *outp++ = hexchar_net_shell[(uc) & 0x0f];
                *outp++ = ' ';
            }
            else if (line != 0)
            {
                *outp++ = ' ';
                *outp++ = ' ';
                *outp++ = ' ';
            }
        }

        if (ascii)
        {
            *outp++ = ' ';
            *outp++ = ' ';

            /* display data in ascii */
            while (--alen >= 0)
            {
                uint8_t uc = *ap++;

                *outp++ = ((uc >= 0x20) && (uc < 0x7f)) ? uc : '.';
            }
        }

        /* output the line */
        *outp++ = '\n';
        //print_line(outbuf, outp - &outbuf[0]);
        printf("%s\n", outbuf);

        memset(outbuf, 0, sizeof(outbuf));
        outp = &outbuf[0];
        len -= HEX_BYTES_PER_LINE;
        line++;
    } /* while (len > 0) */
    return;
}

static int hex2byte(const char *hex)
{
	int a, b;

	a = net_ascii_to_hex(*hex++);
	if (a < 0)
		return -1;

	b = net_ascii_to_hex(*hex++);
	if (b < 0)
		return -1;

	return (a << 4) | b;
}

static int hexstr2bin(const char *hex, uint8_t *buf, size_t len)
{
	int a;
	const char *ipos = hex;
	uint8_t *opos = buf;

    while ((ipos - hex) < len)
    {
        /* skip delimiter */
        while (*ipos == ':' || *ipos == '.' || *ipos == '-' || *ipos == ' ')
        {
            ipos++;
            if (ipos - hex >= len)
            {
                goto end;
            }
        }

		a = hex2byte(ipos);
		if (a < 0)
			return -1;

		*opos++ = a;
		ipos += 2;
	}

end:
	return (opos - buf);
}

int32_t net_ether_aton(const char *orig, uint8_t *eth)
{
  const char *bufp;
  int i;

  i = 0;
  for(bufp = orig; *bufp != '\0'; ++bufp) {
    unsigned int val;
    unsigned char c = *bufp++;
    if (isdigit(c)) val = c - '0';
    else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
    else break;

    val <<= 4;
    c = *bufp++;
    if (isdigit(c)) val |= c - '0';
    else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
    else break;

    eth[i] = (unsigned char) (val & 0377);
    if(++i == 6) //MAC_LEN
    {
        /* That's it.  Any trailing junk? */
        if (*bufp != '\0') {
            //QCLI_Printf(qcli_wlan_group, "iw_ether_aton(%s): trailing junk!\r\n", orig);
            return(-1);
        }
        return(0);
    }
    if (*bufp != ':')
        break;
  }
  return(-1);
}

uint8_t net_ascii_to_hex(char val)
{
    if('0' <= val && '9' >= val)
    {
        return (uint8_t)(val - '0');
    }
    else if('a' <= val && 'f' >= val)
    {
        return (uint8_t)((val - 'a') + 0x0a);
    }
    else if('A' <= val && 'F' >= val)
    {
        return (uint8_t)((val - 'A') + 0x0a);
    }
    return 0xff;/* Error */
}

int hwaddr_pton(const char *txt, uint8_t *addr, size_t buflen)
{
    int i;
    const char *pos = txt;

    if (txt == NULL || addr == NULL || buflen < __QAPI_WLAN_MAC_LEN)
    {
        return -1;
    }

    for (i = 0; i < __QAPI_WLAN_MAC_LEN; i++)
    {
        int a, b;

        while (*pos == ':' || *pos == '.' || *pos == '-')
        {
            pos++;
        }

        a = net_ascii_to_hex(*pos++);
        if (a < 0)
            return -1;
        b = net_ascii_to_hex(*pos++);
        if (b < 0)
            return -1;
        *addr++ = (a << 4) | b;
    }

    return 0;
}

void eth_rx_thread(void *arg)
{
    fd_set read_fd_set;
	fd_set write_fd_set;
	fd_set error_fd_set;
	int max_fd = -1;
    int ret_val = -1;
    struct sockaddr_in from_addr;
    int32_t fromlen;
    int32_t received;
    struct eth_hdr *ethhdr;
    struct ip_hdr *iphdr;
    struct netif *netif = NULL;
    char *buf = NULL;
    int sock = QAPI_ERROR;
    uint8_t eth_proto = ETHPROTO_EAP;
    struct timeval time_out;

    netif = get_netif_by_device(STA_DEVICE);

    buf = malloc(ETH_RAW_RX_BUFFER_SIZE);
    if (buf == NULL)
    {
        printf("ERROR: No memory\n");
        goto end;
    }

    if(eth_raw_rx_protocol == ETHTYPE_EAP){
        eth_proto = ETHPROTO_EAP;
    }
    else if(eth_raw_rx_protocol == ETHTYPE_IP){
        eth_proto = ETHPROTO_IP;
    }
    else{
        printf("doesn't support for this ether type\n");
        goto end;
    }
    /* Open an socket */
    sock = socket(AF_PACKET, SOCK_RAW, eth_proto);
    if (sock == QAPI_ERROR)
    {
        printf("ERROR: Failed to create socket\n");
        goto end;
    }
    printf("****************************************************\n");
    printf("Ethernet RX Test\n");
    printf("****************************************************\n");
    printf(" EtherType: 0x%04x\n", eth_raw_rx_protocol);
    printf("Type \"eth rx -q\" to termintate test.\n");
    printf("****************************************************\n");

    memset(&from_addr, 0, sizeof(from_addr));
    fromlen = sizeof(struct sockaddr_in);

    /* Receive loop */
    while (!eth_rx_quit)
    {
        max_fd = -1;
        FD_ZERO(&read_fd_set);
        FD_ZERO(&write_fd_set);
        FD_ZERO(&error_fd_set);

        /*???*/
        if(sock >= 0)
        {
            FD_SET(sock, &read_fd_set);
            max_fd = (sock > max_fd) ? sock : max_fd;
        }
        time_out.tv_sec = 0;
        time_out.tv_usec = 100;
        ret_val = lwip_select(max_fd + 1, &read_fd_set, &write_fd_set, &error_fd_set, &time_out);

        if (eth_rx_quit)
        {
            goto end;
        }

        if (ret_val > 0)
        {
            /*??????*/
            if ((sock >= 0) && FD_ISSET(sock, &read_fd_set))
            {
                while(1)
                {
                    received = lwip_recvfrom(sock, buf, ETH_RAW_RX_BUFFER_SIZE, 0, (struct sockaddr *)&from_addr, &fromlen);
                    //printf( "printf lwip_recvfrom lan msg\n");
                    if (received > 0)
                    {
                        /*IPv4*/
                        if(eth_raw_rx_protocol == ETHTYPE_IP){
                            iphdr = (struct ip_hdr *)buf;
                            ip_addr_t *ip_local_addr = (ip_addr_t *)netif_ip_addr4(netif);

                            if (iphdr->dest.addr == ip_local_addr->u_addr.ip4.addr){
                                info_printf("STA IPv4: %s, start to dump\n", ipaddr_ntoa(ip_local_addr));
                                HEXDUMP((char *)buf, received, true, false);
                            }
                        }
                        else{
                            ethhdr = (struct eth_hdr *)buf;      
                            printf("Received %d bytes from %02x:%02x:%02x:%02x:%02x:%02x\n", received,
                                ethhdr->src.addr[0], ethhdr->src.addr[1], ethhdr->src.addr[2], ethhdr->src.addr[3], ethhdr->src.addr[4], ethhdr->src.addr[5]);
                            HEXDUMP((char *)buf, received, true, false);
                        }
                    }
                    break;
                }
            }
        }	
    }
end:
    if (buf)
    {
        free(buf);
    }

    if (sock != QAPI_ERROR)
    {
        closesocket(sock);
    }

    vTaskDelete(NULL);
}
static qapi_Status_t eth_rx(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{

    int rc = QAPI_ERROR;
    struct eth_hdr *eth;

    int i = 0;
    
    uint32_t status = pdPASS;
    TaskHandle_t eth_rx_task_handle;
    eth_raw_rx_protocol = ETHTYPE_EAP; /*default to receive EAP*/

    for (i = 1; i < Parameter_Count; i++)
    {
        if (Parameter_List[i].String_Value[0] == '-')
        {
            switch (Parameter_List[i].String_Value[1])
            {
                case 'p':   /* -p 0x888e */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid ||
                        (eth_raw_rx_protocol = Parameter_List[i].Integer_Value) < 0x600)
                    {
                        printf("ERROR: Invalid etherType: %s\n", Parameter_List[i].String_Value);
                        goto end;
                    }
                    break;

                case 'q':   /* -q */
                    eth_rx_quit = 1;
                    rc = QAPI_OK;
                    printf("eth rx session closed.\n");
                    goto end;

                default:
                    printf("ERROR: Unknown option: %s\n", Parameter_List[i].String_Value);
                    goto end;
            }
        }
        else
        {
            printf("ERROR: Unknown option: %s\n", Parameter_List[i].String_Value);
            goto end;
        }

        if (i == Parameter_Count)
        {
            printf("What is value of %s?\n", Parameter_List[i-1].String_Value);
            goto end;
        }
    }   /* for */

    if(eth_rx_quit == 0){
        printf("ERROR: only one rx session supported for eth rx demo, please close the previous one first.\n");
        goto end;
    }

    eth_rx_quit = 0;

    status = (uint32_t)nt_qurt_thread_create(eth_rx_thread, "eth_rx_task", 1024, NULL, 6, &eth_rx_task_handle);
    
    if(status){
        rc = QAPI_OK;
        goto end;
    }
    /* Bind 
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sll_family = AF_PACKET;
    local_addr.sll_protocol = htons(protocol);
    if (ifname)
    {
        local_addr.sll_ifindex = rc = qapi_Net_Interface_Get_Ifindex(ifname);
        if (rc < 0)
        {
            PRINTF("ERROR: Failed to get ifIndex\n");
            goto end;
        }
    }
    
    rc = qapi_bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_ll));
    if (rc != QAPI_OK)
    {
        PRINTF("ERROR: Socket bind error.\n");
        goto end;
    }
    */
    /* ------ Start test.----------- */
    
end:
    if (rc != QAPI_OK)
    {
        return QAPI_ERROR;
    }

    return QAPI_OK;
}
static qapi_Status_t eth_tx(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    char *ifname = NULL;
    char *payload = NULL;
    int sock = QAPI_ERROR;
    int rc = QAPI_ERROR;
    int i;
    struct sockaddr_storage to;
    struct netif *netif = NULL;
    struct eth_hdr *eth;
    char *da;
    uint8_t *srcmac;
    uint8_t eth_proto = ETHPROTO_EAP;
    uint8_t eapol[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* DA */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* SA */
        0x88, 0x8e,                         /* EtherType */
        0x01,           /* EAPOL protocol version */
        0x00,           /* EAPOL type: EAPOL-Packet */ 
        0x00, 0x05,     /* EAPOL payload length */
        0x01,           /* EAP code: request */
        0xaa,           /* EAP id */
        0x00, 0x05,     /* EAP length */
        0x01            /* EAP type: identity */
    };
    /* default to send EAPOL frame */
    uint8_t *pkt = eapol;
    int len = sizeof(eapol);
    uint16_t protocol = ETHTYPE_EAP;
    struct sockaddr_storage from;
    netif = get_netif_by_device(STA_DEVICE);

    if (Parameter_Count < 2)
    {
        eth_help();
        goto end;
    }

    da = Parameter_List[1].String_Value;

    for (i = 2; i < Parameter_Count; i++)
    {
        if (Parameter_List[i].String_Value[0] == '-')
        {
            switch (Parameter_List[i].String_Value[1])
            {
                case 'd':   /* -d "00 01 02 03" */
                    i++;
                    payload = Parameter_List[i].String_Value;
                    break;

                case 'p':   /* -p 0x888e */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid ||
                        (protocol = Parameter_List[i].Integer_Value) < 0x600)
                    {
                        printf("ERROR: Invalid etherType: %s\n", Parameter_List[i].String_Value);
                        goto end;
                    }
                    break;

                default:
                    printf("ERROR: Unknown option: %s\n", Parameter_List[i].String_Value);
                    goto end;
            }
        }
        else
        {
            printf("ERROR: Unknown option: %s\n", Parameter_List[i].String_Value);
            goto end;
        }

        if (i == Parameter_Count)
        {
            printf("What is value of %s?\n", Parameter_List[i-1].String_Value);
            goto end;
        }
    }   /* for */

    if (payload)
    {
        len = strlen(payload) + sizeof(struct eth_hdr);
        pkt = (uint8_t*)malloc(len);
        
        if (pkt == NULL)
        {
            printf("ERROR: No memory\n");
            goto end;
        }

        memset(pkt, 0 ,len);
        
        rc = hexstr2bin(payload, pkt + sizeof(struct eth_hdr), strlen(payload));
        if (rc < 0)
        {
            printf("ERROR: Invalid data string: %s\n", payload);
            goto end;
        }
        len = rc + sizeof(struct eth_hdr);
    }
    /*
    
    ip_addr_t *ip_local_addr = (ip_addr_t *)netif_ip_addr4(netif);

    struct sockaddr_in *from4 = (struct sockaddr_in*)&from;
    from4->sin_len = sizeof(struct sockaddr_in);
    from4->sin_family = AF_INET;
    from4->sin_port = htons(0);
    inet_addr_from_ip4addr(&(from4->sin_addr), ip_2_ip4(ip_local_addr));
    */
    if(protocol == ETHTYPE_EAP){
        eth_proto = ETHPROTO_EAP;
    }
    else if(protocol == ETHTYPE_IP){
        eth_proto = ETHPROTO_IP;
    }
    else{
        printf("not supported ether type\n");
        goto end;
    }
    /* Open an socket*/
    sock = socket(AF_PACKET, SOCK_RAW, eth_proto);
    if (sock == QAPI_ERROR)
    {
        printf("ERROR: Failed to create socket\n");
        goto end;
    }
    
    eth = (struct eth_hdr *)pkt;

    /* DA */
    rc = net_ether_aton(da, eth->dest.addr);  
    if (rc != QAPI_OK)
    {
        printf("ERROR: Invalid MAC address\n");
        goto end;
    }

    /* SA */
    memcpy(eth->src.addr, netif->hwaddr, 6);

    eth->type = htons(protocol);
    
    /* send it */
    rc = sendto(sock, pkt, len, 0, (struct sockaddr*)&to, sizeof(to));
    if (rc < 0)
    {
        printf("ERROR: Failed to send (%d)\n", rc);
        goto end;
    }

    printf("\nSent %d bytes to %s\n", rc, da); 
    rc = QAPI_OK;

end:
    if (pkt && pkt != eapol)
    {
        free(pkt);
    }

    if (sock != QAPI_ERROR)
    {
        closesocket(sock);
    }

    if (rc != QAPI_OK)
    {
        return QAPI_ERROR;
    }

    return QAPI_OK;
}
static void eth_help(void)
{
    printf("eth tx <dest mac addr> [-d <data bytes>] [-p <etherType>]\n"); 
    printf("eth rx [-p <etherType>] [-q]\n"); 
    printf("Examples:\n");
    printf(" eth tx 00:11:22:33:44:55 -p 0x888e -d \"01 06 12 05 01 ab 00 05 01\"\n");
    printf(" eth rx -p 0x888e\n");
    printf(" eth rx -q\n");
}
static qapi_Status_t eth(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;

    if (Parameter_Count < 1)
    {
        eth_help();
        return QAPI_ERROR;
    }

    if (strncmp(Parameter_List[0].String_Value, "tx", 1) == 0)
    {
        status = eth_tx(Parameter_Count, Parameter_List);
    }
    else
    if (strncmp(Parameter_List[0].String_Value, "rx", 1) == 0)
    {
        status = eth_rx(Parameter_Count, Parameter_List);
    }
    else
    {
        printf("ERROR: Unknown command: %s\n", Parameter_List[0].String_Value);
        status = QAPI_ERROR;
    }

    return status;
}
#endif /*CONFIG_SUPPORT_LWIP_RAW_SOCKET*/

const QAPI_Console_Command_t net_shell_cmds[] = {
    // cmd_function    cmd_string               usage_string             description
    {ifconfig, "ifconfig", "\n\nifconfig [interface] [ipv4addr] [subnetmask] [default_gateway]\n",
     "\nShow/Configure one or all network interface"},
    {dhcpv4c, "dhcpv4c", "\n\ndhcpv4c <interface> <new|release>\n",
     "\nDHCPv4 Client: Acquire an IPv4 address using Dynamic Host Configuration Protocol v4"},
    {dnsc, "dnsc",
     "\ndnsc:  show the current list of dns servers \n"
     "dnsc addsvr <ip>: add a DNS server \n"
     "dnsc delsvr <ip>: delete a DNS server \n"
     "dnsc gethostbyname <hostname>: resolve a hostname (string) into an IP address \n"
     "dnsc gethostbyname2 <hostname> <iptype>: like dns_gethostbyname, but returned address type can be controlled\n"
     "                                       iptype: v4, v6 , v4v6, v6v4 \n",
     "\n Resolve a hostname (string) into an IP address"},
    {pingv4, "ping", "\n\nping <host> [ -s (packet lengh)] [-c (count)] [-d (delay(ms))]\n",
     "\nSend ICMP ECHO_REQUEST to network hosts in IPv4/IPv6 network"},
    {iperf, "iperf", "\n\nUsage: iperf [-s|-c host] [-p][-i][-t][-n][-l][-b][-S]\n", "\niperf test"},
    {iperf_quit, "iperf_quit", "\n\nUsage: iperf quit\n", "\nquit iperf"},
#if LWIP_IPV6
    {prefix_set, "prefix_set", "\n\nprefixset [prefix]\n",
     "\nSet prefix to AP interface or show the current prefix of AP interface\n",
     "\nTo show the current prefix: prefix_set\n", "\nTo set the prefix: prefix_set 2001:db8:0:0\n"},
#endif
    {prefix_v6, "prefix", "\n\nprefix <interface> [(<ipv6addr> <prefixlen> <prefix_lifetime> <valid_lifetime>)]\n",
     "\nSend prefix to network hosts in IPv6 network"},
#ifdef CONFIG_NET_SSL_DEMO
    {ssl_client, "ssl_client", "\n\nUsage: ssl_client [param1] [value] [param2] [value]...\n", "\nssl client command"},
    {ssl_server, "ssl_server", "\n\nUsage: ssl_server [param1] [value] [param2] [value]...\n", "\nssl server command"},
    {ssl_quit, "ssl_quit", "\n\nUsage: ssl_quit\n", "\nssl quit"},
#endif
    {dhcpv4s, "dhcpv4s", "\n\ndhcpv4s <interface> <start|stop|pool> <start_ip> <end_ip> [<lease_time_minute>]\n",
     "\nDHCPv4 Server: Set up and configure Dynamic Host Configuration Protocol v4 server"},

#ifdef CONFIG_HTTP_CLIENT_DEMO
    {httpc_command_handler, "httpc",
     "\n\nhttpc [start|stop]\n"
     "httpc [connect|disconnect|get|post|put|patch] <...>\n",
     "\nHTTP Client: Perform Hypertext Transport protocol client operations.\n"
     "Type command name to get more info on usage. For example \"httpc get\".\n"},
#endif

#ifdef CONFIG_HTTP_SERVER_DEMO
    {httpd_command_handler, "httpd",
     "\n\nhttpd [enable|disable] [server_port]\n"
     "\nHTTP SERVER: Perform Hypertext Transport protocol server operations.\n"},
    {syscfg_command, "syscfg",
     "\n\nsyscfg\n"
     "\nsyscfg: get ssid and passwod of http server config.\n"},
#endif

#ifdef CONFIG_MQTT_CLIENT_DEMO
    {mqttc_demo, "mqttc", "\n\nType \"mqttc\" to get more info on usage\n",
     "\nMQTT Client: Set up and configure MQ Telemetry Transport client"},
#endif

    {pmtud_demo, "pmtud", "\n\nType \"pmtud\" to get mtu on the path to dst\n",
     "\nMTUD Client: Type command. For example \"pmtud --dst\".\n"},
#ifdef CONFIG_SNTP_CLIENT_DEMO
    {sntpc, "sntpc",
     "\n\nsntpc\n"
     "sntpc [start|stop]\n"
     "sntpc setOpMode <0|1>\n"
     "sntpc setServer <IP addr|name> [id]",
     "\nSNTP client start or stop, configure"},
#endif
    {socketstat, "socketstat", "\n\nsocketstat\n", "\nShow the socket count in lwip stack"},
#ifdef CONFIG_SUPPORT_LWIP_RAW_SOCKET
    {eth,		"eth",	"\n\neth\n" \
                            "eth tx <dest mac addr> [-d <data bytes>] [-p <etherType>]\n" \
                            "eth rx [-p <etherType>] [-q] [-f <1|0, 1 for not consuming eth header for ip>]\n" \
                            "Examples:\n" \
                                " eth tx 00:11:22:33:44:55 -p 0x888e -d \"01 06 12 05 01 ab 00 05 01\"\n"\
                                "eth rx -p 0x888e\n"\
                                "eth rx -q",
                                "\neth tx/rx demo"},
#endif /* CONFIG_SUPPORT_LWIP_RAW_SOCKET */
};

const QAPI_Console_Command_Group_t net_shell_cmd_group = {
    NET_SHELL_GROUP_NAME, sizeof(net_shell_cmds) / sizeof(QAPI_Console_Command_t), net_shell_cmds};

QAPI_Console_Group_Handle_t net_shell_cmd_group_handle;

void net_shell_init(void)
{
#if LWIP_IPV4
    ip_addr_t staipaddr = IPADDR4_INIT_BYTES(192, 168, 0, 100);  // IPV4_adderss for netif (STA)
    ip_addr_t apipaddr = IPADDR4_INIT_BYTES(192, 168, 0, 1);     // IPV4_adderss for netif // (APor GW)
    ip_addr_t net_mask = IPADDR4_INIT_BYTES(255, 255, 255, 0);   // IPV4_netmask adderss for netif (AP)

    /* Static Ip assigned */
    ip_addr_copy(default_ip_address[0], staipaddr);
    ip_addr_copy(default_ip_address[1], apipaddr);
    ip_addr_copy(default_gw[0], apipaddr);
    ip_addr_copy(default_gw[1], apipaddr);
    ip_addr_copy(default_netmask[0], net_mask);
    ip_addr_copy(default_netmask[1], net_mask);

#endif /* LWIP_IPV4 */

    net_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &net_shell_cmd_group);
    if (net_shell_cmd_group_handle) {
        info_printf("Net Registered\n");
    }
}
