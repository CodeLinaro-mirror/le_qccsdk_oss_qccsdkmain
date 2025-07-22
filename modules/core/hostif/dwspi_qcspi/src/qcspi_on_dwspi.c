/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file qcspi_on_dwspi.c
 * @brief Command layer task and ISR for SPI slave SW
 *========================================================================*/

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <assert.h>
#include "nt_hw.h"
#include "wlan_dev.h"
#include "nt_qspi_api.h"
#include "ring_svc_api.h"
#include "qcspi_on_dwspi_internal.h"
#include "qcspi_on_dwspi_api.h"
#include "task.h"
#include "ferm_prof.h"

/*-------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
TaskHandle_t g_cmd_layer_task_handle;
uint32_t g_status_register;
struct spi_shared_s g_spi_shared;

struct command_s {
    uint8_t command;
    uint32_t address;
    uint16_t rw_data_len;
    uint16_t rem_rw_data_len;
} g_command_isr;

/*-------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/

/**
  @brief This function pushes the value of the SW maintained
         status register variable to the Tx FIFO.
  @param None
  @return None
*/
static void send_status_register()
{
    write_header_to_tx_fifo();
    write_to_tx_fifo((uint8_t)(g_status_register >> 24));
    write_to_tx_fifo((uint8_t)(g_status_register >> 16));
    write_to_tx_fifo((uint8_t)(g_status_register >> 8));
    write_to_tx_fifo((uint8_t)g_status_register);
}

/**
  @brief This function is a task, that receives the data stream from the driver
         (ISR), parses and handles the SPI command.
  @param params: NULL (task parameter; not used)
  @return None
*/

static void command_layer_task(void *params)
{
    uint8_t value;
    uint8_t state = SM_INIT;
    struct command_s command = (struct command_s){0, 0, 0, 0};
    bool is_task_busy = false;

    for (;;) {
        if (state != C_FREAD && state != C_MIR && state != C_RDSR && state != SM_INIT && state != SM_CHECK_NEXT_CMD &&
            RING_EMPTY()) {
            IS_TASK_SUSPENDED(true);
            vTaskSuspend(NULL);
            IS_TASK_SUSPENDED(false);
        }

        switch (state) {
            /* INIT state - Only for the first time when task is created */
            case SM_INIT:
                state = SM_CMD;

                IS_TASK_SUSPENDED(true);
                vTaskSuspend(NULL);
                IS_TASK_SUSPENDED(false);
                break;

            /* CMD state - Receive the Command Byte */
            case SM_CMD:
                command.command = *(g_spi_shared.p_IR + g_spi_shared.read_offset++);
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;

                if (command.command == C_NOP) {
                    state = SM_CHECK_NEXT_CMD;
                } else if (command.command == C_RDSR) {
                    state = C_RDSR;
                } else if (command.command == C_FREAD || command.command == C_MIR || command.command == C_WRITE ||
                           command.command == C_IRW) {
                    state = SM_LEN1;
                } else {
                    /* Master is expected to send correct commands.
                       The code branches here only when master sends
                       invalid command byte.
                       The command byte is ignored and the next byte
                       is checked for command to ensure that the
                       state machine is not branched to an invalid state.
                    */
                    WLAN_DBG1_PRINT("SPI_ERROR: Invalid Command from Master -", command.command);
                    WLAN_DBG0_PRINT("Calling assert()");
                    assert(0);
                    state = SM_CMD;
                }
                break;

            /* LEN1 state - Receive the first byte of 2-byte length */
            case SM_LEN1:
                command.rw_data_len = *(g_spi_shared.p_IR + g_spi_shared.read_offset++) << 8;
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;
                state = SM_LEN2;
                break;

            /* LEN2 state - Receive the second byte of 2-byte length */
            case SM_LEN2:
                command.rw_data_len |= *(g_spi_shared.p_IR + g_spi_shared.read_offset++);
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;
                command.rem_rw_data_len = command.rw_data_len;
                state = SM_ADDR1;
                break;

            /* ADDR1 state - Receive the first byte of 4-byte address */
            case SM_ADDR1:
                command.address = *(g_spi_shared.p_IR + g_spi_shared.read_offset++) << 24;
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;
                state = SM_ADDR2;
                break;

            /* ADDR2 state - Receive the second byte of 4-byte address */
            case SM_ADDR2:
                command.address |= *(g_spi_shared.p_IR + g_spi_shared.read_offset++) << 16;
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;
                state = SM_ADDR3;
                break;

            /* ADDR3 state - Receive the third byte of 4-byte address */
            case SM_ADDR3:
                command.address |= *(g_spi_shared.p_IR + g_spi_shared.read_offset++) << 8;
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;
                state = SM_ADDR4;
                break;

            /* ADDR4 state - Receive the fourth byte of 4-byte address */
            case SM_ADDR4:
                command.address |= *(g_spi_shared.p_IR + g_spi_shared.read_offset++);
                g_spi_shared.read_offset = g_spi_shared.read_offset % IR_MAX_SIZE;

                /* Changing the state to the corresponding command
                   Command is validated in the SM_CMD state
                */
                state = command.command;
                break;

            /* FREAD/MIR command - Data Read */
            case C_FREAD:
            case C_MIR:
                is_task_busy = true;
                uint8_t block, available_fifo_lvl, i;
                available_fifo_lvl = FIFO_DEPTH - ((uint8_t)TX_FIFO_LEVEL);

                /* It is made sure that during the initial Tx FIFO
                   load, there are 6 empty entries available,
                   so that incase of any FIFO errors, the status
                   register contents can be sent, without Tx Overflow */

                if (available_fifo_lvl <= (READ_START_FRAME_SIZE + SR_SEND_SIZE)) {
                    continue;
                } else {
                    available_fifo_lvl -= (READ_START_FRAME_SIZE + SR_SEND_SIZE);
                }

                block = command.rw_data_len < available_fifo_lvl ? command.rw_data_len : available_fifo_lvl;

                write_header_to_tx_fifo();
                /* Fill up the Tx FIFO */
                i = 0;
                for (; i < block; i++) {
                    write_to_tx_fifo(*((uint8_t *)command.address++));
                    command.rem_rw_data_len--;
                }

                /* Offload Data read to the Driver layer */
                if (command.rem_rw_data_len != 0) {
                    g_spi_shared.data_read_address = command.address;
                    g_spi_shared.data_read_len = command.rw_data_len;
                    g_spi_shared.data_read_rem_len = command.rem_rw_data_len;

                    /* Enabling ssi_txe_intr. To be disabled in ISR after Data Read is done */
                    value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                    value |= (1 << QWLAN_SPI_IMR_TXEIM_OFFSET);
                    NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                    state = SM_CMD;
                } else {
                    g_spi_shared.data_read_address = 0;
                    g_spi_shared.data_read_len = 0;
                    g_spi_shared.data_read_rem_len = 0;

                    state = SM_CHECK_NEXT_CMD;
                    is_task_busy = false;
                }

                if (state == SM_CMD) {
                    IS_TASK_SUSPENDED(true);
                    vTaskSuspend(NULL);
                    IS_TASK_SUSPENDED(false);
                }
                break;

            /* WRITE command - Data Write */
            case C_WRITE:
                is_task_busy = true;

                /* Disabling ssi_rxf_intr */
                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value &= ~(1 << QWLAN_SPI_IMR_RXFIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                /* If all remaining data are already available in the IR,
                    read the IR for all the data and check for next command,
                    else set the threshold, move to data write state and suspend */
                if (RING_LEVEL() >= command.rem_rw_data_len) {
                    while (command.rem_rw_data_len != 0) {
                        *((uint8_t *)command.address++) = *(g_spi_shared.p_IR + g_spi_shared.read_offset);
                        g_spi_shared.read_offset = (g_spi_shared.read_offset + 1) % IR_MAX_SIZE;
                        command.rem_rw_data_len--;
                    }
                    state = SM_CHECK_NEXT_CMD;
                    is_task_busy = false;
                } else {
                    g_spi_shared.IR_threshold = command.rem_rw_data_len < IR_THRESH_DIFF
                                                    ? (g_spi_shared.read_offset + command.rem_rw_data_len) % IR_MAX_SIZE
                                                    : (g_spi_shared.read_offset + IR_THRESH_DIFF) % IR_MAX_SIZE;
                    state = SM_DATA_WRITE;
                }

                /* Enabling ssi_rxf_intr */
                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value |= (1 << QWLAN_SPI_IMR_RXFIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                /* If data write is not complete, suspend the task
                    (to be resumed after reaching threshold) */
                if (state == SM_DATA_WRITE) {
                    IS_TASK_SUSPENDED(true);
                    vTaskSuspend(NULL);
                    IS_TASK_SUSPENDED(false);
                }
                break;

            /* DATA_WRITE state - State to handle ongoing data write */
            case SM_DATA_WRITE:

                /* Data write until threshold is reached */
                while (g_spi_shared.read_offset != g_spi_shared.IR_threshold) {
                    *((uint8_t *)command.address++) = *(g_spi_shared.p_IR + g_spi_shared.read_offset);
                    g_spi_shared.read_offset = (g_spi_shared.read_offset + 1) % IR_MAX_SIZE;
                    command.rem_rw_data_len--;
                }

                /* Disabling ssi_rxf_intr */
                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value &= ~(1 << QWLAN_SPI_IMR_RXFIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                /* If data write is done, check for next command.
                    If all remaining data are already available in the IR,
                    read the IR for all the data and check for next command,
                    else set the threshold and suspend (resumed to same state)*/

                if (command.rem_rw_data_len == 0) {
                    state = SM_CHECK_NEXT_CMD;
                    is_task_busy = false;
                } else if (RING_LEVEL() >= command.rem_rw_data_len) {
                    while (command.rem_rw_data_len != 0) {
                        *((uint8_t *)command.address++) = *(g_spi_shared.p_IR + g_spi_shared.read_offset);
                        g_spi_shared.read_offset = (g_spi_shared.read_offset + 1) % IR_MAX_SIZE;
                        command.rem_rw_data_len--;
                    }
                    state = SM_CHECK_NEXT_CMD;
                    is_task_busy = false;
                } else {
                    g_spi_shared.IR_threshold = command.rem_rw_data_len < IR_THRESH_DIFF
                                                    ? (g_spi_shared.read_offset + command.rem_rw_data_len) % IR_MAX_SIZE
                                                    : (g_spi_shared.read_offset + IR_THRESH_DIFF) % IR_MAX_SIZE;
                }

                /* Enabling ssi_rxf_intr */
                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value |= (1 << QWLAN_SPI_IMR_RXFIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                /* If data write is not complete, suspend the task
                    (to be resumed after reaching threshold) */
                if (state == SM_DATA_WRITE) {
                    IS_TASK_SUSPENDED(true);
                    vTaskSuspend(NULL);
                    IS_TASK_SUSPENDED(false);
                }
                break;

            /* IRW command - For M2N SPI Intr; Kick off the handler, and check for next cmd */
            case C_IRW:
                state = SM_IRW;
                break;

            case SM_IRW:
                command.rem_rw_data_len--;
                if (command.rem_rw_data_len == 0) {
                    g_spi_shared.read_offset += command.rw_data_len;
                    g_spi_shared.read_offset %= IR_MAX_SIZE;
                    state = SM_CHECK_NEXT_CMD;

                    ringif_tickle_all_rings(false);
                }
                break;

            /* RDSR command - Read Status Register */
            case C_RDSR:
                if ((FIFO_DEPTH - TX_FIFO_LEVEL + 1) >= (READ_START_FRAME_SIZE + SR_SEND_SIZE)) {
                    send_status_register();
                    state = SM_CHECK_NEXT_CMD;
                }
                break;

            /* CHECK_NEXT_CMD state - State to check whether a cmd is already available in IR */
            case SM_CHECK_NEXT_CMD:;
                uint8_t next_command_available = 0;

                /* Disable ssi_rxf_intr */
                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value &= ~(1 << QWLAN_SPI_IMR_RXFIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                /* If RDSR cmd is available in IR, handle it and stay in the same state
                    If a complete command frame is available, move to SM_CMD to handle the command frame
                    If full cmd frame is not available, suspend the task */
                if (RING_LEVEL() > 0) {
                    if (*(g_spi_shared.p_IR + g_spi_shared.read_offset) == C_RDSR) {
                        send_status_register();
                        g_spi_shared.read_offset = (g_spi_shared.read_offset + 1) % IR_MAX_SIZE;
                        next_command_available = 1;
                    } else if (RING_LEVEL() >= CMD_FRAME_SIZE) {
                        next_command_available = 1;
                        state = SM_CMD;
                    }
                }

                /* Enable ssi_rxf_intr */
                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value |= (1 << QWLAN_SPI_IMR_RXFIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);

                if (next_command_available == 0) {
                    IS_TASK_SUSPENDED(true);
                    vTaskSuspend(NULL);
                    IS_TASK_SUSPENDED(false);
                }
                break;
            default:
                WLAN_DBG1_PRINT("SPI_ERROR: Invalid State in Task -", state);
                WLAN_DBG0_PRINT("Calling assert()");
                assert(0);
        }
        /* NULL parameter is passed;
           The below 'if' clause is used just to avoid the
           compilation error which says that the variable
           'params' is defined but not used */
        if (params == NULL) {
        }
    }
}

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/

/**
 * @brief This function creates the command layer task
 * @param None
 * @see command_layer_task
 * @return None
 */
void cmd_layer_task_create(void)
{
    xTaskCreate(command_layer_task, "CMD Task\0", AP_TASK_STACK_SIZE, NULL, CMD_TASK_PRIORITY,
                &g_cmd_layer_task_handle);
}

#ifdef SUPPORT_QCSPI_ON_DWSPI
/**
  @brief This is the ISR for the QCSPI slave SW for Fermion
  @param None
  @return None
*/
void __attribute__((section(".after_ram_vectors"))) nt_spi_slv_interrupt(void)
{
    PROF_IRQ_ENTER();

    uint8_t value;
    static uint8_t state_isr = SM_CMD;
    static bool b_read_in_isr = false, b_write_cmd = false;
    bool b_status_send = false;

    /* ssi_rxf_intr - receiving data from Rx FIFO*/
    if ((NT_REG_RD(QWLAN_SPI_ISR_REG) & QWLAN_SPI_ISR_RXFIS_MASK) == 0x10) {
        uint8_t iter = 0;
        uint8_t rx_fifo_lvl = RX_FIFO_LEVEL;
        for (; iter < rx_fifo_lvl; iter++) {
            value = read_from_rx_fifo();

            if (true == b_write_cmd) {
                ;
            } else if (false == b_read_in_isr) {
                if (((g_spi_shared.write_offset + 1) % IR_MAX_SIZE) == (g_spi_shared.read_offset)) {
                    WLAN_DBG0_PRINT("SPI_ERROR: IR Full ! Calling assert()");
                    assert(0); /* IR full */
                }
                *(g_spi_shared.p_IR + g_spi_shared.write_offset++) = value;
                g_spi_shared.write_offset %= IR_MAX_SIZE;
            }

            switch (state_isr) {
                /* CMD state - If RDSR is received, resume the task; else goto next state*/
                case SM_CMD:
                    g_command_isr.command = value;

                    if (g_command_isr.command == C_RDSR) {
                        portYIELD_FROM_ISR(xTaskResumeFromISR(g_cmd_layer_task_handle));
                    } else if (g_command_isr.command == C_NOP) {
                        g_spi_shared.write_offset -= 1;
                        g_spi_shared.write_offset %= IR_MAX_SIZE;
                    } else if (g_command_isr.command == C_FREAD || g_command_isr.command == C_MIR ||
                               //							 g_command_isr.command == C_WRITE ||
                               g_command_isr.command == C_IRW) {
                        state_isr = SM_LEN1;
                    } else if (g_command_isr.command == C_WRITE) {
                        state_isr = SM_LEN1;
                        b_write_cmd = true;
                        g_spi_shared.write_offset -= 1;
                        g_spi_shared.write_offset %= IR_MAX_SIZE;
                    } else {
                        /* Master is expected to send correct commands.
                           The code branches here only when master sends
                           invalid command byte.
                           The command byte is ignored and the next byte
                           is checked for command to ensure that the
                           state machine is not branched to an invalid state.
                        */
                        state_isr = SM_CMD;
                    }
                    break;

                /* LEN1 - Receive first byte of 2-byte length */
                case SM_LEN1:
                    g_command_isr.rw_data_len = value << 8;
                    state_isr = SM_LEN2;
                    break;

                /* LEN2 - Receive second byte of 2-byte length */
                case SM_LEN2:
                    g_command_isr.rw_data_len |= value;
                    g_command_isr.rem_rw_data_len = g_command_isr.rw_data_len;

                    if ((g_command_isr.command == C_FREAD || g_command_isr.command == C_MIR) &&
                        g_command_isr.rw_data_len <= MAX_READ_ISR) {
                        b_read_in_isr = true;
                        g_spi_shared.write_offset -= CMD_LEN_BYTES;
                        g_spi_shared.write_offset %= IR_MAX_SIZE;
                    }

                    state_isr = SM_ADDR1;
                    break;

                /* ADDR - skip the address states; to reach end of cmd frame */
                case SM_ADDR1:
                    //					if (true == b_read_in_isr)
                    //					{
                    g_command_isr.address = value << 24;
                    //					}
                    state_isr = SM_ADDR2;
                    break;
                case SM_ADDR2:
                    //					if (true == b_read_in_isr)
                    //					{
                    g_command_isr.address |= value << 16;
                    //					}
                    state_isr = SM_ADDR3;
                    break;
                case SM_ADDR3:
                    //					if (true == b_read_in_isr)
                    //					{
                    g_command_isr.address |= value << 8;
                    //					}
                    state_isr = SM_ADDR4;
                    break;

                /* ADDR4 - end of cmd frame; resume the task */
                case SM_ADDR4:
                    /* If cmd is IRW (M2N SPI intr), handle if task is suspended, else leave it to the task to handle */
                    g_command_isr.address |= value;

                    if (g_command_isr.command == C_IRW) {
                        /* We do not resume task in case of IRW.
                           We wait intill complete IRW frame is
                           received, and then check the task
                           state and take action.
                        */
                        state_isr = SM_IRW_TRACK_ISR;
                        break;
                    }

                    /* If cmd is WRITE, go to WRITE_TRACK_ISR state to track the length of data to be written
                        and to resume the task based on IR_threshold
                       If cmd is any other (FREAD/MIR), move to CMD state and wait for next cmd */
                    else if (g_command_isr.command == C_WRITE) {
                        state_isr = SM_WRITE_TRACK_ISR;
                        set_SR_field(SR_NRDY_MASK);
                    }
                    /* If cmd is READ and data to be read is less than 16,
                        the data read is done in the interrupt context */
                    else if ((g_command_isr.command == C_FREAD || g_command_isr.command == C_MIR) &&
                             true == b_read_in_isr) {
                        state_isr = SM_READ_ISR;
                        b_read_in_isr = false;
                        //						g_command_isr.address |= value;
                        break;
                    } else {
                        set_SR_field(SR_NRDY_MASK);
                        state_isr = SM_CMD;
                    }

                    /* Resume the task to handle the cmd */

                    portYIELD_FROM_ISR(xTaskResumeFromISR(g_cmd_layer_task_handle));
                    break;

                /* WRITE_TRACK_ISR - track the length of data to be written
                    and to resume the task based on IR_threshold */
                case SM_WRITE_TRACK_ISR:
                    *((uint8_t *)(g_command_isr.address++)) = value;
                    g_command_isr.rem_rw_data_len--;
#if 0
					if (g_spi_shared.write_offset == g_spi_shared.IR_threshold)
					{
						portYIELD_FROM_ISR(xTaskResumeFromISR(g_cmd_layer_task_handle));
					}
#endif
                    if (g_command_isr.rem_rw_data_len == 0) {
                        state_isr = SM_CMD;
                        b_write_cmd = false;
                    }
                    break;

                case SM_IRW_TRACK_ISR:
                    /* Once all the bytes are received */
                    g_command_isr.rem_rw_data_len--;
                    if (g_command_isr.rem_rw_data_len == 0) {
                        state_isr = SM_CMD;

                        if (0 == (g_status_register & SR_NRDY_MASK)) {
                            g_spi_shared.write_offset =
                                (g_spi_shared.write_offset - (CMD_FRAME_SIZE + IRW_DATA_SIZE)) % IR_MAX_SIZE;
                            ringif_tickle_all_rings(true);
                        }
                    }
                    break;
                case SM_READ_ISR:
                    write_header_to_tx_fifo();
                    uint8_t i = 0;
                    for (; i < g_command_isr.rw_data_len; i++) {
                        write_to_tx_fifo(*((uint8_t *)g_command_isr.address++));
                    }
                    state_isr = SM_CMD;
                    break;
                default:
                    WLAN_DBG1_PRINT("SPI_ERROR: Invalid state in ISR -", state_isr);
                    WLAN_DBG0_PRINT("Calling assert()");
                    assert(0);
            }
        }
    }

    /* ssi_txe_intr - to write data to Tx FIFO on a data read operation */
    if ((NT_REG_RD(QWLAN_SPI_ISR_REG) & QWLAN_SPI_ISR_TXEIS_MASK) == 0x1) {
        uint8_t count;
        if (g_spi_shared.data_read_address == 0) {
            ;
        } else {
            uint32_t available_fifo_lvl = FIFO_DEPTH - ((uint8_t)TX_FIFO_LEVEL);
            uint16_t block = ((g_spi_shared.data_read_rem_len < available_fifo_lvl) ? g_spi_shared.data_read_rem_len
                                                                                    : available_fifo_lvl);

            if (block > MAX_TXF_REFILL_IN_ISR) {
                block = MAX_TXF_REFILL_IN_ISR;
            }

            for (count = 0; count < block; count++) {
                write_to_tx_fifo(*((uint8_t *)g_spi_shared.data_read_address++));
                g_spi_shared.data_read_rem_len--;
            }

            /* Disable the ssi_txe_intr once all the data are written to Tx FIFO */
            if (g_spi_shared.data_read_rem_len == 0) {
                g_spi_shared.data_read_address = 0;
                clear_SR_field(SR_NRDY_MASK);

                value = NT_REG_RD(QWLAN_SPI_IMR_REG);
                value &= ~(1 << QWLAN_SPI_IMR_TXEIM_OFFSET);
                NT_REG_WR(QWLAN_SPI_IMR_REG, value);
            }
        }
    }

    /* ssi_rxo_intr */
    if ((NT_REG_RD(QWLAN_SPI_ISR_REG) & QWLAN_SPI_ISR_RXOIS_MASK) == 0x8) {
        WLAN_DBG0_PRINT("SPI FIFO ERROR: ssi_rxo_intr");
        b_status_send = true;
        set_SR_field(SR_RXOERR_MASK);
        NT_REG_RD(QWLAN_SPI_RXOICR_REG); /** clear receive FIFO over flow interrupt bit */
    }

    if ((NT_REG_RD(QWLAN_SPI_ISR_REG) & QWLAN_SPI_ISR_RXUIS_MASK) == 0x4) {
        WLAN_DBG0_PRINT("SPI FIFO ERROR: ssi_rxu_intr");
        b_status_send = true;
        set_SR_field(SR_NRDY_MASK);
        NT_REG_RD(QWLAN_SPI_RXUICR_REG);  // clear receive FIFO under flow interrupt bit
    }

    if ((NT_REG_RD(QWLAN_SPI_ISR_REG) & QWLAN_SPI_ISR_TXOIS_MASK) == 0x2) {
        WLAN_DBG0_PRINT("SPI_FIFO_ERROR: ssi_txo_intr");
        NT_REG_RD(QWLAN_SPI_TXOICR_REG); /** clear transmit FIFO over flow interrupt bit */
    }
    if (b_status_send) {
        send_status_register();

        clear_SR_field(SR_RXOERR_MASK);
        clear_SR_field(SR_NRDY_MASK);
    }

    PROF_IRQ_EXIT();
}
#endif
