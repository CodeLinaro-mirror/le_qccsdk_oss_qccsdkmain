/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CHOP_INTERNAL_H__
#define __CHOP_INTERNAL_H__

typedef struct co_struct {
    devh_t *dev; /* overloaded to dev on which the current
                  * CHOP is done */
    channel_t *current_channel;
} CO_STRUCT;

#endif /* __CHOP_INTERNAL_H__ */
