/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/* Standard includes. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "qapi_types.h"
#include "qapi_status.h"
#include "qcli.h"
#include "qcli_api.h"
#include "qcli_pal.h"
#include "qcli_util.h"

#include "safeAPI.h"
#include "usd_demo.h"

int my_hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

int my_hex2byte(const char *hex)
{
	int a, b;
	a = my_hex2num(*hex++);
	if (a < 0)
		return -1;
	b = my_hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

int my_hexstr2bin(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = my_hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}

struct wpabuf * wpabuf_parse_bin(const char *buf)
{
	size_t len;
	struct wpabuf *ret;

	len = os_strlen(buf);
	if (len & 0x01)
		return NULL;
	len /= 2;

	ret = wpabuf_alloc(len);
	if (ret == NULL)
		return NULL;

	if (my_hexstr2bin(buf, wpabuf_put(ret, len), len)) {
		wpabuf_free(ret);
		return NULL;
	}

	return ret;
}

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define USD_CLIENT_PRINTF(...)     printf(__VA_ARGS__)

#define USD_PRINTF_HANDLE qcli_usd_group

QAPI_Console_Group_Handle_t qcli_usd_group;   /* Handle for our QCLI Command Group. */

/**********************************************************************************************************/
/* Function Declarations                                                                                  */
/**********************************************************************************************************/
static qapi_Status_t Command_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_cancel_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_update_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_subscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_cancel_subscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_transmit(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

const QAPI_Console_Command_t USD_Command_List[] = 
{
    /* cmd_function             cmd_string           usage_string             description */
	{Command_init,              "init",              "\n\ninit",              "USD service interface: init"},
    {Command_publish,           "publish",           "\n\npublish",           "USD service interface: publish a service"},
    {Command_cancel_publish,    "cancel_publish",    "\n\ncancel_publish",    "USD service interface: cancel_publish"},
    {Command_update_publish,    "update_publish",    "\n\nupdate_publish",    "USD service interface: update_publish"},
    {Command_subscribe,         "subscribe",         "\n\nsubscribe",         "USD service interface: subscribe a service"},
    {Command_cancel_subscribe,  "cancel_subscribe",  "\n\ncancel_subscribe",  "USD service interface: cancel_subscribe"},
    {Command_transmit,          "transmit",          "\n\ntransmit",          "USD service interface: transmit"},
};

const QAPI_Console_Command_Group_t USD_Command_Group =
{
    "USD",  /* Unsynchorized Service Discovery Unit */
    sizeof(USD_Command_List) / sizeof(QAPI_Console_Command_t),
    USD_Command_List,
};

struct nan_de *de;
/**********************************************************************************************************/
/* Function Definitions                                                                                   */
/**********************************************************************************************************/
/* This function is used to register the MPU Command Group with QCLI   */
void Initialize_USD_Demo(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    USD_PRINTF_HANDLE = QAPI_Console_Register_Command_Group(NULL, &USD_Command_Group);
    if (USD_PRINTF_HANDLE) {
        QCLI_Printf(USD_PRINTF_HANDLE, "USD Registered \n");
    }
}

static void usd_client_help()
{
	USD_CLIENT_PRINTF("Usage:\n");
	USD_CLIENT_PRINTF("usd init 0 0\n");
    USD_CLIENT_PRINTF("usd publish <service_name> <srv_proto_type> <ssi>\n");
    USD_CLIENT_PRINTF("usd subscribe <service_name> <srv_proto_type> <ssi>\n");
    USD_CLIENT_PRINTF("usd update_publish <publish_id> <ssi>\n");
    USD_CLIENT_PRINTF("usd transmit <handle> <req_instance_id> <address> <ssi>\n");
    USD_CLIENT_PRINTF("usd cancle_subscribe <subscribe_id>\n");
    USD_CLIENT_PRINTF("usd cancle_publish <subscribe_id>\n");
}

static qapi_Status_t Command_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	struct nan_callbacks cb;
	os_memset(&cb, 0, sizeof(cb));
	cb.discovery_result = nan_de_discovery_result;
	cb.replied = nan_de_replied;
	cb.publish_terminated = nan_de_publish_terminated;
	cb.subscribe_terminated = nan_de_subscribe_terminated;
	cb.receive = nan_de_receive;

	de = nan_de_init(NULL, false, &cb);
	if (!de)
	{
		USD_CLIENT_PRINTF("fail to init nan de\n");
		return -1;
	}
	return 0;
}

static qapi_Status_t Command_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	int publish_id;
	struct nan_publish_params params;
	enum nan_service_protocol_type srv_proto_type = 0;
	char *service_name = NULL;
	struct wpabuf *ssi = NULL;
	struct wpabuf *elems = NULL;

	memset(&params, 0, sizeof(params));
	/* USD shall use both solicited and unsolicited transmissions */
	params.unsolicited = true;
	params.solicited = true;
	/* USD shall require FSD without GAS */
	params.fsd = true;
	params.freq = NAN_USD_DEFAULT_FREQ;

	if (Parameter_Count < 2)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}

	/* usd publish <service_name> <srv_proto_type> <ssi> */
	service_name = Parameter_List[1].String_Value;
	srv_proto_type = Parameter_List[2].Integer_Value;
	ssi = wpabuf_parse_bin(Parameter_List[3].String_Value);
	if (!ssi)
	{
		USD_CLIENT_PRINTF("fail to parse ssi\n");
		goto fail;
	}
	publish_id = nan_de_publish(de, service_name, srv_proto_type,
					  ssi, elems, &params);
	if (publish_id > 0)
		USD_CLIENT_PRINTF("success to create publish: %d\n", publish_id);
	else
		USD_CLIENT_PRINTF("fail to create publish\n");
fail:
	wpabuf_free(ssi);
	return QAPI_OK;

}


static qapi_Status_t Command_cancel_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	int publish_id = 0;

	if (Parameter_Count < 2)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}

	/* usd cancle_publish <subscribe_id> */
	publish_id = Parameter_List[1].Integer_Value;

	if (publish_id <= 0)
	{
		USD_CLIENT_PRINTF("Invalid or missing NAN_CANCEL_PUBLISH publish_id\n");
		return QAPI_ERR_INVALID_PARAM;
	}

	nan_de_cancel_publish(de, publish_id);
    return QAPI_OK;
}

static qapi_Status_t Command_update_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	int publish_id = 0;
	struct wpabuf *ssi = NULL;

	if (Parameter_Count < 2)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}

	/* usd update_publish <publish_id> <ssi>*/
	publish_id = Parameter_List[1].Integer_Value;
	ssi = wpabuf_parse_bin(Parameter_List[1].String_Value);

	if (publish_id <= 0)
	{
		USD_CLIENT_PRINTF("Invalid or missing NAN_CANCEL_PUBLISH publish_id\n");
		goto fail;;
	}

	nan_de_update_publish(de, publish_id, ssi);
	
fail:
	wpabuf_free(ssi);
	return QAPI_OK;

}


static qapi_Status_t Command_subscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	int subscribe_id;
	struct nan_subscribe_params params;
	const char *service_name = NULL;
	struct wpabuf *ssi = NULL;
	enum nan_service_protocol_type srv_proto_type = 0;
	const struct wpabuf *elems = NULL;

	if (Parameter_Count < 2)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}

	memset(&params, 0, sizeof(params));
	params.freq = NAN_USD_DEFAULT_FREQ;
	
	/* usd subscribe <service_name> <srv_proto_type> <ssi> */
	service_name = Parameter_List[1].String_Value;
	srv_proto_type = Parameter_List[2].Integer_Value;
	ssi = wpabuf_parse_bin(Parameter_List[3].String_Value);

	subscribe_id = nan_de_subscribe(de, service_name,
					      srv_proto_type, ssi, elems,
					      &params);
	if (subscribe_id <= 0)
	{
		USD_CLIENT_PRINTF("Invalid or missing NAN_CANCEL_PUBLISH publish_id\n");
	}

	wpabuf_free(ssi);
	
    return QAPI_OK;
}


static qapi_Status_t Command_cancel_subscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	int subscribe_id = 0;

	if (Parameter_Count < 2)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}
	
	/* usd cancle_subscribe <subscribe_id> */
	subscribe_id = Parameter_List[1].Integer_Value;

	if (subscribe_id <= 0) {
		USD_CLIENT_PRINTF("CTRL: Invalid or missing NAN_CANCEL_SUBSCRIBE subscribe_id\n");
		return QAPI_ERR_INVALID_PARAM;
	}

	nan_de_cancel_subscribe(de, subscribe_id);
    return QAPI_OK;
}


static qapi_Status_t Command_transmit(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	int req_instance_id = 0;
	int handle = 0;
	struct wpabuf *ssi = NULL;
	uint8_t peer_addr[ETH_ALEN];

	if (Parameter_Count < 2)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}

	memset(peer_addr, 0, ETH_ALEN);
	/* usd transmit <handle> <req_instance_id> <address> <ssi>*/
	handle = Parameter_List[1].Integer_Value;
	req_instance_id = Parameter_List[2].Integer_Value;
	hwaddr_aton(Parameter_List[3].String_Value, peer_addr);
	ssi = wpabuf_parse_bin(Parameter_List[4].String_Value);
	
	nan_de_transmit(de, handle, ssi, NULL, peer_addr,
				    req_instance_id);
    return QAPI_OK;
}

qapi_Status_t usd_demo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	char *cmd;

	if (Parameter_Count < 2 || Parameter_List == NULL)
	{
		usd_client_help();
		return QAPI_ERR_INVALID_PARAM;
	}

	cmd = Parameter_List[0].String_Value;

	/* usd publish <service_name> <srv_proto_type> <ssi>*/
	if (strcmp(cmd, "init") == 0)
	{
		Command_init(Parameter_Count, Parameter_List);
	}
	else if(strcmp(cmd, "publish") == 0)
	{
		Command_publish(Parameter_Count, Parameter_List);
	}
	/* usd subscribe <service_name> <srv_proto_type> <ssi> */
	else if (strcmp(cmd, "subscribe") == 0)
	{
		Command_subscribe(Parameter_Count, Parameter_List);
	}
	/* usd update_publish <publish_id> <ssi>*/
	else if (strcmp(cmd, "update_publish") == 0)
	{
		Command_update_publish(Parameter_Count, Parameter_List);
	}
	/* usd transmit <handle> <req_instance_id> <address> <ssi>*/
	else if (strcmp(cmd, "transmit") == 0)
	{
		Command_transmit(Parameter_Count, Parameter_List);
	}
	/* usd cancle_subscribe <subscribe_id> */
	else if (strcmp(cmd, "cancle_subscribe") == 0)
	{
		Command_cancel_subscribe(Parameter_Count, Parameter_List);
	}
	/* usd cancle_publish <subscribe_id> */
	else if (strcmp(cmd, "cancle_publish") == 0)
	{
		Command_cancel_publish(Parameter_Count, Parameter_List);
	}
	else if (strcmp(cmd, "help") == 0)
	{
		usd_client_help();
	}
	else 
	{
		USD_CLIENT_PRINTF("Invalid command %s\n", cmd);
		usd_client_help();
	}

    return QAPI_ERR_INVALID_PARAM;
}
