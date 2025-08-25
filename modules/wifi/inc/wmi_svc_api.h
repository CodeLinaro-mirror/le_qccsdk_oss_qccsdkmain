/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the API for the Wireless Module Interface (WMI) Service
//

#ifndef WMI_SVC_API_H_
#define WMI_SVC_API_H_

//#include <htc_buf.h>
#include <wmi.h>

#ifdef ATH_KF
#define EVT_PKT_IN_USE (1 << 0)
#define EVT_USE_CDR    (1 << 1)

#define EVT_PKT_IS_FREE(e) !((e)->Flags & EVT_PKT_IN_USE)
#define EVT_MARK_FREE(e)   (e)->Flags &= ~EVT_PKT_IN_USE;
#define EVT_MARK_INUSE(e)  (e)->Flags |= EVT_PKT_IN_USE

#define IS_EVT_CDR(e)    ((e)->Flags & EVT_USE_CDR)
#define EVT_CLEAR_CDR(e) (e)->Flags &= ~EVT_USE_CDR;
#define EVT_MARK_CDR(e)  (e)->Flags |= EVT_USE_CDR;

#define WMISVC_REV 2

#define WMI_SVC_MAX_BUFFERED_EVENT_SIZE 256  /* maximum length of buffered events */
#define WMI_SVC_MSG_SIZE                1536 /* maximum size of any WMI control or event message */

/* event classes */
typedef enum WMI_EVT_CLASS {
    WMI_EVT_CLASS_NONE = -1,
    WMI_EVT_CLASS_DIRECT_BUFFER = 0, /* lowest priority */
    WMI_EVT_CLASS_LOW = 1,
    WMI_EVT_CLASS_HIGH = 2,
    WMI_EVT_CLASS_CMD_REPLY = 3, /* highest priority */
    WMI_EVT_CLASS_MAX
} WMI_EVT_CLASS;

/* command handler callback when a message is dispatched */
typedef void (*WMI_CMD_HANDLER)(void *pContext,      /* application supplied context from dispatch table */
                                uint16_t Command,    /* command ID that was dispatched */
                                uint16_t info1,      /* meta data for command */
                                uint8_t *pCmdBuffer, /* command data, 256 bytes max, 32-bit aligned */
                                int Length);         /* length of command (excludes WMI header) */
#endif                                               // ATH_KF

typedef enum _WMI_FILTER_ACTION {
    WMI_EVT_FILTER_ALLOW = 0, /* allow event to be sent to the host */
    WMI_EVT_FILTER_DISCARD,   /* discard event */
    WMI_CMD_FILTER_ALLOW,     /* allow command to pass to the registered command handler */
    WMI_CMD_FILTER_HANDLED    /* command is handled in the filter, do not pass command */
} WMI_FILTER_ACTION;

#ifdef ATH_KF
/* event filter callback to allow applications to filter WMI events (i.e. WOW support) */
typedef WMI_FILTER_ACTION (*WMI_EVENT_FILTER_CB)(void *pContext,      /* app supplied context */
                                                 uint16_t EventID,    /* event ID */
                                                 uint16_t info,       /* the info param passed to WMI_SendEvent */
                                                 uint8_t *pEvtBuffer, /* pointer to event data, if needed */
                                                 int Length);         /* length of event data, if needed */

/* command filter callback to allow applications to filter WMI commands before they
 * are dispatched to registered handlers*/
typedef WMI_FILTER_ACTION (*WMI_CMD_FILTER_CB)(void *pContext,      /* app supplied context */
                                               uint16_t CmdID,      /* Command ID */
                                               uint8_t *pCmdBuffer, /* pointer to command data, if needed */
                                               int Length);         /* length of command data, if needed */

/* configuration settings for the WMI service */
typedef struct _WMI_SVC_CONFIG {
    int MaxCmdReplyEvts;             /* total buffers for command replies */
    int MaxLowPriEvts;               /* total buffers for low priority events */
    int MaxHiPriEvts;                /* total buffers for high priority events */
    int MaxDirectBuffers;            /* total resources for direct-buffer events */
    WMI_EVENT_FILTER_CB EventFilter; /* event filter function */
    void *EventFilterContext;        /* event filter context */
    WMI_CMD_FILTER_CB CmdFilter;     /* command filter function */
    void *CmdFilterContext;          /* command filter context */
} WMI_SVC_CONFIG;

/* command dispatch entry */
typedef struct _WMI_DISPATCH_ENTRY {
    WMI_CMD_HANDLER pCmdHandler; /* dispatch function */
    uint16_t CmdID;              /* WMI command to dispatch from */
    uint16_t CheckLength;        /* expected length of command, set to 0 to bypass check */
} WMI_DISPATCH_ENTRY;

/* dispatch table that is used to register a set of dispatch entries */
typedef struct _WMI_DISPATCH_TABLE {
    struct _WMI_DISPATCH_TABLE *pNext; /* next dispatch, WMI-reserved */
    uint16_t minCmd;                   /* min command in table */
    uint16_t maxCmd;                   /* max command in table */
    int NumberOfEntries;               /* number of elements pointed to by pTable */
    WMI_DISPATCH_ENTRY *pTable;        /* start of table */
} WMI_DISPATCH_TABLE;

#define WMI_DISPATCH_ENTRY_COUNT(table) (sizeof((table)) / sizeof(WMI_DISPATCH_ENTRY))

/* handy macro to declare a dispatch table */
#define WMI_DECLARE_DISPATCH_TABLE(name, dispatchEntries)                                  \
    WMI_DISPATCH_TABLE name = {NULL, 0, 0xffff, WMI_DISPATCH_ENTRY_COUNT(dispatchEntries), \
                               (WMI_DISPATCH_ENTRY *)(dispatchEntries)}

/* WMI_SET_..._COMMAND are used to set the min and max command ID values for a table.
 *  this allows for a more optimized parsing algo as each command is routed. */
#define WMI_SET_MIN_COMMAND(pDispTable, min) (pDispTable)->minCmd = (min)
#define WMI_SET_MAX_COMMAND(pDispTable, max) (pDispTable)->maxCmd = (max)

/* callback for the delivery of direct-buffer events */
typedef void (*WMI_DIRECT_BUFFER_COMPLETE_CB)(void *pContext);

typedef struct _WMI_DIRECT_EVT {
    WMI_DIRECT_BUFFER_COMPLETE_CB pCompletion; /* direct buffer delivery completion routine */
    void *pContext;                            /* context for completion */
} WMI_DIRECT_EVT;

typedef struct _WMI_BUFFER_EVT {
    uint8_t *pEvtPayload;
} WMI_BUFFER_EVT;

/* WMI event packet, used to manage the flow of events from the application to WMI */
typedef struct _WMI_EVT_PACKET {
    HTC_BUFFER HtcBuf; /* HTC buffer info used for delivering this event packet MUST BE FIRST ELEMENT */

    union {
        WMI_BUFFER_EVT Buffered; /* Used for buffered Event Classes.  Buffer shall be:
                                    1. Cache aligned
                                    2. 32-bit aligned
                                    3. WMI_SVC_MAX_BUFFERED_EVENT_SIZE bytes in length */

        WMI_DIRECT_EVT Direct; /* Used when Event Class is WMI_EVT_CLASS_DIRECT_BUFFER
                                  Callers that use direct buffers must provide enough headroom
                                  for WMI + HTC headers.
                               */
    } BufType;

    WMI_EVT_CLASS EventClass; /* the event class this packet belongs to */
    uint16_t Flags;           /* internal flags reserved for WMI */

} WMI_EVT_PACKET;

/* macro to get a pointer to the payload portion of a buffered event */
#define WMI_EVT_GET_BUFFERED_EVT_PTR(pEvt) (pEvt)->BufType.Buffered.pEvtPayload

#define HTC_BUF_HDL(pEvt) (&((pEvt)->HtcBuf))
#define WMI_EVT_ATTACH_DIRECT_BUFFER(pEvt, pBuffer, pCompletionCb, pCt) \
    {                                                                   \
        (pEvt)->BufType.Direct.pCompletion = (pCompletionCb);           \
        (pEvt)->BufType.Direct.pContext = (pCt);                        \
        HTC_BUF_HDL(pEvt)->buffer = (pBuffer);                          \
    }

/* macro to get a pointer to the payload portion of an attached buffer (DIRECT buffer type only) */
#define WMI_EVT_GET_DIRECT_EVT_PTR(pEvt) (HTC_BUF_HDL((pEvt))->buffer)

/**** APIs ****/

#if 0
#define WMI_SERVICE_MODULE_INSTALL()
    /* no indirection table, call directly */
#define WMI_Init                      _WMI_Init
#define WMI_Ready                     _WMI_Ready
#define WMI_RegisterDispatchTable     _WMI_RegisterDispatchTable
#define WMI_AllocEvent(class, length) _WMI_AllocEvent(class, length)
#define WMI_SendEvent                 _WMI_SendEvent
#define WMI_GetPendingEventsCount     _WMI_GetPendingEventsCount
#define WMI_GetControlEp              _WMI_GetControlEp
#define WMI_Dispatch                  _WMI_Dispatch

void            _WMI_Init(WMI_SVC_CONFIG *pWmiConfig);
void            _WMI_Ready(void);
void            _WMI_RegisterDispatchTable(WMI_DISPATCH_TABLE *pDispatchTable);
WMI_EVT_PACKET *_WMI_AllocEvent(WMI_EVT_CLASS EventClass, int Length);
void            _WMI_SendEvent(WMI_EVT_PACKET *pEvt, uint16_t EventId, int Length);
int             _WMI_GetPendingEventsCount(void);
int             _WMI_GetControlEp(void);
void            _WMI_SendCompleteHandler(HTC_ENDPOINT_ID Endpt, HTC_BUFFER *pHTCBuf);
NT_BOOL          _WMI_RegisterEventFilter(WMI_EVENT_FILTER_CB cb, void* filterContext, NT_BOOL addFilter);
void            _WMI_Dispatch(uint16_t cmd, uint8_t* pCmdBuffer, int length, uint16_t info1);

#else

/* the API table */
typedef struct _wmi_svc_apis {
    void (*_WMI_Init)(WMI_SVC_CONFIG *pWmiConfig);
    void (*_WMI_RegisterDispatchTable)(WMI_DISPATCH_TABLE *pDispatchTable);
    WMI_EVT_PACKET *(*_WMI_AllocEvent)(WMI_EVT_CLASS EventClass, int Length);
    void (*_WMI_SendEvent)(WMI_EVT_PACKET *pEvt, uint16_t EventId, uint16_t info, int Length);
    int (*_WMI_GetPendingEventsCount)(void);
    void (*_WMI_SendCompleteHandler)(HTC_ENDPOINT_ID Endpt, HTC_BUFFER *pHTCBuf);
    int (*_WMI_GetControlEp)(void);
    NT_BOOL (*_WMI_RegisterEventFilter)(WMI_EVENT_FILTER_CB cb, void *filterContext, NT_BOOL addFilter);
    void (*_WMI_Dispatch)(uint16_t cmd, uint8_t *pCmdBuffer, int length, uint16_t info1);
    void *pReserved; /* for expansion if need be */
} WMI_SVC_APIS;

extern void WMI_service_module_install(WMI_SVC_APIS *pAPIs);

#endif
#endif  // ATH_KF

#endif /*WMI_SVC_API_H_*/
