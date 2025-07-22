/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ecc_lock.h"
#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "qurt_signal.h"
#include "qurt_types.h"
#include "qccx.h"
#include "wifi_fw_pwr_cb_infra.h"

#include "pka.h"
#include "pka_internal.h"

#if CONFIG_SOC_QCC730V1
#define ELP_PKA_AHB      QCC730V1_ELP_PKA_AHB_BASE
#define ECC_LOCK_WRAPPER QCC730V1_LOCK_WRAPPER_L2_BASE
#elif CONFIG_SOC_QCC730V2
#define ELP_PKA_AHB      QCC730V2_ELP_PKA_AHB_BASE
#define ECC_LOCK_WRAPPER QCC730V2_LOCK_WRAPPER_L2_BASE
#endif

#define TAKE_LOCK(__lock__) ((qurt_mutex_lock_timed(&(__lock__), QURT_TIME_WAIT_FOREVER)) == QURT_EOK)
#define RELEASE_LOCK(__lock__)          \
    do {                                \
        qurt_mutex_unlock(&(__lock__)); \
    } while (0)

#define io_read32(addr)         (*(volatile uint32_t *)addr)
#define io_write32(addr, value) (*(volatile uint32_t *)addr = value)

struct pka_state *g_elppka_ctxt;
static uint8_t pka_init_done = 0;

void pka_enable_clock(pka_state_t *ctxt)
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_CCU_BASE_Type *ccu = QCC730V1_CCU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
#endif
    (void)ctxt;
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.ECC_ENABLE_CLK = 1;
}

void pka_disable_clock(pka_state_t *ctxt)
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_CCU_BASE_Type *ccu = QCC730V1_CCU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
#endif
    (void)ctxt;
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.ECC_ENABLE_CLK = 0;
}

void pka_power_switch_to_config()
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif
    pmu->pmu.PMU_SECIP_GDSCR.bit.COLLAPSE_EN_SW = 0;
    pmu->pmu.PMU_SECIP_GDSCR.bit.HW_CONTROL = 0;
}

/**
 * @brief  Presleep/ Postawake Callbacks
 * @param  evt - Denotes the sleep event
 * @return None
 */
void pka_power_state_change_cb(uint8_t evt, void *p_args)
{
    (void)p_args;

    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        pka_deinit(&g_pka_ctxt);
    }
    if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        pka_init(&g_pka_ctxt);
    }
}

int pka_init(pka_state_t *ctxt)
{
    if (pka_init_done)
        return 0;

    // power switch to config
    pka_power_switch_to_config();
    // enable PKA clock
    pka_enable_clock(ctxt);

    // initialize PKA HW lock
    ecc_lock_init(&ctxt->lock_ctxt, (uint32_t *)ECC_LOCK_WRAPPER);

    // initialize the PKA hardware
    g_elppka_ctxt = &ctxt->elppka_ctxt;
    elppka_init(&ctxt->elppka_ctxt, (uint32_t *)ELP_PKA_AHB);

    // obtain the PKA HW lock
    do {
        ecc_lock_request(&ctxt->lock_ctxt);
    } while (!ecc_lock_is_locked_by_us(&ctxt->lock_ctxt));

    // cancel pending PKA operation
    elppka_cancel_outstanding_operation(&ctxt->elppka_ctxt);

    // load PKA FW
    elppka_fw_load(&ctxt->elppka_ctxt);

    // todo:initialize the low power mode lock (to control whether the chip can or cannot go to sleep)
    /*
    ctxt->unpa_client_ctxt = unpa_create_client("pka", UNPA_CLIENT_REQUIRED, g_unpa_resource_name);
    if ( !ctxt->unpa_client_ctxt ) {
        // disable PKA lock
        pka_disable_clock(ctxt);
        return -1;
    }
    */

    // disable PKA clock
    pka_disable_clock(ctxt);

    // NO PKA HW access below this point !!!

    // initialize the SW mutex so only a single thread can access the PKA HW at a time
    qurt_mutex_create(&ctxt->mutex);

    fpci_evt_cb_reg((ps_evt_cb_t)&pka_power_state_change_cb,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, 10, NULL);

    pka_init_done = 1;
    return 0;
}

int pka_deinit(pka_state_t *ctxt)
{
    if (!pka_init_done)
        return 0;

    if (ctxt->mutex) {
        qurt_mutex_delete(&ctxt->mutex);
        ctxt->mutex = 0;
    }
    pka_init_done = 0;
    return 0;
}

int pka_lock(pka_state_t *ctxt, pka_operand_endianness_t pka_operand_endianness)
{
    // obtain the SW lock
    if (TAKE_LOCK(ctxt->mutex)) {
        // enable PKA clock
        pka_enable_clock(ctxt);
        // todo: obtain the low power mode lock (so the chip doesn't go to sleep)
        // unpa_issue_request(ctxt->unpa_client_ctxt, QAPI_SLP_LAT_PERF);
        // set endianess for PKA
        pka_set_endianess(ctxt, pka_operand_endianness);
        return 0;
    } else {
        // should never come here
        return -1;
    }
}

int pka_unlock(pka_state_t *ctxt)
{
    // todo: release the low power mode lock (so the chip can go to sleep)
    // unpa_cancel_request(ctxt->unpa_client_ctxt);
    // disable PKA clock
    pka_disable_clock(ctxt);
    RELEASE_LOCK(ctxt->mutex);
    return 0;
}

pka_state_t *GET_PKA_CTXT(void *ctxt)
{
    (void)ctxt;
    return &g_pka_ctxt;
}
