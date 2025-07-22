/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "printfext.h"
#include <ctype.h>

#include "ferm_hkadc_drv.h"

#include "timer.h"
#include "timer_internal.h"

void hkadc_drv_dump(const char *title)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;
    volatile uint32_t data = 0;
    RPMU_BASE_rpmu_Type *const p_rpmu = ((RPMU_BASE_rpmu_Type *)QCC730V2_RPMU_BASE_BASE);

    if (title) {
        info_printf("%s\n", title);
    }

    info_printf(
        "PMU_CFG_HKADC_DATA_AVG_CNT reg(*0x%x)=0x%x CFG_HKADC_DATA_AVG_CNT=%d CFG_TEMP_VBATT_MON_SEL=%d "
        "CFG_HKADC_DIV_CLK_EN=%d\n",
        &p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.reg, p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.reg,
        p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_HKADC_DATA_AVG_CNT,
        p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_TEMP_VBATT_MON_SEL,
        p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_HKADC_DIV_CLK_EN);
    info_printf(
        "PMU_CFG_ACAL_VBAT_MON_EN reg(*0x%x)=0x%x TEMP_MON_EN=%d VBAT_MON_EN=%d AUTO_TEMP_MON_EN=%d "
        "CFG_TEMP_MON_DONE_INTR_EN=%d CFG_VBAT_MON_DONE_INTR_EN=%d AUTO_VBATT_MON_EN=%d\n",
        &p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.reg, p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.reg,
        p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.TEMP_MON_EN, p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.VBAT_MON_EN,
        p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.AUTO_TEMP_MON_EN,
        p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.CFG_TEMP_MON_DONE_INTR_EN,
        p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.CFG_VBAT_MON_DONE_INTR_EN,
        p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.AUTO_VBATT_MON_EN);
#if 0 
    log_printf("PMU_TEMP_SNR_RD_DATA reg(*0x%x)=0x%x TC_MEASURED_DATA=%d TC_MEASURED_DATA_VALID=%d\n",
        &p_hkadc->PMU_TEMP_SNR_RD_DATA.reg, p_hkadc->PMU_TEMP_SNR_RD_DATA.reg,
        p_hkadc->PMU_TEMP_SNR_RD_DATA.bit.TC_MEASURED_DATA,
        p_hkadc->PMU_TEMP_SNR_RD_DATA.bit.TC_MEASURED_DATA_VALID);
    log_printf("PMU_VBAT_MON_RD_DATA reg(*0x%x)=0x%x VBAT_MON_DATA=%d VBAT_MON_DATA_VALID=%d\n",
        &p_hkadc->PMU_VBAT_MON_RD_DATA.reg, p_hkadc->PMU_VBAT_MON_RD_DATA.reg,
        p_hkadc->PMU_VBAT_MON_RD_DATA.bit.VBAT_MON_DATA,
        p_hkadc->PMU_VBAT_MON_RD_DATA.bit.VBAT_MON_DATA_VALID);
#else
    HKADC_READ_DATA(data, &p_hkadc->PMU_TEMP_SNR_RD_DATA.reg);
    info_printf("PMU_TEMP_SNR_RD_DATA reg(*0x%x)=0x%x TC_MEASURED_DATA=%d TC_MEASURED_DATA_VALID=0x%x\n",
                &p_hkadc->PMU_TEMP_SNR_RD_DATA.reg, data,
                (data & PMU_BASE_pmu_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_Msk),
                (data & PMU_BASE_pmu_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_Msk));
    HKADC_READ_DATA(data, &p_hkadc->PMU_VBAT_MON_RD_DATA.reg);
    info_printf("PMU_VBAT_MON_RD_DATA reg(*0x%x)=0x%x VBAT_MON_DATA=%d VBAT_MON_DATA_VALID=0x%x\n",
                &p_hkadc->PMU_VBAT_MON_RD_DATA.reg, data, (data & PMU_BASE_pmu_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_Msk),
                (data & PMU_BASE_pmu_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_VALID_Msk));
#endif

    info_printf("PMU_CFG_TEMP_MON_INTERVAL reg(*0x%x)=0x%x\n", &p_hkadc->PMU_CFG_TEMP_MON_INTERVAL.reg,
                p_hkadc->PMU_CFG_TEMP_MON_INTERVAL.reg);

    // log_printf("PMU_CFG_TEMP_MON_TH reg=0x%x CFG_TEMP_PANIC_HIGH_TH=%d\n", p_hkadc_reg->PMU_CFG_TEMP_MON_TH.reg,
    // p_hkadc_reg->PMU_CFG_TEMP_MON_TH.bit.CFG_TEMP_PANIC_HIGH_TH); log_printf("PMU_CFG_VABT_MON_TH reg=0x%x
    // CFG_VBAT_LOW_TH=%d\n", p_hkadc_reg->PMU_CFG_VABT_MON_TH.reg,
    // p_hkadc_reg->PMU_CFG_VABT_MON_TH.bit.CFG_VBAT_LOW_TH);

    info_printf("ulpsmps2 OTP_oneshot=%d\n", ulpsmps2_get_OTP_oneshot());

    info_printf(
        "RPMU_R_PMU_SMPS2_18 reg(*0x%x)=0x%x SMPS2_ONESHOT_TRIM=%d SMPS2_ULP_COMP_VREF_TRIM=%d SMPS2_ULPM_VFB_SRC=%d\n",
        &p_rpmu->RPMU_R_PMU_SMPS2_18.reg, p_rpmu->RPMU_R_PMU_SMPS2_18.reg,
        p_rpmu->RPMU_R_PMU_SMPS2_18.bit.SMPS2_ONESHOT_TRIM, p_rpmu->RPMU_R_PMU_SMPS2_18.bit.SMPS2_ULP_COMP_VREF_TRIM,
        p_rpmu->RPMU_R_PMU_SMPS2_18.bit.SMPS2_ULPM_VFB_SRC);
}

int32_t hkadc_single_temp_monitor_get_raw(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;
    volatile uint32_t data_valid = 0;
    int32_t max_wait_cnt = CONFIG_HDADC_DRV_TEMP_TIMEOUT_COUNT;
    int32_t ret = 0;
    volatile uint32_t data = 0;

    hkadc_single_temp_monitor_enable();
    do {
        HKADC_READ_DATA(data, &p_hkadc->PMU_TEMP_SNR_RD_DATA.reg);
        // data_valid = p_hkadc->PMU_TEMP_SNR_RD_DATA.bit.TC_MEASURED_DATA_VALID;
        data_valid = (data & PMU_BASE_pmu_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_Msk);
        max_wait_cnt--;
    } while ((!data_valid) && (max_wait_cnt >= 0));
    if (data_valid) {
        // ret = p_hkadc->PMU_TEMP_SNR_RD_DATA.bit.TC_MEASURED_DATA;
        ret = (data & PMU_BASE_pmu_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_Msk);
    } else {
        info_printf("ERROR: hkadc get temp data time out cnt=%d\n", CONFIG_HDADC_DRV_TEMP_TIMEOUT_COUNT);
        ret = QAPI_HKADC_DATA_TIME_OUT;
    }
    dump_printf("%s %d data=0x%x data_valid=0x%x adcdata=0x%x max_wait_cnt=%d\n", __FUNCTION__, __LINE__, data,
                data_valid, ret, max_wait_cnt);
    return ret;
}

int32_t hkadc_single_vbat_monitor_get_raw(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;
    volatile uint32_t data_valid = 0;
    int32_t max_wait_cnt = CONFIG_HDADC_DRV_VBAT_TIMEOUT_COUNT;
    int32_t ret = 0;
    volatile uint32_t data = 0;

    hkadc_single_vbat_monitor_enable();
    do {
        HKADC_READ_DATA(data, &p_hkadc->PMU_VBAT_MON_RD_DATA.reg);
        // data_valid = p_hkadc->PMU_VBAT_MON_RD_DATA.bit.VBAT_MON_DATA_VALID;
        data_valid = (data & PMU_BASE_pmu_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_VALID_Msk);
        max_wait_cnt--;
    } while ((!data_valid) && (max_wait_cnt >= 0));
    if (data_valid) {
        // ret = p_hkadc->PMU_VBAT_MON_RD_DATA.bit.VBAT_MON_DATA;
        ret = (data & PMU_BASE_pmu_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_Msk);
    } else {
        info_printf("hkadc get vbat data time out cnt=%d\n", CONFIG_HDADC_DRV_VBAT_TIMEOUT_COUNT);
        ret = QAPI_HKADC_DATA_TIME_OUT;
    }
    dump_printf("%s %d data=0x%x data_valid=0x%x adcdata=0x%x max_wait_cnt=%d\n", __FUNCTION__, __LINE__, data,
                data_valid, ret, max_wait_cnt);
    return ret;
}

/* test code */
#if 0
/*
void test_alg (uint32_t cc_vbat_milivolt, uint32_t temp_deg)
{
    uint32_t t_one_shot_ns = (((2670*450)-120*(cc_vbat_milivolt-630))*(temp_deg - 40))/(55*(cc_vbat_milivolt-630)) + 120;
    info_printf("Input: cc_vbat_milivolt=%dmV temp_deg=%dC\n", cc_vbat_milivolt, temp_deg);
    info_printf("Output: t_one_shot_ns=%dns\n", t_one_shot_ns);
}
*/

/*
void test_alg (uint32_t cc_vbat_milivolt, uint32_t temp_deg)
{
    uint32 d1 = ((2670*450)*(temp_deg - 40))/((cc_vbat_milivolt - 630)*55);
    float d3 = (120*(95 - temp_deg))/55;

    uint32 d4 = (uint32)(d1 * d2);
    uint32 d5 = (uint32)(d3 * ((float)120));
    uint32 t_one_shot_ns = d4 + d5;
    info_printf("Input: cc_vbat_milivolt=%dmV temp_deg=%dC\n", cc_vbat_milivolt, temp_deg);
    info_printf("Output: d1=%d d2=%d d3=%d d4=%d d5=%d t_one_shot_ns=%dns\n", (uint32)d1, (uint32)d2, (uint32)d3, d4, d5, t_one_shot_ns);
}
*/

void ulpsmps2_oneshot_test (int32_t temp_deg, uint32_t cc_vbat_milivolt)
{


    LOG_FUNC_LINE_ENTRY;

/*
    test_alg(2600, 95);
    test_alg(3300, 95);
    test_alg(3600, 95);
    test_alg(2600, 40);
    test_alg(3300, 40);
    test_alg(3600, 40);
*/

    ulpsmps2_get_optimized_oneshot_code(cc_vbat_milivolt, temp_deg, NULL, NULL);
    ulpsmps2_get_optimized_oneshot_code(2600, 95, NULL, NULL);
    ulpsmps2_get_optimized_oneshot_code(3300, 95, NULL, NULL);
    ulpsmps2_get_optimized_oneshot_code(3600, 95, NULL, NULL);
    ulpsmps2_get_optimized_oneshot_code(2600, 40, NULL, NULL);
    ulpsmps2_get_optimized_oneshot_code(3300, 40, NULL, NULL);
    ulpsmps2_get_optimized_oneshot_code(3600, 40, NULL, NULL);

    LOG_FUNC_LINE_EXIT;

}

void hkadc_test (void)
{
    uint64_t ts_before = 0;
    uint64_t ts_after = 0;
    uint64_t ts_diff_hkadc = 0;
    uint64_t ts_diff_1 = 0;
    uint64_t ts_diff_2 = 0;
    uint64_t ts_diff_3 = 0;
    uint64_t ts_diff_4 = 0;
    uint64_t ts_diff_5 = 0;
    uint64_t ts_diff_6 = 0;
    int32_t reg_data = 0;
    int32_t temp_deg = 0;
    int32_t cc_temp_deg = 0;
    int32_t cc_temp_deg_b = 0;
    int32_t temp_deg_sum = 0;
    int32_t temp_deg_avg = 0;
    int32_t temp_deg_cnt = 0;
    int temp_loops = 10;
    uint32_t cc_vbat_milivolt = 0;
    uint32_t cc_vbat_milivolt_b = 0;
    uint32_t cc_vbat_milivolt_c = 0;
    uint32_t cc_vbat_milivolt_d = 0;
    uint32_t cc_vbat_milivolt_e = 0;
    uint32_t cc_vbat_milivolt_f = 0;
    uint32_t cc_vbat_milivolt_sum = 0;
    uint32_t cc_vbat_milivolt_avg = 0;
    uint32_t cc_vbat_milivolt_cnt = 0;

    int vbat_loops = 10;
    int i;

    LOG_FUNC_LINE_ENTRY;

    //pre_init
    TIMER_INIT_HW();
    LOG_FUNC_LINE_STR("vm read temp");
    pmu_ts_init();

    hkadc_drv_dump("before hkadc reset");
    hkadc_reset();
    hkadc_drv_dump("after hkadc reset");

    for (i=0; i<temp_loops; i++) {
        ts_before = hres_timer_curr_time_us();
        reg_data = hkadc_single_temp_monitor_get_raw();
        if (reg_data < 0) {
            continue;
        }
        ts_after = hres_timer_curr_time_us();
        ts_diff_hkadc = ts_after - ts_before;
        //hkadc_dump("read temp");
        //dump_printf("reg_data=0x%x\n", reg_data);

        ts_before = hres_timer_curr_time_us();
        temp_deg = pmu_ts_convert_to_deg_cel(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_1 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_temp_deg = cc_rawtemp2degcel(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_2 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_temp_deg_b = cc_rawtemp2degcel_b(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_3 = ts_after - ts_before;

        temp_deg_sum += temp_deg;
        temp_deg_cnt++;

        info_printf("temp i=%d t=%dus reg_data=0x%x VM=%dC cc_float=%dC cc_int=%dC\n", i, (uint32_t)ts_diff_hkadc, reg_data, temp_deg, cc_temp_deg, cc_temp_deg_b);
        info_printf("VM %dus cc_float %dus cc_int %dus\n", (uint32_t)ts_diff_1, (uint32_t)ts_diff_2, (uint32_t)ts_diff_3);
    }
    if (temp_deg_cnt > 0) {
        temp_deg_avg = temp_deg_sum/temp_deg_cnt;
    }
    info_printf("temp_deg_avg=%dC valid cnt=%d\n", temp_deg_avg, temp_deg_cnt);

    for (i=0; i<vbat_loops; i++) {
        ts_before = hres_timer_curr_time_us();
        reg_data = hkadc_single_vbat_monitor_get_raw();
        if (reg_data < 0) {
            continue;
        }
        ts_after = hres_timer_curr_time_us();
        ts_diff_hkadc = ts_after - ts_before;
        //hkadc_dump("read vbat");
        //dump_printf("reg_data=0x%x\n", reg_data);

        ts_before = hres_timer_curr_time_us();        
        cc_vbat_milivolt = cc_rawvbat2milivolt(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_1 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_vbat_milivolt_b = cc_rawvbat2milivolt_b(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_2 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_vbat_milivolt_c = cc_rawvbat2milivolt_c(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_3 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_vbat_milivolt_d = cc_rawvbat2milivolt_d(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_4 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_vbat_milivolt_e = cc_rawvbat2milivolt_e(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_5 = ts_after - ts_before;

        ts_before = hres_timer_curr_time_us();
        cc_vbat_milivolt_f = cc_rawvbat2milivolt_f(reg_data);
        ts_after = hres_timer_curr_time_us();
        ts_diff_6 = ts_after - ts_before;

        cc_vbat_milivolt_sum += cc_vbat_milivolt_b;
        cc_vbat_milivolt_cnt++;

        info_printf("vbat milivolt i=%d t=%dus reg_data=0x%x cc_50=%dmV cc_16_float=%dmV cc_16_int=%dmV cc_int_26_1850=%dmV cc_int_26_1800=%dmV cc_int_50_2000=%dmV\n", 
            i, (uint32_t)ts_diff_hkadc, reg_data, cc_vbat_milivolt, cc_vbat_milivolt_b, cc_vbat_milivolt_c, cc_vbat_milivolt_d, cc_vbat_milivolt_e, cc_vbat_milivolt_f);
        info_printf("cc_50 %dus cc_16_float %dus cc_16_int %dus cc_int_26_1850=%dus cc_int_26_1800=%dus cc_int_50_2000=%dus\n", 
            (uint32_t)ts_diff_1, (uint32_t)ts_diff_2, (uint32_t)ts_diff_3, (uint32_t)ts_diff_4, (uint32_t)ts_diff_5, (uint32_t)ts_diff_6);
    }
    if (cc_vbat_milivolt_cnt > 0) {
        cc_vbat_milivolt_avg = cc_vbat_milivolt_sum/cc_vbat_milivolt_cnt;
    }
    info_printf("cc_vbat_milivolt_avg=%dmV valid cnt=%d\n", cc_vbat_milivolt_avg, cc_vbat_milivolt_cnt);

    ulpsmps2_oneshot_test(temp_deg_avg, cc_vbat_milivolt_avg);

    LOG_FUNC_LINE_EXIT;
}
#endif

PMU_BASE_pmu_Type *const p_hkadc_reg = ((PMU_BASE_pmu_Type *)QCC730V2_PMU_BASE_BASE);
RPMU_BASE_rpmu_Type *const p_rpmu = ((RPMU_BASE_rpmu_Type *)QCC730V2_RPMU_BASE_BASE);

bool __attribute__((section(".__sect_ps_txt"))) hkadc_get_temp_raw_data(uint32_t *temp_data)
{
    volatile uint32_t data = 0;

    HKADC_READ_DATA(data, &p_hkadc_reg->PMU_TEMP_SNR_RD_DATA.reg);
    if (temp_data) {
        *temp_data = (data & PMU_BASE_pmu_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_Msk);
    }

    return !!(data & PMU_BASE_pmu_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_Msk);
}

bool __attribute__((section(".__sect_ps_txt"))) hkadc_get_vbat_raw_data(uint32_t *vbat_data)
{
    volatile uint32_t data = 0;

    HKADC_READ_DATA(data, &p_hkadc_reg->PMU_VBAT_MON_RD_DATA.reg);
    if (vbat_data) {
        *vbat_data = (data & PMU_BASE_pmu_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_Msk);
    }

    return !!(data & PMU_BASE_pmu_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_VALID_Msk);
}

void __attribute__((section(".__sect_ps_txt"))) hkadc_stop(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;

    // LOG_FUNC_LINE;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.TEMP_MON_EN = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.VBAT_MON_EN = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.AUTO_TEMP_MON_EN = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.AUTO_VBATT_MON_EN = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.CFG_TEMP_MON_DONE_INTR_EN = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.CFG_VBAT_MON_DONE_INTR_EN = 0;
}

void hkadc_reset(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;

    // LOG_FUNC_LINE;
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_HKADC_DATA_AVG_CNT = 0x3;
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_TEMP_VBATT_MON_SEL = 0;
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_HKADC_DIV_CLK_EN = 0;
    hkadc_stop();
}

void __attribute__((section(".__sect_ps_txt"))) hkadc_single_temp_monitor_enable(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;

    hkadc_stop();
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_TEMP_VBATT_MON_SEL = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.TEMP_MON_EN = 1;
}

void __attribute__((section(".__sect_ps_txt"))) hkadc_single_vbat_monitor_enable(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;

    hkadc_stop();
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_TEMP_VBATT_MON_SEL = 1;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.VBAT_MON_EN = 1;
}

void hkadc_auto_temp_monitor_enable(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;

    hkadc_stop();
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_TEMP_VBATT_MON_SEL = 0;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.AUTO_TEMP_MON_EN = 1;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.CFG_TEMP_MON_DONE_INTR_EN = 1;
    return;
}

void hkadc_auto_vbat_monitor_enable(void)
{
    PMU_BASE_pmu_Type *const p_hkadc = p_hkadc_reg;

    hkadc_stop();
    p_hkadc->PMU_CFG_HKADC_DATA_AVG_CNT.bit.CFG_TEMP_VBATT_MON_SEL = 1;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.AUTO_VBATT_MON_EN = 1;
    p_hkadc->PMU_CFG_ACAL_VBAT_MON_EN.bit.CFG_VBAT_MON_DONE_INTR_EN = 1;
    return;
}

#include "cortexm/ExceptionHandlers.h"
#include "nt_common.h"
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_enable_vbatt_temp_mon_done_int
 * @brief     : Enable pmu_ccpu_temp_mon_done_intr Interuupt //Device Specific 79
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void hkadc_temp_monitor_done_sys_intr_enable(bool enable)
{
    uint32_t temp1;
    uint32_t bit_temp = (0x1 << 15);  // 64+15=79, Device Specific 79

    if (enable) {
        temp1 = bit_temp | NT_REG_RD(NVIC_ISER2);
        NT_REG_WR(NVIC_ISER2, temp1);
    } else {
        temp1 = bit_temp | NT_REG_RD(NVIC_ICER2);
        NT_REG_WR(NVIC_ICER2, temp1);
    }
}

void hkadc_vbat_monitor_done_sys_intr_enable(bool enable)
{
    uint32_t temp1;
    uint32_t bit_temp = (0x1 << 16);  // 64+16=80, Device Specific 80

    if (enable) {
        temp1 = bit_temp | NT_REG_RD(NVIC_ISER2);
        NT_REG_WR(NVIC_ISER2, temp1);
    } else {
        temp1 = bit_temp | NT_REG_RD(NVIC_ICER2);
        NT_REG_WR(NVIC_ICER2, temp1);
    }
    return;
}

#if 0
#define LOWEST_TEMPERATURE_IN_CELSIUS                  -40  // lowest temperature in celcius
#define LOWEST_TEMPERATURE_REG_READ                    50   // lowest temperature in the look up table(not in celcius)
#define TEMPERATURE_INCREMENT                          0.40244  // temperature difference for one point change
#define LOWEST_VOLTAGE_IN_VOLTS                        1.6      // lowest voltage in volts
#define LOWEST_VOLTAGE_REG_READ                        50       // lowest voltage in the look up table(not in volts)
#define VOLTAGE_INCREMENT                              0.0049   // voltage difference for one point change
#define QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_MIN_SAMPLES 0x3
#define VBATT_RINT_TIMEOUT_CONFIG                      0x32  // TPE will stop vbatt measurement this many(50) u seconds after last TX
                                                                                // if vbatt does not reach rint_vbatt_threshold
#define VBATT_RINT_VBATT_START_THRESHOLD_CONFIG \
    0xDD  // 2.5 V,First sample of Vbatt is greater than this Rint measurement will be cancelled
#define VBATT_RINT_VBATT_THRESHOLD_CONFIG 0xDD  // 2.5 V, initial vbatt_threshold_config, TPE will continue to measure
																				//vbatt samples until average is greater than this threshold
#define TXOP_VBATT_THRESHOLD_CONFIG \
    0x3F  // 1.8 V(c7/b0/99/77/6b/56/49/3F) vbatt_threshold_config, TPE will continue to measure
																				//vbatt samples until average is greater than this threshold
#define TPE_SW_RINT_VBATT_RINT_VBATT_THRESHOLD_MASK 0x1FF  // Mask to disable rint_vbatt_threshold bits
#define VBATT_RINT_AVERAGE_SAMPLES_CONFIG           0x1    // 2 samples will be taken for averaging
#define VBATT_TXOP_AVERAGE_SAMPLES_CONFIG           0x1    // 2 samples will be taken for averaging
#define VBATT_RINT_MIN_SAMPLES_CONFIG               0x1    // after this much samples taken Rint will be measured
#define VBATT_TXOP_MIN_SAMPLES_CONFIG               0x1    //	after this much samples taken TXoP will be adjusted
#define VBATT_RINT_VBATT_WINDOW_CONFIG \
    0xAAA  // random value for now to configure vbatt_window time for rint measurement

int32_t cc_rawtemp2degcel(uint32_t raw_temp)
{
    float value = (float)raw_temp;

    value = ((value-LOWEST_TEMPERATURE_REG_READ)*TEMPERATURE_INCREMENT ) + LOWEST_TEMPERATURE_IN_CELSIUS;
    return (int32_t)value;
}

uint32_t hkadc_degcel2rawtemp_ideal (int32_t temp)
{
    uint32_t rawtemp = (uint32_t)(temp+40);
    return (((rawtemp*410)/165)+50);
}

uint32_t cc_rawvbat2milivolt(uint32_t raw_temp)
{
    float value = (float)raw_temp;

    /* converting to whole number and millivolt */
    //2.0/410=0.0048780
    value = (((value - LOWEST_VOLTAGE_REG_READ)*VOLTAGE_INCREMENT)+LOWEST_VOLTAGE_IN_VOLTS)*1000;
    return (uint32_t)value;
}

uint32_t cc_rawvbat2milivolt_b(uint32_t hx_voltage)
{
    float voltage = (float)hx_voltage;

    voltage = (((voltage - 16 ) * ( 1.8 / 410 )) + 1.6)*1000;
    return (uint32_t)voltage;
}

uint32_t cc_rawvbat2milivolt_c(uint32_t hx_voltage)
{
    int32_t voltage = (int32_t)hx_voltage;

    //1.8/410=0.004390243902439025
    voltage = ((voltage-16)*439)/100 + 1600;
    return (uint32_t)voltage;
}

uint32_t cc_rawvbat2milivolt_d(uint32_t hx_voltage)
{
    int32_t voltage = (int32_t)hx_voltage;

    voltage = ((voltage-26)*1850)/410 + 1600;
    return (uint32_t)voltage;
}

uint32_t cc_rawvbat2milivolt_e(uint32_t hx_voltage)
{
    int32_t voltage = (int32_t)hx_voltage;

    voltage = ((voltage-26)*1800)/410 + 1600;
    return (uint32_t)voltage;
}
#endif

int32_t __attribute__((section(".__sect_ps_txt"))) hkadc_temp_raw2C(uint32_t raw_data)
{
    int32_t temp_celc = (int32_t)raw_data;

    // in neutrino1 FPU is not enabled.
    // the actual equation is temperature_in_celcious= (((temp_decimal-50)*(165/410))-40)
    //(165/410)=0.4024
    // to get that 0.4024*10000=4024
    // to balance the equation  40*10000 and the result is divided by 10000

    temp_celc = (((temp_celc - 50) * 165) / 410) - 40;
    return temp_celc;
}

uint32_t __attribute__((section(".__sect_ps_txt"))) hkadc_vbat_raw2mV(uint32_t raw_data)
{
    int32_t voltage = (int32_t)raw_data;

    voltage = ((voltage - 50) * 2000) / 410 + 1600;
    return (uint32_t)voltage;
}

uint32_t __attribute__((section(".__sect_ps_txt"))) ulpsmps2_get_OTP_oneshot(void)
{
    return HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                     FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW9_W0,
                     SMPS2_ONESHOT_TRIM);  // bit 6:11
}

uint32_t __attribute__((section(".__sect_ps_txt"))) ulpsmps2_get_oneshot(void)
{
#if 0
    RPMU_BASE_rpmu_Type * const p_rpmu = ((RPMU_BASE_rpmu_Type *)QCC730V2_RPMU_BASE_BASE);
    return p_rpmu->RPMU_R_PMU_SMPS2_18.bit.SMPS2_ONESHOT_TRIM;
#else
    volatile uint32_t data = 0;
    HKADC_READ_DATA(data, &p_rpmu->RPMU_R_PMU_SMPS2_18.reg);
    return (data & RPMU_BASE_rpmu_RPMU_R_PMU_SMPS2_18_SMPS2_ONESHOT_TRIM_Msk);
#endif
}

void __attribute__((section(".__sect_ps_txt"))) ulpsmps2_set_oneshot(uint32_t oneshot)
{
#if 0
    RPMU_BASE_rpmu_Type * const p_rpmu = ((RPMU_BASE_rpmu_Type *)QCC730V2_RPMU_BASE_BASE);
    p_rpmu->RPMU_R_PMU_SMPS2_18.bit.SMPS2_ONESHOT_TRIM = oneshot;
#else
    volatile uint32_t data = 0;
    HKADC_READ_DATA(data, &p_rpmu->RPMU_R_PMU_SMPS2_18.reg);
    data = ((data & (~RPMU_BASE_rpmu_RPMU_R_PMU_SMPS2_18_SMPS2_ONESHOT_TRIM_Msk)) | oneshot);
    HKADC_WRITE_DATA(data, &p_rpmu->RPMU_R_PMU_SMPS2_18.reg);
    HKADC_READ_DATA(data, &p_rpmu->RPMU_R_PMU_SMPS2_18.reg);
#if 0
    {
        
        do {
            HKADC_READ_DATA(data2, &p_rpmu->RPMU_R_PMU_SMPS2_18.reg);
        } while (data2!=data);
    }
#endif
#endif
}

#include "timer.h"
#include "timer_internal.h"

uint32_t __attribute__((section(".__sect_ps_txt")))
ulpsmps2_get_optimized_oneshot(uint32_t cc_vbat_milivolt, int32_t temp_deg, uint32_t *p_OTP_oneshot,
                               uint32_t *p_t_one_shot_ns)
{
    int32_t final_code = 0;
    uint32_t OTP_oneshot = ulpsmps2_get_OTP_oneshot();

    if (p_OTP_oneshot) {
        *p_OTP_oneshot = OTP_oneshot;
    }

    if (temp_deg < TEMPERATUREC_GOLDEN) {
        final_code = CX_ONESHOT_GOLDEN;
    } else {
        // uint64_t ts_before = hres_timer_curr_time_us();
        // float f_t_one_shot_ns = (((2.67*450)/(cc_vbat_milivolt/1000 - 0.63) - 120) * (temp_deg - 40))/55 + 120;
        uint32_t t_one_shot_ns =
            (((2670 * 450) - 120 * (cc_vbat_milivolt - 630)) * (temp_deg - 40)) / (55 * (cc_vbat_milivolt - 630)) + 120;
        int32_t delta_t = ((int32_t)t_one_shot_ns) - 450;
        int32_t delta_code = delta_t / 35;
        int32_t new_code = (int32_t)OTP_oneshot + delta_code;
        // uint64_t ts_after = hres_timer_curr_time_us();
        // uint64_t ts_diff = ts_after - ts_before;
        final_code = new_code;

        if (final_code < CX_ONESHOT_GOLDEN) {
            final_code = CX_ONESHOT_GOLDEN;
        } else if (final_code > CX_ONESHOT_MAX) {
            final_code = CX_ONESHOT_MAX;
        }
        if (p_t_one_shot_ns) {
            *p_t_one_shot_ns = t_one_shot_ns;
        }
        // log_printf("Input: OTP_oneshot=%d cc_vbat_milivolt=%dmV temp_deg=%dC\n", OTP_oneshot, cc_vbat_milivolt,
        // temp_deg); log_printf("Output: t=%dus t_one_shot_ns=%dns delta_t=%dns delta_code=%d new_code=%d
        // final_code=%d\n", (uint32_t)ts_diff, t_one_shot_ns, delta_t, delta_code, new_code, final_code);
    }

    return (uint32_t)final_code;
}
