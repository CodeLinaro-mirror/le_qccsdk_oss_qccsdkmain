/*========================================================================
*
* @file wifi_fw_sys_img_loader_api.h
* @brief Contains the structures and APIs which needs to be exposed.
*========================================================================*/
#ifndef _WIFI_FW_SYS_IMG_LOADER_API_
#define _WIFI_FW_SYS_IMG_LOADER_API_
typedef enum sbl_func_status
{
SBL_FUNC_FAILURE,
SBL_FUNC_SUCCESS,
}sbl_func_status_t;

sbl_func_status_t sys_loader(void);

#endif
