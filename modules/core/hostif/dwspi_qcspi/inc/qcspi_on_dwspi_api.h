/*========================================================================
* @file qcspi_on_dwspi_api.h
* @brief APIs related to SPI slave SW
*========================================================================*/
#ifndef _QCSPI_ON_DWSPI_API_H_
#define _QCSPI_ON_DWSPI_API_H_
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define SR_DEFAULT 0x80200000
#define TXFTL 0x000000E0
#define RXFTL 0
#define IR_MAX_SIZE 1024

struct spi_shared_s
{
	uint8_t * p_IR;
	uint16_t read_offset;
	uint16_t write_offset;
	uint16_t IR_threshold;
	uint32_t data_read_address;
	uint16_t data_read_len;
	uint16_t data_read_rem_len;
	bool is_task_suspended;
};

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
void cmd_layer_task_create(void);
#ifdef SUPPORT_QCSPI_ON_DWSPI
void nt_spi_slv_interrupt(void);
#endif
#endif /* #ifndef _QCSPI_ON_DWSPI_API_H_ */
