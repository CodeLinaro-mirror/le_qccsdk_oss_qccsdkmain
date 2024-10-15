/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/******************************************************************
 * @file sort.c
 * @brief  Sort library definition
 *
 *
 *****************************************************************/
#include "sort.h"

#include "safeAPI.h"

/************************************************************************************
 * Static Declaration
 *************************************************************************************/

static void swap(char* x, char* y, char* dummy_buff, size_t element_size);

/************************************************************************************
 * @brief  Bubble Sort
 *
 * @param p_sort    Pointer to Sort instance
 * @param dir       Assending / Decending
 ************************************************************************************/
void sort_bubble(sort_t* p_sort, sort_direction_t dir)
{
    size_t i, j;
    for (i = 0; i < (p_sort->size_buffer - p_sort->buffer_node_size); i += p_sort->buffer_node_size)
    {
        for (j = 0; j < (p_sort->size_buffer - p_sort->buffer_node_size - i); j += p_sort->buffer_node_size)
            if (dir ^ (memcmp((p_sort->p_buffer + j + p_sort->sort_param_offset),
                       (p_sort->p_buffer + j + p_sort->sort_param_offset + p_sort->buffer_node_size),
                       (p_sort->sort_param_size == 0 ? p_sort->buffer_node_size : p_sort->sort_param_size)) > 0))
            {
                swap(p_sort->p_buffer + j,
                     p_sort->p_buffer + j + p_sort->buffer_node_size,
                     p_sort->p_dummy_buff, p_sort->buffer_node_size);
            }
    }

    return;
}

/*******************************************************************
 * @brief Swap 2 memory sections
 *
 * @param x             Memory pointer 1
 * @param y             Memory pointer 2
 * @param dummy_buff    Pointer to dummy buffer used as temp
 * @param element_size  Size of one element
 ******************************************************************/
static void swap(char* x, char* y, char* dummy_buff, size_t element_size)
{
    memscpy(dummy_buff, element_size, x, element_size);
    memscpy(x, element_size, y, element_size);
    memscpy(y, element_size, dummy_buff, element_size);
}
