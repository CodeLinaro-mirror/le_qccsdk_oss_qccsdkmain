/*
 * For this file, which was received with alternative licensing options for
 * distribution, Qualcomm Technologies, Inc. has selected the BSD license.
 */

/*
 * This Synopsys software and associated documentation (hereinafter the
 * "Software") is an unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you. The
 * Software IS NOT an item of Licensed Software or a Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Products
 * with Synopsys or any supplement thereto. Synopsys is a registered trademark
 * of Synopsys, Inc. Other names included in the SOFTWARE may be the
 * trademarks of their respective owners.
 *
 * The contents of this file are dual-licensed; you may select either version
 * 2 of the GNU General Public License ("GPL") or the BSD-3-Clause license
 * ("BSD-3-Clause"). The GPL is included in the COPYING file accompanying the
 * SOFTWARE. The BSD License is copied below.
 *
 * BSD-3-Clause License:
 * Copyright (c) 2024 Synopsys, Inc. and/or its affiliates.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer, without
 *    modification.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The names of the above-listed copyright holders may not be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "elppka.h"
#include "elppka_hw.h"
#include "elppka_cmds.h"
#include "elppka_intr.h"
#include "elppdu_error.h"
#include "crypto_os_port.h"
#include <string.h>
#include "elppka_fw.h"

#if PKA_USE_INTERRUPT
#include "qurt_internal.h"
#include "qurt_signal.h"
#include "qurt_types.h"

#define ECC_QUART_SIGNAL_MASK 0x00000001
qurt_signal_t g_pka_operation_complete_signal;
#endif

#define MAX_PKA_OPERATION_TIME_IN_MS 6000

uint32 g_max_pka_opeartion_time_in_ms = MAX_PKA_OPERATION_TIME_IN_MS;

const uint32_t *gp_pka_fw_ram_data = &g_pka_fw_ram_data[0];

#if defined(__ICCARM__) || defined(__GNUC__)
static inline void DataMemoryBarrier(void)
{
    asm volatile("dmb");
}
#else
static inline void DataMemoryBarrier(void)
{
    __asm("dmb");
}
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define io_read32(addr)         (*(volatile uint32_t *)addr)
#define io_write32(addr, value) (*(volatile uint32_t *)addr = value)

/* write a 32-bit word to a given address */
static inline void pdu_io_write32(void *addr, uint32_t val)
{
    io_write32(addr, val);
}

/* read a 32-bit word from a given address */
static inline uint32_t pdu_io_read32(void *addr)
{
    return io_read32(addr);
}

/*
 * Determine the base radix for the given operand size,
 *   ceiling(lg(size/8))
 * where size > 16 bytes.
 * Returns 0 if the size is invalid.
 */
static unsigned elppka_base_radix(unsigned size)
{
    if (size <= 16)
        return 0; /* Error */
    if (size <= 32)
        return 2;
    if (size <= 64)
        return 3;
    if (size <= 128)
        return 4;
    if (size <= 256)
        return 5;
    if (size <= 512)
        return 6;

    return 0;
}

/*
 * Helper to compute the operand page size, which depends only on the base
 * radix.
 */
static unsigned elppka_page_size(unsigned size)
{
    unsigned ret;

    ret = elppka_base_radix(size);
    if (!ret)
        return ret;
    return 8 << ret;
}

/*
 * Check that the given PKA operand index is valid for a particular bank and
 * operand size.  The bank and size values themselves are not validated.
 */
static int index_is_valid(const struct pka_config *cfg, unsigned bank, unsigned index, unsigned size)
{
    unsigned ecc_max_bytes, rsa_max_bytes, abc_storage, d_storage;

    ecc_max_bytes = cfg->ecc_size >> 3;
    rsa_max_bytes = cfg->rsa_size >> 3;

    if (size > ecc_max_bytes && size > rsa_max_bytes)
        return 0;
    if (index > 7)
        return 0;

    abc_storage = MAX(ecc_max_bytes * 8, rsa_max_bytes * 2);
    d_storage = MAX(ecc_max_bytes * 8, rsa_max_bytes * 4);

    if (bank == PKA_OPERAND_D) {
        return index < d_storage / size;
    } else {
        return index < abc_storage / size;
    }
}

/*
 * Determine the offset (in 32-bit words) of a particular operand in the PKA
 * memory map.
 * Returns the (non-negative) offset on success, or -errno on failure.
 */
static int operand_base_offset(const struct pka_config *cfg, unsigned bank, unsigned index, unsigned size)
{
    unsigned pagesize;
    int ret;

    pagesize = elppka_page_size(size);
    if (!pagesize)
        return CRYPTO_INVALID_SIZE;

    if (!index_is_valid(cfg, bank, index, pagesize))
        return CRYPTO_NOT_FOUND;

    switch (bank) {
        case PKA_OPERAND_A:
            ret = PKA_OPERAND_A_BASE;
            break;
        case PKA_OPERAND_B:
            ret = PKA_OPERAND_B_BASE;
            break;
        case PKA_OPERAND_C:
            ret = PKA_OPERAND_C_BASE;
            break;
        case PKA_OPERAND_D:
            ret = PKA_OPERAND_D_BASE;
            break;
        default:
            return CRYPTO_INVALID_ARGUMENT;
    }

    return ret + index * (pagesize >> 2);
}

/* Parse out the fields from a type-0 BUILD_CONF register in bc. */
static int elppka_get_config_type0(uint32_t bc, struct pka_config *out)
{
    struct pka_config cfg = {0};

    if (bc & (1ul << PKA_BC_FW_HAS_RAM)) {
        cfg.fw_ram_size = 256u << ((bc >> PKA_BC_FW_RAM_SZ) & ((1ul << PKA_BC_FW_RAM_SZ_BITS) - 1));
    }
    if (bc & (1ul << PKA_BC_FW_HAS_ROM)) {
        cfg.fw_rom_size = 256u << ((bc >> PKA_BC_FW_ROM_SZ) & ((1ul << PKA_BC_FW_ROM_SZ_BITS) - 1));
    }

    cfg.alu_size = 32u << ((bc >> PKA_BC_ALU_SZ) & ((1ul << PKA_BC_ALU_SZ_BITS) - 1));
    cfg.rsa_size = 512u << ((bc >> PKA_BC_RSA_SZ) & ((1ul << PKA_BC_RSA_SZ_BITS) - 1));
    cfg.ecc_size = 256u << ((bc >> PKA_BC_ECC_SZ) & ((1ul << PKA_BC_ECC_SZ_BITS) - 1));

    *out = cfg;
    return 0;
}

/* Parse out the fields from a type-1 BUILD_CONF register in bc. */
static int elppka_get_config_type1(uint32_t bc, struct pka_config *out)
{
    struct pka_config cfg = {0};
    uint32_t tmp;

    tmp = (bc >> PKA_BC1_FW_RAM_SZ) & ((1ul << PKA_BC1_FW_RAM_SZ_BITS) - 1);
    if (tmp)
        cfg.fw_ram_size = 256u << (tmp - 1);

    tmp = (bc >> PKA_BC1_FW_ROM_SZ) & ((1ul << PKA_BC1_FW_ROM_SZ_BITS) - 1);
    if (tmp)
        cfg.fw_rom_size = 256u << (tmp - 1);

    tmp = (bc >> PKA_BC1_RSA_SZ) & ((1ul << PKA_BC1_RSA_SZ_BITS) - 1);
    if (tmp)
        cfg.rsa_size = 512u << (tmp - 1);

    tmp = (bc >> PKA_BC1_ECC_SZ) & ((1ul << PKA_BC1_ECC_SZ_BITS) - 1);
    if (tmp)
        cfg.ecc_size = 256u << (tmp - 1);

    tmp = (bc >> PKA_BC1_ALU_SZ) & ((1ul << PKA_BC1_ALU_SZ_BITS) - 1);
    cfg.alu_size = 32u << tmp;

    *out = cfg;
    return 0;
}

/* Read out PKA H/W configuration into config structure. */
static int elppka_get_config(uint32_t *regs, struct pka_config *out)
{
    uint32_t bc = pdu_io_read32(&regs[PKA_BUILD_CONF]);

    switch ((bc >> PKA_BC_FORMAT_TYPE) & ((1ul << PKA_BC_FORMAT_TYPE_BITS) - 1)) {
        case 0:
            return elppka_get_config_type0(bc, out);
        case 1:
            return elppka_get_config_type1(bc, out);
    }

    return CRYPTO_NOT_IMPLEMENTED;
}

int elppka_start(struct pka_state *pka, uint32_t pka_cmd, uint32_t flags, unsigned size)
{
    uint32_t ctrl, base;

    if (pka_cmd >= PKA_CMD_LAST) {
        return CRYPTO_NOT_IMPLEMENTED;
    }

    uint32_t entry = g_pka_cmd_addresses[pka_cmd];

    base = elppka_base_radix(size);
    if (!base)
        return CRYPTO_INVALID_SIZE;

    ctrl = base << PKA_CTRL_BASE_RADIX;

    /* Handle Curve 25519 as a special case. */
    if ((pka_cmd == PKA_CMD_C25519_PMULT) || (pka_cmd == PKA_CMD_ED25519_PADD) || (pka_cmd == PKA_CMD_ED25519_PDBL) ||
        (pka_cmd == PKA_CMD_ED25519_PMULT) || (pka_cmd == PKA_CMD_ED25519_PVER) ||
        (pka_cmd == PKA_CMD_ED25519_SHAMIR)) {
        ctrl |= ((size / 4) % 8) << PKA_CTRL_PARTIAL_RADIX;
    } else {
        ctrl |= (size / 4) << PKA_CTRL_PARTIAL_RADIX;
    }
    ctrl |= 1ul << PKA_CTRL_GO;

    pdu_io_write32(&pka->regbase[PKA_F_STACK], 0);
    pdu_io_write32(&pka->regbase[PKA_FLAGS], flags);
    pdu_io_write32(&pka->regbase[PKA_ENTRY], entry);
    pdu_io_write32(&pka->regbase[PKA_CTRL], ctrl);
    DataMemoryBarrier();

    return 0;
}

void elppka_abort(struct pka_state *pka)
{
    pdu_io_write32(&pka->regbase[PKA_CTRL], 1 << PKA_CTRL_STOP_RQST);
    DataMemoryBarrier();
}

#define GET_CCOUNT() (*((volatile uint32_t *)0xE0001004))

// VM TODO: change appropriately to reflect 1s ccount interval
#define CPU_TICKS_PER_SECOND (40 * 1000 * 1000)

#define IS_PKA_OPERATION_TIMEDOUT(start_ccount) (((GET_CCOUNT() - start_ccount) >= CPU_TICKS_PER_SECOND) ? (1) : (0))

int elppka_cancel_outstanding_operation(struct pka_state *pka)
{
    uint32_t rc_register_value = 0;
    do {
        // abort any pending operations
        elppka_abort(pka);
        rc_register_value = pdu_io_read32(&pka->regbase[PKA_RC]);
    } while ((uint32_t)(rc_register_value & (0x1U << PKA_RC_BUSY)) != 0);

    // acknowledge any pending completion interrupt
    elppka_ack_irq(pka);

    return 0;
}

int elppka_prepare_pka_before_starting_operation(struct pka_state *pka)
{
    int status;

    status = elppka_cancel_outstanding_operation(pka);
    if (0 != status) {
        return status;
    }

#if PKA_USE_INTERRUPT
    // enable the generation of interrupt complete interrupt
    elppka_enable_irq(pka);
#endif  // PKA_USE_INTERRUPT

    return 0;
}

int elppka_wait_for_operation_to_complete(struct pka_state *pka, uint32_t timeout_in_ms)
{
    crypto_time_t start_time_in_ticks = CRYPTO_GET_TIME();
    uint32_t elapsed_time_in_ms;

#if PKA_USE_INTERRUPT
    // wait till operation is complete
    qurt_time_t timeout_in_qurt_ticks = qurt_timer_convert_time_to_ticks(timeout_in_ms, QURT_TIME_MSEC);
    uint32_t signals_set = 0;
    qurt_signal_wait_timed(pka->operation_complete_signal_ptr, ECC_QUART_SIGNAL_MASK, QURT_SIGNAL_ATTR_CLEAR_MASK,
                           (uint32 *)&signals_set, timeout_in_qurt_ticks);
    elapsed_time_in_ms = CRYPTO_GET_ELAPSED_TIME_IN_MS(start_time_in_ticks, CRYPTO_GET_TIME());
    // disable the generation of operation complete interrupt
    elppka_disable_irq(pka);
#else
    uint32_t status_register_value = 0;
    do {
        status_register_value = pdu_io_read32(&pka->regbase[PKA_STATUS]);
        elapsed_time_in_ms = CRYPTO_GET_ELAPSED_TIME_IN_MS(start_time_in_ticks, CRYPTO_GET_TIME());
    } while (((status_register_value & (1 << PKA_STAT_IRQ)) == 0) && (elapsed_time_in_ms < timeout_in_ms));
#endif

    // debug code (to estimate good value for watchdog)
    pka->time_in_ms_since_go = elapsed_time_in_ms;
    pka->instructions_since_go = pdu_io_read32(&pka->regbase[PKA_INST_SINCE_GO]);

    // check if the operation timed out
    if (pka->time_in_ms_since_go >= g_max_pka_opeartion_time_in_ms) {
        elppka_cancel_outstanding_operation(pka);
        return -1;
    }

    // acknowledge the STAT_IRQ to clear it
    elppka_ack_irq(pka);

    return 0;
}

int elppka_perform_operation(struct pka_state *pka, uint32_t pka_cmd, uint32_t flags, unsigned size)
{
    int status;

    status = elppka_prepare_pka_before_starting_operation(pka);
    if (0 != status) {
        return status;
    }

    status = elppka_start(pka, pka_cmd, flags, size);
    if (status != 0) {
        return status;
    }

    status = elppka_wait_for_operation_to_complete(pka, g_max_pka_opeartion_time_in_ms);
    if (0 != status) {
        return status;
    }

    // verify that the RC_STOP_REASON field is 0
    pka->last_return_code_register_value = pdu_io_read32(&pka->regbase[PKA_RC]);
    if ((pka->last_return_code_register_value & PKA_RC_REASON_MASK) != 0) {
        return -2;
    }

    return 0;
}

int elppka_load_operand(struct pka_state *pka, unsigned bank, unsigned index, unsigned size, const uint8_t *data)
{
    uint32_t *opbase, tmp;
    unsigned i, n;
    int rc;

    rc = operand_base_offset(&pka->cfg, bank, index, size);
    if (rc < 0)
        return rc;

    opbase = pka->regbase + rc;
    n = size >> 2;

    for (i = 0; i < n; i++) {
        /*
         * For lengths that are not a multiple of 4, the incomplete word is
         * at the _start_ of the data buffer, so we must add the remainder.
         */
        memcpy(&tmp, data + i * 4, 4);
        pdu_io_write32(&opbase[i], tmp);
    }

    /* Write the incomplete word, if any. */
    if (size & 3) {
        tmp = 0;
        char *c_tmp = (char *)&tmp;
        memcpy(c_tmp + sizeof tmp - (size & 3), data + i * 4, size & 3);
        pdu_io_write32(&opbase[i++], tmp);
    }

    /* Zero the remainder of the operand. */
    for (n = elppka_page_size(size) >> 2; i < n; i++) {
        pdu_io_write32(&opbase[i], 0);
    }

    return 0;
}

int elppka_unload_operand(struct pka_state *pka, unsigned bank, unsigned index, unsigned size, uint8_t *data)
{
    uint32_t *opbase, tmp;
    unsigned i, n;
    int rc;

    rc = operand_base_offset(&pka->cfg, bank, index, size);
    if (rc < 0)
        return rc;

    opbase = pka->regbase + rc;
    n = size >> 2;

    for (i = 0; i < n; i++) {
        tmp = pdu_io_read32(&opbase[i]);
        memcpy(data + 4 * i, &tmp, 4);
    }

    if (size & 3) {
        tmp = pdu_io_read32(&opbase[i]);
        char *c_tmp = (char *)&tmp;
        memcpy(data + 4 * i, c_tmp + sizeof tmp - (size & 3), size & 3);
    }

    return 0;
}

void elppka_set_byteswap(struct pka_state *pka, int swap)
{
    uint32_t val = pdu_io_read32(&pka->regbase[PKA_CONF]);

    if (swap) {
        val |= 1 << PKA_CONF_BYTESWAP;
    } else {
        val &= ~(1 << PKA_CONF_BYTESWAP);
    }

    pdu_io_write32(&pka->regbase[PKA_CONF], val);
}

int elppka_init(struct pka_state *pka, uint32_t *regbase)
{
    pka->regbase = regbase;

    int rc = elppka_get_config(regbase, &pka->cfg);
    if (rc < 0) {
        return rc;
    }
    return 0;
}

#if PKA_USE_INTERRUPT
extern struct pka_state *g_elppka_ctxt;

void __attribute__((section(".after_ram_vectors"))) ecc_core_m4f_intr()
{
    struct pka_state *pka = g_elppka_ctxt;

    // acknowledge the operation complete interrupt
    elppka_ack_irq(pka);
    DataMemoryBarrier();

    if (!pka->operation_complete_signal_ptr) {
        return;
    }

    // wake up the operation complete signal
    qurt_signal_set(pka->operation_complete_signal_ptr, ECC_QUART_SIGNAL_MASK);
}
#endif  // PKA_USE_INTERRUPT

void elppka_setup(struct pka_state *pka, int pka_operands_are_in_big_endian_format)
{
    // Based on the PKA spec (EDN-0574):
    // The wide data operand memories A, B, c, and D, are little endian by default, but can be programmed
    // for automatic byte swapping within each 32-bit word by setting the PKA_ENDIAN_BYTE_SWAP bit in the
    // CONFIG register
    pka->operands_are_in_big_endian_format = pka_operands_are_in_big_endian_format;
    if (pka->operands_are_in_big_endian_format) {
        elppka_set_byteswap(pka, 1);
    } else {
        elppka_set_byteswap(pka, 0);
    }

#if PKA_USE_INTERRUPT
    if (!pka->operation_complete_signal_ptr) {
        // initialize the operation complete signal
        pka->operation_complete_signal_ptr = &g_pka_operation_complete_signal;
        qurt_signal_create(pka->operation_complete_signal_ptr);
        qurt_signal_clear(pka->operation_complete_signal_ptr, ECC_QUART_SIGNAL_MASK);

        // enable PKA interrupt
        io_write32(NT_NVIC_ISER1, NVIC_PKA_ENABLE);
    }
#endif  // PKA_USE_INTERRUPT
}

void elppka_ack_irq(struct pka_state *pka)
{
    pdu_io_write32(&pka->regbase[PKA_STATUS], 1 << PKA_STAT_IRQ);
}

void elppka_enable_irq(struct pka_state *pka)
{
    pdu_io_write32(&pka->regbase[PKA_IRQ_EN], (1 << PKA_IRQ_EN_STAT));
}

void elppka_disable_irq(struct pka_state *pka)
{
    pdu_io_write32(&pka->regbase[PKA_IRQ_EN], 0);
}

int elppka_fw_load(struct pka_state *pka)
{
    uint32_t *rambase;
    unsigned long i;

    rambase = &pka->regbase[PKA_FIRMWARE_BASE];

    if (g_pka_fw_rom_data_size) {
        if (g_pka_fw_rom_data_size > pka->cfg.fw_rom_size) {
            return CRYPTO_INVALID_FIRMWARE;
        }

#if 0
      uint32_t tmp, rombase;
      rombase = &pka->regbase[PKA_FIRMWARE_BASE + pka->cfg.fw_ram_size];
      
      /* Check hash in the device against the hash in the image. */
      for (i = 2; i <= 7; i++) {
         tmp = pdu_io_read32(&rombase[i]);

         if (fw->priv->rom_base[i] != tmp) {
            fw->errmsg = "ROM image does not match device";
            return CRYPTO_INVALID_FIRMWARE;
         }
      }
#endif
    }

    if (g_pka_fw_ram_data_size) {
        if (g_pka_fw_ram_data_size > pka->cfg.fw_ram_size) {
            return CRYPTO_INVALID_FIRMWARE;
        }

        for (i = 0; i < g_pka_fw_ram_data_size; i++) {
            pdu_io_write32(&rambase[i], gp_pka_fw_ram_data[i]);
        }
    }

    return 0;
}

static inline uint32_t elppka_read_flags(struct pka_state *pka)
{
    return pdu_io_read32(&pka->regbase[PKA_FLAGS]);
}

uint32_t elppka_is_zero_flag_set(struct pka_state *pka)
{
    return ((elppka_read_flags(pka) & (1 << PKA_FLAG_ZERO)) != 0);
}

uint32_t elppka_set_f0_flag(uint32_t flags)
{
    flags |= (1 << PKA_FLAG_F0);
    return flags;
}

uint32_t elppka_is_f3_flag_set(struct pka_state *pka)
{
    return ((elppka_read_flags(pka) & (1 << PKA_FLAG_F3)) != 0);
}

uint32_t elppka_set_f3_flag(uint32_t flags)
{
    flags |= (1 << PKA_FLAG_F3);
    return flags;
}

void elppka_set_index_l_register(struct pka_state *pka, uint32_t value)
{
    pdu_io_write32(&pka->regbase[PKA_INDEX_L], value);
}

void elppka_set_jump_probability_register(struct pka_state *pka, uint32_t value)
{
    pdu_io_write32(&pka->regbase[PKA_DTA_JUMP], value);
}
