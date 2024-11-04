/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef  NT_S_RECORD_API_H_
#define  NT_S_RECORD_API_H_


/*
 * @FUNCTION   :nt_bl_record_read_from_uart
 * @DESCRIPTION: download record data through uart.
 * @PARA   : NONE
 * @RETURN : integer
 *
 */
int nt_bl_record_read_from_uart(void);



/*
 * @FUNCTION   :nt_bl_data_conv_ascii_to_decimal
 * @DESCRIPTION: converting record data form ascii to decimal.
 * @PARA   : buffer
 * @RETURN : char
 *
 */

char nt_bl_data_conv_ascii_to_decimal(uint8_t buf[]);

/*
 * @FUNCTION   :nt_bl_verify_checksum
 * @DESCRIPTION: verify the checksum before writing and after reading the rram.
 * @PARA   : checksum,add_addr_data
 * @RETURN : uint8_t
 *
 */

char nt_bl_verify_checksum(uint8_t checksum,uint8_t add_addr_data);
/*
 * @FUNCTION   :nt_bl_image_write_and_read_rram
 * @DESCRIPTION: write and read the data from/to rram.
 * @PARA   : address,buffer,data_len,checksum;
 * @RETURN : char
 *
 */
char nt_bl_image_write_and_read_rram(uint32_t address,uint8_t buf[],uint16_t length,uint8_t checksum_before_read);
/*
 * @FUNCTION   :nt_bl_record_decrypted_frame_format
 * @DESCRIPTION: After verify the checksum write data and read rram.
 * @PARA   : buffer,length,type
 * @RETURN : char
 *
 */
char nt_bl_record_decrypted_frame_format(uint8_t s_rbuffer[],uint16_t length,uint8_t type);

void nt_bl_image_downloading_buffer(char uart_ch);

#if defined(SUPPORT_PBL_PATCH)
typedef int (*nt_bl_record_read_from_uart_t)(void);
typedef char (*nt_bl_data_conv_ascii_to_decimal_t)(uint8_t buf[]);
typedef char (*nt_bl_verify_checksum_t)(uint8_t checksum,uint8_t add_addr_data);
typedef char (*nt_bl_image_write_and_read_rram_t)(uint32_t address,uint8_t buf[],uint16_t length,uint8_t checksum_before_read);
typedef char (*nt_bl_record_decrypted_frame_format_t)(uint8_t s_rbuffer[],uint16_t length,uint8_t type);
typedef void (*nt_bl_image_downloading_buffer_t)(char uart_ch);

typedef struct nt_bl_srec_s{
    nt_bl_record_read_from_uart_t record_read_from_uart_pfn;
    nt_bl_data_conv_ascii_to_decimal_t data_conv_ascii_to_decimal_pfn;
    nt_bl_verify_checksum_t verify_checksum_pfn;
    nt_bl_image_write_and_read_rram_t image_write_and_read_rram_pfn;
    nt_bl_record_decrypted_frame_format_t record_decrypted_frame_format_pfn;
    nt_bl_image_downloading_buffer_t image_downloading_buffer_pfn;
}nt_bl_srec_api_t;
#endif /*SUPPORT_PBL_PATCH*/
#endif //NT_S_RECORD_API_H_
