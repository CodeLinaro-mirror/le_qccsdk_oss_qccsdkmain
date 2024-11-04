/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/******************************************************************************
 * @file fifo.h
 * @brief  A common Library declaration to handle static FIFO
 *
 *
 *****************************************************************************/

#ifndef _FIFO_H_
#define _FIFO_H_

#include <string.h>

#define FIFO_CONCAT_2(a, b) a##b
#define FIFO_CONCAT_3(a, b, c) a##b##c

#define FIFO_RESET          (0)
#define FIFO_SET            (1)

typedef const unsigned char cu_ch;

typedef enum fifo_err
{
    FIFO_SUCCESS,
    FIFO_FAILURE
} fifo_ret_t;

typedef struct fifo
{
    char * p_buffer;
    cu_ch  is_circular : FIFO_SET;
    size_t size;
    size_t element_size;
    size_t head;
    size_t tail;
    size_t watermark;
    size_t n_el_curr;
    size_t n_el_max;
    void   (*wm_cb)(struct fifo* p_fifo);
} fifo_t;

/******************************************************************************
 * @brief Define a Circular Buffer instance
 *
 * @param _NAME     Name of the Buffer and instance
 * @param _size     FIFO depth in bytes
 * @param _type     data type of FIFO
 * @param _wm       Watermark Byte
 * @param _wm_cb    Callback post Watermark
 *****************************************************************************/
#define FIFO_INIT_BUFFER_CIRCULAR(_NAME, _size, _type, _wm, _wm_cb) FIFO_INIT_BUFFER(_NAME, _size, _type, _wm, _wm_cb, FIFO_SET)

 /******************************************************************************
  * @brief Define a Linear Buffer instance
  *
  * @param _NAME     Name of the Buffer and instance
  * @param _size     FIFO depth in bytes
  * @param _type     data type of FIFO
  * @param _wm       Watermark Byte
  * @param _wm_cb    Callback post Watermark
  *
  *****************************************************************************/
#define FIFO_INIT_BUFFER_LINEAR(_NAME, _size, _type, _wm, _wm_cb) FIFO_INIT_BUFFER(_NAME, _size, _type, _wm, _wm_cb, FIFO_RESET)
  /******************************************************************************
   * @brief Get instance of FIFO by its _NAME
   *
   * @param _NAME     Name of the Buffer and instance
   *
   *****************************************************************************/

#define FIFO_INSTANCE(_NAME) FIFO_CONCAT_3(fifo_, _NAME, _inst)

#define FIFO_INIT_BUFFER(_NAME, _size, _type, _wm, _wm_cb, _is_circular)        \
    static char FIFO_CONCAT_2(_NAME, _fifo_buffer)[(_size * sizeof(_type)) + FIFO_SET]; \
    static fifo_t FIFO_CONCAT_3(fifo_, _NAME, _inst) =                           \
        {                                                                        \
            .is_circular = _is_circular,                                         \
            .p_buffer = FIFO_CONCAT_2(_NAME, _fifo_buffer),                      \
            .n_el_max = _size,                                                   \
            .size = (_size * sizeof(_type)),                                     \
            .element_size = sizeof(_type),                                       \
            .head = -FIFO_SET,                                                   \
            .tail = -FIFO_SET,                                                   \
            .n_el_curr = FIFO_RESET,                                             \
            .watermark = _wm,                                                    \
            .wm_cb = _wm_cb};


fifo_ret_t fifo_enqueue(fifo_t *p_fifo, char *p_element);
fifo_ret_t fifo_dequeue(fifo_t *p_fifo, char *p_element);
fifo_ret_t fifo_peak(fifo_t *p_fifo, char *p_element);
fifo_ret_t fifo_trav_renterant(fifo_t* p_fifo, size_t *p_next_el, char* p_element);
fifo_ret_t fifo_set_wm(fifo_t* p_fifo, size_t wm);
fifo_ret_t fifo_get_wm(fifo_t* p_fifo, size_t* wm);

#endif /* _FIFO_H_ */
