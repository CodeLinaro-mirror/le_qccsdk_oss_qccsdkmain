/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include "nan_de.h"
#include "common.h"
#include "wpabuf.h"

void Initialize_USD_Demo(void);

#if 0
static const char * hwaddr_parse(const char *txt, u8 *addr)
{
	size_t i;

	for (i = 0; i < ETH_ALEN; i++) {
		int a;

		a = my_hex2byte(txt);
		if (a < 0)
			return NULL;
		txt += 2;
		addr[i] = a;
		if (i < ETH_ALEN - 1 && *txt++ != ':')
			return NULL;
	}
	return txt;
}

int hwaddr_aton(const char *txt, u8 *addr)
{
	if (NULL != hwaddr_parse(txt, addr))
		return 0;
	else 
		return -1;
}

/**
 * wpabuf_parse_bin - Parse a null terminated string of binary data to a wpabuf
 * @buf: Buffer with null terminated string (hexdump) of binary data
 * Returns: wpabuf or %NULL on failure
 *
 * The string len must be a multiple of two and contain only hexadecimal digits.
 */
#endif


qapi_Status_t usd_demo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
