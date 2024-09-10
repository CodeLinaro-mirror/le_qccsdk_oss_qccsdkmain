/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pka.h"
#include "pka_internal.h"
#include "elppka.h"
#include "elppka_cmds.h"

const unsigned char array_of_zeros[64] = { 0 };

pka_state_t g_pka_ctxt;

int pka_set_endianess(pka_state_t * ctxt, pka_operand_endianness_t pka_operand_endianness)
{
    ctxt = GET_PKA_CTXT(ctxt);
    int are_operands_in_big_endian_format = (pka_operand_endianness == PKA_OPERAND_ENDIANNESS_BIG_ENDIAN);
    elppka_setup(&ctxt->elppka_ctxt, are_operands_in_big_endian_format);
    return 0;
}


#define RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt) (ctxt->elppka_ctxt.cfg.rsa_size/sizeof(uint8_t))
#define ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt) (ctxt->elppka_ctxt.cfg.ecc_size/sizeof(uint8_t))


int pka_calc_r_inv(
    pka_state_t * ctxt,
    const uint8_t * modulus_m,
    uint32_t size,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != modulus_m) &&
        (size > 0) &&
        (size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid) {
        return -1;
    }
    
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_CALC_R_INV, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_calc_m_prime(
    pka_state_t * ctxt,
    const pka_calc_m_prime_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->r_inv) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, params->r_inv);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_CALC_MP, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 1, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_calc_r_sqr(
    pka_state_t * ctxt,
    const pka_calc_r_sqr_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->r_inv) &&
        (0 != params->modulus_m) &&
        (params->size > 0 ) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, params->r_inv);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_CALC_R_SQR, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 3, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_modmult(
    pka_state_t * ctxt,
    const pka_modmult_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input_x) &&
        (0 != params->input_y) &&
        (0 != params->modulus_m) &&
        (0 != params->m_prime) &&
        (0 != params->r_sqr) &&
        (params->size > 0 ) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, params->input_x);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 0, size, params->input_y);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 1, size, params->m_prime);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 3, size, params->r_sqr);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_MODMULT, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_modadd(
    pka_state_t * ctxt,
    const pka_modadd_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input_x) &&
        (0 != params->input_y) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }

    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, params->input_x);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 0, size, params->input_y);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_MODADD, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_modsub(
    pka_state_t * ctxt,
    const pka_modsub_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input_x) &&
        (0 != params->input_y) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, params->input_x);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 0, size, params->input_y);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_MODSUB, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_modred(
    pka_state_t * ctxt,
    const pka_reduce_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, params->input);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }

    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_REDUCE, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_reduce(
    pka_state_t * ctxt,
    const pka_reduce_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));

    if ( !are_parameters_valid ) {
        return -1;
    }

    int status = -2;

    if ( params->size <= sizeof(array_of_zeros))
    {
        pka_crt_bit_serial_modred_double_precision_params_t pka_crt_bit_serial_modred_double_precision_params = {
            .x_lo = params->input,
            .x_hi = array_of_zeros,
            .modulus_m = params->modulus_m,
            .size = params->size
        };
        status = pka_crt_bit_serial_modred_double_precision(
            ctxt,
            &pka_crt_bit_serial_modred_double_precision_params,
            result
            );
    }
    else {
        status = pka_modred(ctxt, params, result);
    }

    return status;
}


int pka_moddiv(
    pka_state_t * ctxt,
    const pka_moddiv_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input_x) &&
        (0 != params->input_y) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, params->input_x);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, params->input_y);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_MODDIV, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_modinv(
    pka_state_t * ctxt,
    const pka_modinv_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->input) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;

    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, params->input);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    if ( 0 != status ) {
        return status;
    }
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_MODINV, flags, size);
    if ( 0 != status) {
        return status;
    }
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, result);
    if ( 0 != status ) {
        return status;
    }
    
    return 0;
}


int pka_modexp(
    pka_state_t * ctxt,
    const pka_modexp_params_t * params,
    uint8_t * result
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result) &&
        (0 != params) &&
        (0 != params->base) &&
        (0 != params->exponent) &&
        (0 != params->modulus_m) &&
        (0 != params->m_prime) &&
        (0 != params->r_sqr) &&
        (params->size > 0) &&
        (params->size <= RSA_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;
    
    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, params->base);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 2, size, params->exponent);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 1, size, params->m_prime);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 3, size, params->r_sqr);

    if ( params->enable_timing_attack_hardening ) {
        // enable "Timing Neutral Square-and-Multiply Algorithm"
        elppka_set_jump_probability_register(&ctxt->elppka_ctxt, PKA_JUMP_PROBABILITY_100_PERCENT);

        // enable "Time-Invariant Square-and-Multiply Algorithm"
        flags = elppka_set_f0_flag(flags);
    }
    else {
        elppka_set_jump_probability_register(&ctxt->elppka_ctxt, PKA_JUMP_PROBABILITY_0_PERCENT);
    }

    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_MODEXP, flags, size);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, result);
    return status;
}


int pka_crt_modexp(
    pka_state_t * ctxt,
    const  pka_crt_modexp_params_t * params,
    uint8_t * result_c_lo,
    uint8_t * result_c_hi
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_c_lo) &&
        (0 != result_c_hi) &&
        (0 != params) &&
        (0 != params->msg_lo) &&
        (0 != params->msg_hi) &&
        (0 != params->p ) &&
        (0 != params->q ) &&
        (0 != params->u ) &&
        (0 != params->d_p ) &&
        (0 != params->d_q ) &&
        (0 != params->r_sqr_mod_p ) &&
        (0 != params->p_prime ) &&
        (0 != params->r_sqr_mod_q ) &&
        (0 != params->q_prime ) &&
        (params->size > 0) &&
        (params->size <= (ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt) / 2));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    uint32_t flags = 0;
    
    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, params->msg_lo);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 3, size, params->msg_hi);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, params->p);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 3, size, params->q);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 2, size, params->u);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 3, size, params->d_p);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 2, size, params->d_q);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 5, size, params->r_sqr_mod_p);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 4, size, params->p_prime);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 3, size, params->r_sqr_mod_q);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 6, size, params->q_prime);

    if ( params->enable_timing_attack_hardening ) {
        // enable "Timing Neutral Square-and-Multiply Algorithm"
        elppka_set_jump_probability_register(&ctxt->elppka_ctxt, PKA_JUMP_PROBABILITY_100_PERCENT);

        // enable "Time-Invariant Square-and-Multiply Algorithm"
        flags = elppka_set_f0_flag(flags);
    }
    else {
        elppka_set_jump_probability_register(&ctxt->elppka_ctxt, PKA_JUMP_PROBABILITY_0_PERCENT);
    }

    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_CRT, flags, size);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 0, size, result_c_lo);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 1, size, result_c_hi);
    return status;
}


int pka_crt_key_setup(
    pka_state_t * ctxt,
    const pka_crt_key_setup_params_t * params,
    const pka_crt_key_setup_results_t * results
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != results) &&
        (0 != results->result_d_p) &&
        (0 != results->result_d_q) &&
        (0 != results->result_u) &&
        (0 != params) &&
        (0 != params->p ) &&
        (0 != params->q ) &&
        (0 != params->d_lo ) &&
        (0 != params->d_hi ) &&
        (params->size > 0) &&
        (params->size <= (ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt) / 2));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->p);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 1, size, params->q);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 1, size, params->d_lo);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 3, size, params->d_hi);
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_CRT_KEY_SETUP, 0, size);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 1, size, results->result_d_p);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, results->result_d_q);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 3, size, results->result_u);
    return status;
}


int pka_crt_bit_serial_modred(
    pka_state_t * ctxt,
    const pka_crt_bit_serial_modred_params_t * params,
    uint8_t * result_c
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_c) &&
        (0 != params) &&
        (0 != params->x) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= (ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt) / 2));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, params->x);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_BIT_SERIAL_MOD, 0, size);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, result_c);
    return status;
}


int pka_crt_bit_serial_modred_double_precision(
    pka_state_t * ctxt,
    const pka_crt_bit_serial_modred_double_precision_params_t * params,
    uint8_t * result_c
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_c) &&
        (0 != params) &&
        (0 != params->x_lo) &&
        (0 != params->x_hi) &&
        (0 != params->modulus_m) &&
        (params->size > 0) &&
        (params->size <= (ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt) / 2));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, params->x_lo);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 1, size, params->x_hi);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->modulus_m);
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_BIT_SERIAL_MOD_DP, 0, size);
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_C, 0, size, result_c);
    return status;
}


int pka_set_common_curve_parameters(
    pka_state_t * ctxt,
    const pka_set_common_curve_parameters_params_t * params
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != params) &&
        (0 != params->a) &&
        (0 != params->p) &&
        (0 != params->p_prime) &&
        (0 != params->rr ) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid || !params) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    int status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 6, size, params->a);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->p);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 1, size, params->p_prime);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 3, size, params->rr);
    return status;
}


static inline uint32_t get_next_power_of_2(const uint32_t number)
{
    uint32_t next_power_of_2 = 1;
    while ( next_power_of_2 < number ) {
        next_power_of_2 = next_power_of_2 << 1;
    }
    return next_power_of_2;
}

int pka_pmult(
    pka_state_t * ctxt,
    const pka_pmult_params_t * params,
    uint8_t * result_q_x,
    uint8_t * result_q_y
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_q_x) &&
        (0 != result_q_y) &&
        (0 != params) &&
        (0 != params->p_x) &&
        (0 != params->p_y) &&
        (0 != params->key) &&
        (params->key_size > 2) &&
        (params->key_size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt)) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    uint32_t flags = 0;

    const uint32_t key_size = params->key_size;
    const uint32_t size = params->size;

    uint8_t * tmp_key = 0;
    const uint32_t extended_key_size = get_next_power_of_2(key_size);
    if ( extended_key_size != key_size) {
        tmp_key = (uint8_t *) malloc(extended_key_size);
        if ( 0 == tmp_key ) {
            return -3;
        }		
        memset(&tmp_key[key_size], 0, extended_key_size - key_size);
        memcpy(&tmp_key[0], params->key, key_size);		
    }

    const uint8_t * extended_key = (tmp_key) ? tmp_key : params->key;
    
    int status;
    
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, params->p_x);
    if ( 0 != status ) {
        goto pka_pmult_exit;
    }
    
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, params->p_y);
    if ( 0 != status ) {
        goto pka_pmult_exit;
    }
    
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 7, extended_key_size, extended_key);
    if ( 0 != status ) {
        goto pka_pmult_exit;
    }
    
    if ( params->enable_a_minus_3_optimization ) {
        flags = elppka_set_f3_flag(flags);
    }

    if ( params->enable_timing_attack_hardening ) {
        // enable "Timing Neutral Double-and-Add Algorithm"
        elppka_set_jump_probability_register(&ctxt->elppka_ctxt, PKA_JUMP_PROBABILITY_100_PERCENT);

        // enable "(Almost) Time-Invariant Double-and-Add Algorithm"
        uint32_t size_of_order_of_base_point_in_bits = params->size * 8;
        elppka_set_index_l_register(&ctxt->elppka_ctxt, size_of_order_of_base_point_in_bits);
    }
    else {
        elppka_set_jump_probability_register(&ctxt->elppka_ctxt, PKA_JUMP_PROBABILITY_0_PERCENT);
        elppka_set_index_l_register(&ctxt->elppka_ctxt, 0);
    }

    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_PMULT, flags, size);
    if ( 0 != status ) {
        goto pka_pmult_exit;
    }
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, result_q_x);
    if ( 0 != status ) {
        goto pka_pmult_exit;
    }
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, result_q_y);
    
pka_pmult_exit:
    if ( 0 != tmp_key ) {
        free(tmp_key);
        tmp_key = 0;
    }
    
    return status;
}


int pka_pmult_shamir(
    pka_state_t * ctxt,
    const pka_pmult_shamir_params_t * params,
    uint8_t * result_r_x,
    uint8_t * result_r_y
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_r_x) &&
        (0 != result_r_y) &&
        (0 != params) &&
        (0 != params->p_x) &&
        (0 != params->p_y) &&
        (0 != params->q_x) &&
        (0 != params->q_y) &&
        (0 != params->multiplier_for_p) &&
        (0 != params->multiplier_for_q) &&
        (params->key_size > 2) &&
        (params->key_size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt)) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid ) {
        return -1;
    }
    
    uint32_t flags = 0;

    const uint32_t key_size = params->key_size;
    const uint32_t size = params->size;

    uint8_t * tmp_multiplier_for_p = 0;
    uint8_t * tmp_multiplier_for_q = 0;
    const uint32_t extended_key_size = get_next_power_of_2(key_size);
    if ( extended_key_size != key_size) {
        tmp_multiplier_for_p = (uint8_t *) malloc(extended_key_size);
        if ( 0 == tmp_multiplier_for_p ) {
            return -2;
        }
        memset(&tmp_multiplier_for_p[key_size], 0, extended_key_size - key_size);
        memcpy(&tmp_multiplier_for_p[0], params->multiplier_for_p, key_size);
        tmp_multiplier_for_q = (uint8_t *) malloc(extended_key_size);
        if ( 0 == tmp_multiplier_for_q ) {
            if ( tmp_multiplier_for_p ) {
                free(tmp_multiplier_for_p);
                tmp_multiplier_for_p = 0;
            }
            return -3;
        }
        memset(&tmp_multiplier_for_q[key_size], 0, extended_key_size - key_size);
        memcpy(&tmp_multiplier_for_q[0], params->multiplier_for_q, key_size);
    }

    const uint8_t * extended_multiplier_for_p = (tmp_multiplier_for_p) ? tmp_multiplier_for_p : params->multiplier_for_p;
    const uint8_t * extended_multiplier_for_q = (tmp_multiplier_for_q) ? tmp_multiplier_for_q : params->multiplier_for_q;
    
    uint32_t status;

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, params->p_x);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, params->p_y);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 3, size, params->q_x);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 3, size, params->q_y);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 7, extended_key_size, extended_multiplier_for_p);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 7, extended_key_size, extended_multiplier_for_q);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }
    
    if ( params->enable_a_minus_3_optimization ) {
        flags = elppka_set_f3_flag(flags);
    }
    
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_SHAMIR, flags, size);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 3, size, result_r_x);
    if ( 0 != status ) {
        goto pka_pmult_shamir_exit;
    }

    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 3, size, result_r_y);
    
pka_pmult_shamir_exit:
    if ( tmp_multiplier_for_p ) {
        free(tmp_multiplier_for_p);
        tmp_multiplier_for_p = 0;
    }
    if ( tmp_multiplier_for_q ) {
        free(tmp_multiplier_for_q);
        tmp_multiplier_for_q = 0;
    }
    return status;
}


int pka_padd(
    pka_state_t * ctxt,
    const pka_padd_params_t * params,
    uint8_t * result_r_x,
    uint8_t * result_r_y
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_r_x) &&
        (0 != result_r_y) &&
        (0 != params) &&
        (0 != params->p_x) &&
        (0 != params->p_y) &&
        (0 != params->q_x) &&
        (0 != params->q_y) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    uint32_t status;
    
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, params->p_x);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, params->p_y);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 3, size, params->q_x);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 3, size, params->q_y);
    
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_PADD, 0, size);
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, result_r_x);
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, result_r_y);
    
    return status;
}


int pka_pdbl(
    pka_state_t * ctxt,
    const pka_pdbl_params_t * params,
    uint8_t * result_q_x,
    uint8_t * result_q_y
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_q_x) &&
        (0 != result_q_y) &&
        (0 != params) &&
        (0 != params->p_x) &&
        (0 != params->p_y) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    uint32_t status;
    
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 3, size, params->p_x);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 3, size, params->p_y);
    
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_PDBL, 0, size);
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, result_q_x);
    
    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, result_q_y);
    
    return status;
}


int pka_pver(
    pka_state_t * ctxt,
    const pka_pver_params_t * params,
    uint32_t * is_on_curve
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != is_on_curve) &&
        (0 != params) &&
        (0 != params->p_x) &&
        (0 != params->p_y) &&
        (0 != params->b) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    uint32_t status;
    
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, params->p_x);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_B, 2, size, params->p_y);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 7, size, params->b);
    
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_PVER, 0, size);
    
    *is_on_curve = elppka_is_zero_flag_set(&ctxt->elppka_ctxt);
    
    return status;
}


int pka_is_a_minus_3(
    pka_state_t * ctxt,
    const pka_is_a_minus_3_params_t * params,
    uint32_t * is_a_minus_3
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != is_a_minus_3) &&
        (0 != params) &&
        (0 != params->a) &&
        (0 != params->p) &&
        (params->size > 0) &&
        (params->size <= ECC_MAX_OPERAND_SIZE_IN_BYTES(ctxt));
    
    if ( !are_parameters_valid) {
        return -1;
    }
    
    const uint32_t size = params->size;
    
    uint32_t status;
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 6, size, params->a);
    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, size, params->p);
    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_IS_A_M3, 0, size);
    
    *is_a_minus_3 = elppka_is_zero_flag_set(&ctxt->elppka_ctxt);
    
    return status;
}

static const unsigned char m25519_big_endian[32] = {
    0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xed
    };

static const unsigned char m25519_little_endian[32] = {
    0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f
    };

int pka_load_c25519_curve_data(pka_state_t * ctxt)
{
    ctxt = GET_PKA_CTXT(ctxt);

    const unsigned int operand_size_in_bytes = 32;
    const unsigned char * m25519;
    unsigned char k[operand_size_in_bytes];

	memset(k, 0, operand_size_in_bytes);
    
    int status;

    if ( ctxt->elppka_ctxt.operands_are_in_big_endian_format )
    {
        // set k to 121666 (0x1db42 in hex)
        k[29] = 1;
        k[30] = 0xdb;
        k[31] = 0x42;
        m25519 = m25519_big_endian;
    }
    else
    {
        // set k to 121666 (0x1db42 in hex)
        k[0] = 0x42;
        k[1] = 0xdb;
        k[2] = 1;
        m25519 = m25519_little_endian;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 2, operand_size_in_bytes, k);
	if ( 0 != status ) {
        goto pka_load_c25519_curve_data_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 0, operand_size_in_bytes, m25519);

pka_load_c25519_curve_data_exit:
    return status;
}

int pka_c25519_pmult(
    pka_state_t * ctxt,
    pka_c25519_pmult_params_t * params,
    uint8_t * result_r_x
    )
{
    ctxt = GET_PKA_CTXT(ctxt);

    uint32_t are_parameters_valid = 1 &&
        (0 != ctxt) &&
        (0 != result_r_x) &&
        (0 != params->p_x) &&
        (0 != params->key) &&
        ( !params->enable_blinding || (params->enable_blinding && params->blinding_value));
    
    if ( !are_parameters_valid) {
        return -1;
    }
    
    int status;
    const uint32_t size = 256 / 8;

    uint32_t flags = 0;
    if ( params->enable_blinding ) {
        flags = elppka_set_f0_flag(flags);
        status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 7, size, params->blinding_value);
        if ( 0 != status ) {
            goto pka_c25519_pmult_exit;
        }
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, params->p_x);
    if ( 0 != status ) {
        goto pka_c25519_pmult_exit;
    }

    status = elppka_load_operand(&ctxt->elppka_ctxt, PKA_OPERAND_D, 7, size, params->key);
    if ( 0 != status ) {
        goto pka_c25519_pmult_exit;
    }

    status = elppka_perform_operation(&ctxt->elppka_ctxt, PKA_CMD_C25519_PMULT, flags, size);
    if ( 0 != status ) {
        goto pka_c25519_pmult_exit;
    }

    status = elppka_unload_operand(&ctxt->elppka_ctxt, PKA_OPERAND_A, 2, size, result_r_x);

pka_c25519_pmult_exit:
    return status;
}

int pka_precompute(
    pka_state_t * ctxt,
    uint8_t * result_r_inv,
    uint8_t * result_m_prime,
    uint8_t * result_r_sqr,
    const uint8_t * modulus_m,
    uint32_t size
    )
{
    int status = -1;
    status = pka_calc_r_inv(ctxt, modulus_m, size, result_r_inv);
    if ( 0 != status ) {
        goto pka_precompute_exit;
    }

    pka_calc_m_prime_params_t pka_calc_m_prime_params =
    {
        .r_inv = result_r_inv,
        .modulus_m = modulus_m,
        .size = size
    };
    status = pka_calc_m_prime(ctxt, &pka_calc_m_prime_params, result_m_prime);
    if ( 0 != status ) {
        goto pka_precompute_exit;
    }

    pka_calc_r_sqr_params_t  pka_calc_r_sqr_params =
    {
        .r_inv = result_r_inv,
        .modulus_m = modulus_m,
        .size = size
    };
    status = pka_calc_r_sqr(ctxt, &pka_calc_r_sqr_params, result_r_sqr);

pka_precompute_exit:
    return status;
}	

