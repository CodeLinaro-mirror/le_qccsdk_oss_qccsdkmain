/*
 * NAN Discovery Engine
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef NAN_DE_H
#define NAN_DE_H

#include "nan.h"
#include "wlan_dev.h"
//#include "../../dpm/inc/os_al.h"
#include "../../../../../comp/supplicant/src/utils/os.h"

#include "nt_logger_api.h"

typedef unsigned char u8;

//struct wpabuf {
//	uint8_t size; /* total size of the allocated buffer */
//	uint8_t used; /* length of data in the buffer */
//	uint8_t *ext_data; /* pointer to external data; NULL if data follows
//		       * struct wpabuf */
//	/* optionally followed by the allocated buffer */
//};



/* Maximum number of active local publish and subscribe instances */
#ifndef NAN_DE_MAX_SERVICE
#define NAN_DE_MAX_SERVICE 20
#endif /* NAN_DE_MAX_SERVICE */
#define ETH_ALEN 6

struct nan_de;

enum nan_de_reason {
	NAN_DE_REASON_TIMEOUT,
	NAN_DE_REASON_USER_REQUEST,
	NAN_DE_REASON_FAILURE,
};

enum nan_de_service_type {
	NAN_DE_PUBLISH,
	NAN_DE_SUBSCRIBE,
};

struct nan_publish_params {
	/* configuration_parameters */

	/* Publish type */
	bool unsolicited;
	bool solicited;

	/* Solicited transmission type */
	bool solicited_multicast;

	/* Time to live (in seconds); 0 = one TX only */
	unsigned int ttl;

	/* Event conditions */
	bool disable_events;

	/* Further Service Discovery flag */
	bool fsd;

	/* Further Service Discovery function */
	bool fsd_gas;

	/* Default frequency (defaultPublishChannel) */
	unsigned int freq;

	/* Multi-channel frequencies (publishChannelList) */
	const int *freq_list;

	/* Announcement period in ms; 0 = use default */
	unsigned int announcement_period;
};

struct nan_subscribe_params {
	/* configuration_parameters */

	/* Subscribe type */
	bool active;

	/* Time to live (in seconds); 0 = until first result */
	unsigned int ttl;

	/* Selected frequency */
	unsigned int freq;

	/* Query period in ms; 0 = use default */
	unsigned int query_period;
};

struct nan_callbacks {
	/*
	void *ctx;

	int (*tx)(void *ctx, unsigned int freq, unsigned int wait_time,
		  const uint8_t *dst, const uint8_t *src, const uint8_t *bssid,
		  const struct wpabuf *buf);
	int (*listen)(void *ctx, unsigned int freq, unsigned int duration);
	*/

	/* NAN DE Events */
	void (*discovery_result)(int subscribe_id,
				 enum nan_service_protocol_type srv_proto_type,
				 const uint8_t *ssi, uint8_t ssi_len,
				 int peer_publish_id,
				 const uint8_t *peer_addr, bool fsd, bool fsd_gas);

	void (*replied)(int publish_id, const uint8_t *peer_addr,
			int peer_subscribe_id,
			enum nan_service_protocol_type srv_proto_type,
			const uint8_t *ssi, uint8_t ssi_len);

	void (*publish_terminated)(int publish_id,
				    enum nan_de_reason reason);

	void (*subscribe_terminated)(int subscribe_id,
				     enum nan_de_reason reason);

	void (*receive)(int id, int peer_instance_id,
			const uint8_t *ssi, uint8_t ssi_len,
			const uint8_t *peer_addr);
};

struct nan_de_service {
	int id;
	enum nan_de_service_type type;
	char *service_name;
	u8 service_id[NAN_SERVICE_ID_LEN];
	struct nan_publish_params publish;
	struct nan_subscribe_params subscribe;
	enum nan_service_protocol_type srv_proto_type;
	struct wpabuf *ssi;
	struct wpabuf *elems;
	struct os_reltime time_started;
	struct os_reltime end_time;
	struct os_reltime last_multicast;
	struct os_reltime first_discovered;
	struct os_reltime last_followup;
	bool needs_fsd;
	unsigned int freq;
	unsigned int default_freq;
	int *freq_list;

	/* pauseState information for Publish function */
	struct os_reltime pause_state_end;
	u8 sel_peer_id;
	u8 sel_peer_addr[ETH_ALEN];

	/* Publish state - channel iteration */
	bool in_multi_chan;
	bool first_multi_chan;
	int multi_chan_idx; /* index to freq_list[] */
	struct os_reltime next_publish_state;
	struct os_reltime next_publish_chan;
	unsigned int next_publish_duration;
};


struct nan_de {
	uint8 nmi[ETH_ALEN];
	bool ap;
	struct nan_callbacks cb;

	struct nan_de_service *service[NAN_DE_MAX_SERVICE];
	unsigned int num_service;

	int next_handle;

	unsigned int ext_listen_freq;
	unsigned int listen_freq;
	unsigned int tx_wait_status_freq;
	unsigned int tx_wait_end_freq;
};

struct nan_de * nan_de_init(const uint8_t *nmi, bool ap,
			    const struct nan_callbacks *cb);
void nan_de_flush(struct nan_de *de);
void nan_de_deinit(struct nan_de *de);

/*
void nan_de_listen_started(struct nan_de *de, unsigned int freq,
			   unsigned int duration);
*/
void nan_de_listen_ended(struct nan_de *de, unsigned int freq);
void nan_de_tx_status(struct nan_de *de, unsigned int freq, const uint8_t *dst);
void nan_de_tx_wait_ended(struct nan_de *de);

void nan_de_rx_sdf(struct nan_de *de, uint8_t*peer_addr, uint8_t *buf, int8_t len);

const uint8_t * nan_de_get_service_id(struct nan_de *de, int id);



/* Returns -1 on failure or >0 publish_id */
int nan_de_publish(struct nan_de *de, const char *service_name,
		   enum nan_service_protocol_type srv_proto_type,
		   const struct wpabuf *ssi, const struct wpabuf *elems,
		   struct nan_publish_params *params);

void nan_de_cancel_publish(struct nan_de *de, int publish_id);

int nan_de_update_publish(struct nan_de *de, int publish_id,
			  const struct wpabuf *ssi);



/* Returns -1 on failure or >0 subscribe_id */
int nan_de_subscribe(struct nan_de *de, const char *service_name,
		     enum nan_service_protocol_type srv_proto_type,
		     const struct wpabuf *ssi, const struct wpabuf *elems,
		     struct nan_subscribe_params *params);

void nan_de_cancel_subscribe(struct nan_de *de, int subscribe_id);

/* handle = publish_id or subscribe_id
 * req_instance_id = peer publish_id or subscribe_id */
int nan_de_transmit(struct nan_de *de, int handle,
		    const struct wpabuf *ssi, const struct wpabuf *elems,
		    u8 *peer_addr, u8 req_instance_id);

void nan_de_discovery_result(int subscribe_id, enum nan_service_protocol_type srv_proto_type,
			const u8 *ssi, u8 ssi_len, int peer_publish_id,
			const u8 *peer_addr, bool fsd, bool fsd_gas);
			
void nan_de_replied(int publish_id, const u8 *peer_addr,
			int peer_subscribe_id,
			enum nan_service_protocol_type srv_proto_type,
			const u8 *ssi, u8 ssi_len);

void nan_de_publish_terminated(int publish_id,
			enum nan_de_reason reason);

void nan_de_subscribe_terminated(int subscribe_id,
			enum nan_de_reason reason);

void nan_de_receive(int id, int peer_instance_id,
			const u8 *ssi, u8 ssi_len,
			const u8 *peer_addr);
#if 0

__attribute__((unused)) static inline void wpabuf_put_u8(struct wpabuf *buf, uint8_t data)
{
	uint8_t *pos = wpabuf_put(buf, 1);
	*pos = data;
}
/*

inline void ntbuf_put_u8(struct wpabuf *buf, uint8_t data)
{
	wpabuf_put_u8(buf, data);
}
*/


__attribute__((unused)) static inline void wpabuf_put_le16(struct wpabuf *buf, uint16_t data)
{
	uint8_t *pos = wpabuf_put(buf, 2);
	WPA_PUT_LE16(pos, data);
}
/*
inline void ntbuf_put_le16(struct wpabuf *buf, uint16_t data)
{
	wpabuf_put_le16(buf, data);
}*/

#endif

#if 0
/*
static inline void wpabuf_put_le32(struct wpabuf *buf, uint32_t data)
{
	uint8_t *pos = wpabuf_put(buf, 4);
	WPA_PUT_LE32(pos, data);
}
*/
/*

inline void nt_put_le32(struct wpabuf *buf, uint32_t data)
{
	wpabuf_put_le32(buf, data);
}
*/

#if 0
static inline void wpabuf_put_be16(struct wpabuf *buf, uint16_t data)
{
	uint8_t *pos = wpabuf_put(buf, 2);
	WPA_PUT_BE16(pos, data);
}
#endif 
/*

inline void ntbuf_put_be16(struct wpabuf *buf, uint16_t data)
{
	wpabuf_put_be16(buf, data);
}
*/

#endif

#if 0

__attribute__((unused)) static inline void wpabuf_put_be24(struct wpabuf *buf, uint32_t data)
{
	uint8_t *pos = wpabuf_put(buf, 3);
	WPA_PUT_BE24(pos, data);
}
/*

inline void ntbuf_put_be24(struct wpabuf *buf, uint32_t data)
{
	wpabuf_put_be24(buf, data);
}
*/

#endif


#if 0

__attribute__((unused)) static inline void wpabuf_put_be32(struct wpabuf *buf, uint32_t data)
{
	uint8_t *pos = wpabuf_put(buf, 4);
	WPA_PUT_BE32(pos, data);
}
/*
inline void ntbuf_put_be32(struct wpabuf *buf, uint32_t data)
{
	wpabuf_put_be32(buf, data);
}
*/

#endif


#endif /* NAN_DE_H */

