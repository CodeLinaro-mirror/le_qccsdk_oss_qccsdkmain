/*
*/

#ifndef __CHOP_INTERNAL_H__
#define __CHOP_INTERNAL_H__

typedef struct co_struct {
	devh_t                *dev;  /* overloaded to dev on which the current
                                 * CHOP is done */
    channel_t             *current_channel;
} CO_STRUCT;

#endif /* __CHOP_INTERNAL_H__ */
