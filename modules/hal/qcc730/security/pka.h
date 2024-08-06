/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __PKA_H__
#define __PKA_H__

#include <stdint.h>

typedef struct pka_state_s pka_state_t;
extern pka_state_t g_pka_ctxt;
typedef struct pka_reduce_params_s pka_reduce_params_t;

/*
 * The size of the array of zeros should be the maximum of:
 * - ECC supported curve size (512 bits = 64 bytes)
 */

extern const unsigned char array_of_zeros[64];


typedef enum {
    PKA_OPERAND_ENDIANNESS_BIG_ENDIAN = 1,
    PKA_OPERAND_ENDIANNESS_LITTLE_ENDIAN = 2,
    PKA_OPERAND_ENDIANNESS_NETWORK_ORDER = PKA_OPERAND_ENDIANNESS_BIG_ENDIAN,
} pka_operand_endianness_t;


int pka_init(pka_state_t * ctxt);
int pka_deinit(pka_state_t * ctxt);
int pka_lock(pka_state_t * ctxt, pka_operand_endianness_t pka_operand_endianness);
int pka_unlock(pka_state_t * ctxt);
int pka_set_endianess(pka_state_t * ctxt, pka_operand_endianness_t pka_operand_endianness);
int pka_modred(
    pka_state_t * ctxt,
    const pka_reduce_params_t * params,
    uint8_t * result
    );

typedef struct pka_calc_m_prime_params_s {
    const uint8_t * r_inv;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_calc_m_prime_params_t;


typedef struct pka_calc_r_sqr_params_s {
    const uint8_t * r_inv;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_calc_r_sqr_params_t;


typedef struct pka_modmult_params_s {
    const uint8_t * input_x;
    const uint8_t * input_y;
    const uint8_t * modulus_m;
    const uint8_t * m_prime;
    const uint8_t * r_sqr;
    const uint32_t size;
} pka_modmult_params_t;


typedef struct pka_modadd_params_s {
    const uint8_t * input_x;
    const uint8_t * input_y;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_modadd_params_t;


typedef struct pka_modsub_params_s {
    const uint8_t * input_x;
    const uint8_t * input_y;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_modsub_params_t;


typedef struct pka_reduce_params_s {
    const uint8_t * input;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_reduce_params_t;


typedef struct pka_moddiv_params_s {
    const uint8_t * input_x;
    const uint8_t * input_y;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_moddiv_params_t;


typedef struct pka_modinv_params_s {
    const uint8_t * input;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_modinv_params_t;


typedef struct pka_modexp_params_s {
    const uint8_t * base;
    const uint8_t * exponent;
    const uint8_t * modulus_m;
    const uint8_t * m_prime;
    const uint8_t * r_sqr;
    const uint32_t size;
    const uint8_t enable_timing_attack_hardening;
} pka_modexp_params_t;


typedef struct pka_crt_modexp_params_s {
    const uint8_t * msg_lo;
    const uint8_t * msg_hi;
    const uint8_t * p;
    const uint8_t * q;
    const uint8_t * u;
    const uint8_t * d_p;
    const uint8_t * d_q;
    const uint8_t * r_sqr_mod_p;
    const uint8_t * p_prime;
    const uint8_t * r_sqr_mod_q;
    const uint8_t * q_prime;
    const uint32_t size;
    const uint8_t enable_timing_attack_hardening;
} pka_crt_modexp_params_t;


typedef struct pka_crt_key_setup_params_s {
    const uint8_t * p;
    const uint8_t *q;
    const uint8_t *d_lo;
    const uint8_t *d_hi;
    const uint32_t size;
} pka_crt_key_setup_params_t;


typedef struct pka_crt_key_setup_results_s {
    uint8_t * result_d_p;
    uint8_t * result_d_q;
    uint8_t * result_u;
} pka_crt_key_setup_results_t;


typedef struct pka_crt_bit_serial_modred_params_s {
    const uint8_t * x;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_crt_bit_serial_modred_params_t;


typedef struct pka_crt_bit_serial_modred_double_precision_params_s {
    const uint8_t * x_lo;
    const uint8_t * x_hi;
    const uint8_t * modulus_m;
    const uint32_t size;
} pka_crt_bit_serial_modred_double_precision_params_t;


typedef struct pka_set_common_curve_parameters_params_s {
    const uint8_t * a;
    const uint8_t * p;
    const uint8_t * p_prime;
    const uint8_t * rr;
    const uint32_t size;
} pka_set_common_curve_parameters_params_t;


typedef struct pka_pmult_params_s {
    const uint8_t * p_x;
    const uint8_t * p_y;
    const uint8_t * key;
    const uint32_t enable_a_minus_3_optimization;
    const uint32_t key_size;
    const uint32_t size;
    const uint8_t enable_timing_attack_hardening;
} pka_pmult_params_t;


typedef struct pka_pmult_shamir_params_s {
    const uint8_t * p_x;
    const uint8_t * p_y;
    const uint8_t * q_x;
    const uint8_t * q_y;
    const uint8_t * multiplier_for_p;
    const uint8_t * multiplier_for_q;
    const uint32_t enable_a_minus_3_optimization;
    const uint32_t key_size;
    const uint32_t size;
} pka_pmult_shamir_params_t;


typedef struct pka_padd_params_s {
    const uint8_t * p_x;
    const uint8_t * p_y;
    const uint8_t * q_x;
    const uint8_t * q_y;
    const uint32_t size;
} pka_padd_params_t;


typedef struct pka_pdbl_params_s {
    const uint8_t * p_x;
    const uint8_t * p_y;
    const uint32_t size;
} pka_pdbl_params_t;


typedef struct pka_pver_params_s {
    const uint8_t * p_x;
    const uint8_t * p_y;
    const uint8_t * b;
    const uint32_t size;
} pka_pver_params_t;


typedef struct pka_is_a_minus_3_params_s {
    const uint8_t * a;
    const uint8_t * p;
    const uint32_t size;
} pka_is_a_minus_3_params_t;


typedef struct pka_c25519_pmult_params_s {
    const uint8_t * p_x;
    const uint8_t * key;
    const uint32_t enable_blinding;
    const uint8_t * blinding_value;
} pka_c25519_pmult_params_t;

pka_state_t * GET_PKA_CTXT(void * ctxt);


int pka_calc_r_inv(
    pka_state_t * ctxt,
    const uint8_t * modulus_m,
    uint32_t size,
    uint8_t * result
    );


int pka_calc_m_prime(
    pka_state_t * ctxt,
    const pka_calc_m_prime_params_t * params,
    uint8_t * result
    );


int pka_calc_r_sqr(
    pka_state_t * ctxt,
    const pka_calc_r_sqr_params_t * params,
    uint8_t * result
    );


int pka_modmult(
    pka_state_t * ctxt,
    const pka_modmult_params_t * params,
    uint8_t * result
    );


int pka_modadd(
    pka_state_t * ctxt,
    const pka_modadd_params_t * params,
    uint8_t * result
    );


int pka_modsub(
    pka_state_t * ctxt,
    const pka_modsub_params_t * params,
    uint8_t * result
    );


int pka_reduce(
    pka_state_t * ctxt,
    const pka_reduce_params_t * params,
    uint8_t * result
    );


int pka_moddiv(
    pka_state_t * ctxt,
    const pka_moddiv_params_t * params,
    uint8_t * result
    );


int pka_modinv(
    pka_state_t * ctxt,
    const pka_modinv_params_t * params,
    uint8_t * result
    );


int pka_modexp(
    pka_state_t * ctxt,
    const pka_modexp_params_t * params,
    uint8_t * result
    );


int pka_crt_modexp(
    pka_state_t * ctxt,
    const  pka_crt_modexp_params_t * params,
    uint8_t * result_c_lo,
    uint8_t * result_c_hi
    );


int pka_crt_key_setup(
    pka_state_t * ctxt,
    const pka_crt_key_setup_params_t * params,
    const pka_crt_key_setup_results_t * results
    );


int pka_crt_bit_serial_modred(
    pka_state_t * ctxt,
    const pka_crt_bit_serial_modred_params_t * params,
    uint8_t * result_c
    );


int pka_crt_bit_serial_modred_double_precision(
    pka_state_t * ctxt,
    const pka_crt_bit_serial_modred_double_precision_params_t * params,
    uint8_t * result_c
    );


int pka_set_common_curve_parameters(
    pka_state_t * ctxt,
    const pka_set_common_curve_parameters_params_t * params
    );


int pka_pmult(
    pka_state_t * ctxt,
    const pka_pmult_params_t * params,
    uint8_t * result_q_x,
    uint8_t * result_q_y
    );


int pka_pmult_shamir(
    pka_state_t * ctxt,
    const pka_pmult_shamir_params_t * params,
    uint8_t * result_r_x,
    uint8_t * result_r_y
    );


int pka_padd(
    pka_state_t * ctxt,
    const pka_padd_params_t * params,
    uint8_t * result_r_x,
    uint8_t * result_r_y
    );


int pka_pdbl(
    pka_state_t * ctxt,
    const pka_pdbl_params_t * params,
    uint8_t * result_q_x,
    uint8_t * result_q_y
    );


int pka_pver(
    pka_state_t * ctxt,
    const pka_pver_params_t * params,
    uint32_t * is_on_curve
    );


int pka_is_a_minus_3(
    pka_state_t * ctxt,
    const pka_is_a_minus_3_params_t * params,
    uint32_t * is_a_minus_3
    );


int pka_load_c25519_curve_data(pka_state_t * ctxt);


int pka_c25519_pmult(
    pka_state_t * ctxt,
    pka_c25519_pmult_params_t * params,
    uint8_t * result_r_x
    );

#endif // __PKA_H__

