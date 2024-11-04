/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/******************************************************************
 * @file fifo.c
 * @brief  FIFO library definition
 *
 *
 *****************************************************************/
#include "fifo.h"

#include "safeAPI.h"

#define FIFO_ASSERT_IF_TRUE(cond)\
    {                            \
        if (!(cond))             \
            ;                    \
        else                     \
            return FIFO_FAILURE; \
    }

/******************************************************************
 * @brief Enqueue Element to the Queue back
 *
 * @param p_fifo            Pointer to FIFO instance
 * @param p_element         Pointer to element to be enqued
 * @return fifo_ret_t       FIFO_SUCCESS or FIFO_FAILURE
 *****************************************************************/
fifo_ret_t fifo_enqueue(fifo_t* p_fifo, char* p_element)
{

    if ((p_fifo->tail + p_fifo->element_size) % p_fifo->size == p_fifo->head) // condition to check queue is full
    {
        FIFO_ASSERT_IF_TRUE(!p_fifo->is_circular);

        p_fifo->tail += p_fifo->element_size;
        p_fifo->tail %= p_fifo->size;
        p_fifo->head += p_fifo->element_size;
        p_fifo->head %= p_fifo->size;
    }
    else
    {
        if (p_fifo->tail == (size_t)-FIFO_SET) // First Element
        {
            p_fifo->tail = p_fifo->head = FIFO_RESET;
        }
        else if (p_fifo->tail == p_fifo->size - p_fifo->element_size && p_fifo->head != FIFO_RESET)
        {
            p_fifo->tail = FIFO_RESET;
        }
        else
        {
            p_fifo->tail += p_fifo->element_size;
        }
    }
    memscpy(p_fifo->p_buffer + p_fifo->tail, p_fifo->element_size, p_element, p_fifo->element_size);
    p_fifo->n_el_curr++;

    p_fifo->n_el_curr = p_fifo->n_el_curr > p_fifo->n_el_max ? p_fifo->n_el_max : p_fifo->n_el_curr;

    if (p_fifo->watermark <= p_fifo->n_el_curr)
    {
        p_fifo->wm_cb != NULL ? p_fifo->wm_cb(p_fifo) : NULL;
    }

    return FIFO_SUCCESS;
}
/******************************************************************
 * @brief Dequeue Element from Queue Front
 *
 * @param p_fifo            Pointer to FIFO instance
 * @param p_element         Pointer to fetch dequed element
 * @return fifo_ret_t       FIFO_SUCCESS or FIFO_FAILURE
 *****************************************************************/
fifo_ret_t fifo_dequeue(fifo_t *p_fifo, char *p_element)
{
    FIFO_ASSERT_IF_TRUE(p_fifo->head == (size_t)-FIFO_SET);
    memscpy(p_element, p_fifo->element_size, p_fifo->p_buffer + p_fifo->head, p_fifo->element_size);
    p_fifo->n_el_curr--;
    if (p_fifo->head == p_fifo->tail)
    {
        p_fifo->head = p_fifo->tail = (size_t)-FIFO_SET;
        memset(p_fifo->p_buffer, FIFO_RESET, p_fifo->size);
    }
    else
    {
        p_fifo->head = p_fifo->head == p_fifo->size - p_fifo->element_size ? FIFO_RESET : p_fifo->head + p_fifo->element_size;
    }

    return FIFO_SUCCESS;
}
/******************************************************************
 * @brief Queue Traversal a re-enterant function,
 * that can be called multiple times to get the next element
 * without de-queuing
 *
 * @param p_fifo            Pointer to FIFO instance
 * @param p_next_el         Returened by the function call, to be used as a seed for successive calls. First call, keep this as FIFO_RESET
 * @param p_element         Pointer to element to to peak
 * @return fifo_ret_t       FIFO_SUCCESS or FIFO_FAILURE
 *****************************************************************/
fifo_ret_t fifo_trav_renterant(fifo_t* p_fifo, size_t* p_next_el, char* p_element)
{
    FIFO_ASSERT_IF_TRUE(p_fifo->head == (size_t)-FIFO_SET);

    if (*p_next_el != (FIFO_RESET))
    {
        if (*p_next_el % p_fifo->element_size == FIFO_RESET)  //Check alignment
        {
            memscpy(p_element, p_fifo->element_size, p_fifo->p_buffer + *p_next_el, p_fifo->element_size);

            if (*p_next_el == p_fifo->tail)
            {
                *p_next_el = FIFO_RESET;
            }
            else {
                *p_next_el += p_fifo->element_size;
            }
        }
        else
        {
            return FIFO_FAILURE;
        }
    }
    else
    {
        memscpy(p_element, p_fifo->element_size, p_fifo->p_buffer + p_fifo->head, p_fifo->element_size);
        *p_next_el = p_fifo->head + p_fifo->element_size;
    }
    return FIFO_SUCCESS;
}
/******************************************************************
 * @brief Peak Queue Next Available element
 *
 * @param p_fifo            Pointer to FIFO instance
 * @param p_element         Pointer to element to to peak
 * @return fifo_ret_t       FIFO_SUCCESS or FIFO_FAILURE
 *****************************************************************/
fifo_ret_t fifo_peak(fifo_t* p_fifo, char* p_element)
{
    FIFO_ASSERT_IF_TRUE(p_fifo->head == (size_t)-FIFO_SET);
    memscpy(p_element, p_fifo->element_size, p_fifo->p_buffer + p_fifo->head, p_fifo->element_size);
    return FIFO_SUCCESS;
}
/******************************************************************
 * @brief Change watermark value on run time
 *
 * @param p_fifo            Pointer to FIFO instance
 * @param wm                Watermark value
 * @return fifo_ret_t
 *****************************************************************/
fifo_ret_t fifo_set_wm(fifo_t* p_fifo, size_t wm)
{
    p_fifo->watermark = wm;
    return FIFO_SUCCESS;
}
/******************************************************************
 * @brief Change watermark value on run time
 *
 * @param p_fifo            Pointer to FIFO instance
 * @param wm                Watermark value return
 * @return fifo_ret_t
 *****************************************************************/
fifo_ret_t fifo_get_wm(fifo_t* p_fifo, size_t* wm)
{
    *wm = p_fifo->watermark;
    return FIFO_SUCCESS;
}
