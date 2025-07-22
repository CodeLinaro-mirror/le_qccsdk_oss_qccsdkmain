/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file vlint.h
 * @brief vlint header file
 *========================================================================*/

#ifndef VLINT_H
#define VLINT_H

/*-------------------------------------------------------------------------
 * Include files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef FERMION_CONFIG_HCF
#include <stdint.h>
#include <stdbool.h>

/*-------------------------------------------------------------------------
 *Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define MAXINT8VLINTLEN   (1U + 1U)
#define MAXUINT8VLINTLEN  (1U + 1U + 1U)
#define MAXINT16VLINTLEN  (1U + 2U)
#define MAXUINT16VLINTLEN (1U + 2U + 1U)
#define MAXINT32VLINTLEN  (1U + 4U)
#define MAXUINT32VLINTLEN (1U + 4U + 1U)
#define MAXINT48VLINTLEN  (1U + 6U)
#define MAXUINT48VLINTLEN (1U + 6U + 1U)
#define MAXINT64VLINTLEN  (1U + 8U)
#define MAXUINT64VLINTLEN (1U + 8U + 1U)

#define MAXVLINTLEN (MAXUINT64VLINTLEN)

/** Minimum VLINT length. */
#define MINVLINTLEN (1U)

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/** A VLINT is really an array of uint8s. */

typedef uint8_t VLINT;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* Maximum VLINT lengths for given data types == (1 + number_of_octets).
Need extra octet for unsigned numbers. */

/**
 * number of octets in a vlint
 *
 * \returns
 *    The number of octets forming "vl".
 *
 * If the length is found to be absurd (more than MAXVLINTLEN) then the
 * function raises a fault_diatribe().
 */
extern uint16_t vlint_length(const VLINT *vl);

/**
 * number of data octets in a vlint
 *
 *  The vlint_length() function reports the total number of octets forming a
 *  VLINT.  This includes its data structure overhead.  By contrast, this
 *  function reports the number of octets that would be required to hold the
 *  data value stored within a VLINT.
 *
 *  EXAMPLE
 *   64 is stored in a VLINT as {0x81, 0x64}.  vlint_length() would report
 *   "2" for this VLINT, but vlint_data_length() would report "1".
 *
 *  \returns
 *    The number of octets that would be required to hold the data value stored
 *   within "vl".
 *
 *   If the length is found to be absurd (more than (MAXVLINTLEN - 1)) then
 *   the function raises a fault_diatribe().
 */
extern uint16_t vlint_data_length(const VLINT *vl);

/**
 * copy variable length integers
 *
 * Copies the vlint in "src" to "dst".  The function presumes "dst" has
 * sufficient storage to hold "src".
 *
 * \returns
 *   The length of "src" - identical to \c vlint_length(src).
 */
// extern uint16 vlint_copy(VLINT *dst, const VLINT *src);

/**
 * compare variable length integers
 *
 *  Compares the values of "a" and "b".
 *
 * \returns
 *    -1 if a < b
 *   0  if a == b
 *   1  if a > b
 */
// extern int16 vlint_cmp(const VLINT *a, const VLINT *b);

/**
 * determine the sign of a vlint
 *
 * \returns
 *    TRUE if "a" is less than zero, else FALSE.
 */
extern bool vlint_negative(const VLINT *a);

/**
 * has a vlint the value of zero
 *
 * \returns
 *   TRUE if "a" holds the value zero, else FALSE.
 */
// extern bool vlint_is_zero(const VLINT *a);

/**
 * set a vlint to zero
 *
 *  Sets the value of "a" to zero.
 */
// extern void vlint_clear(VLINT *a);

/**
 * Write an int16 into a vlint
 *
 * Writes the value \p i into (the octet array at) \p vl.
 *
 * The length of the octet array should normally be large enough to take
 * the largest possible value of \p i: MAXINT16VLINTLEN, MAXINT32VLINTLEN,
 * etc.  The array may be shorter if it is known that the value of \p i
 * will fit.
 *
 * The function has no knowledge of the octet array's length - it is
 * presumed to be big enough to hold \p i, and any locations not needed to
 * hold \p i are left untouched.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_int16(VLINT *vl, int16 i);

/**
 * Write a  uint16 into a vlint
 *
 * See description of \c vlint_from_int16.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_uint16(VLINT *vl, uint16 i);

/**
 * Write an int32 into a vlint
 *
 * See description of \c vlint_from_int16.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_int32(VLINT *vl, int32 i);

/**
 * Write a uint32 into a vlint
 *
 * See description of \c vlint_from_int16.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_uint32(VLINT *vl, uint32 i);

/**
 * Write an int64 into a vlint
 *
 * See description of \c vlint_from_int16.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_int64(VLINT *vl, int64 i);

/**
 * Write a uint64 into a vlint
 *
 * See description of \c vlint_from_int16.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_uint64(VLINT *vl, uint64 i);

/**
 * Write a data address into a vlint
 *
 * See description of \c vlint_from_int16.
 *
 * \returns
 *    The number of octets in \p vl used to hold \p i.
 */
// extern uint16 vlint_from_daddr(VLINT *vl, unsigned *d);

/**
 *  read an int16 from a vlint
 *
 *  \returns The value coded in the (octet array at) "vl".
 */
extern int16_t vlint_to_int16(const VLINT *vl);

/**
 * read a uint16 from a vlint
 *
 * \returns The value coded in the (octet array at) "vl".
 */
extern uint16_t vlint_to_uint16(const VLINT *vl);

/**
 * read an int32 from a vlint
 *
 * \returns The value coded in the (octet array at) "vl".
 */
extern int32_t vlint_to_int32(const VLINT *vl);

/**
 * read a uint32 from a vlint
 *
 * \returns The value coded in the (octet array at) "vl".
 */
extern uint32_t vlint_to_uint32(const VLINT *vl);

/**
 *  read an int64 from a vlint
 *
 * \returns The value coded in the (octet array at) "vl".
 */

// extern int64 vlint_to_int64(const VLINT *vl);

/**
 * read a uint64 from a vlint
 *
 * \returns The value coded in the (octet array at) "vl".
 */
// extern uint64 vlint_to_uint64(const VLINT *vl);

/**
 * read a data address from a vlint
 *
 * \returns The value coded in the (octet array at) "vl".
 */
// extern unsigned *vlint_to_daddr(const VLINT *vl);

/**
 * write an octet array into a vlint
 *
 * Writes to "vl" the integer value held in the "oalen" array of octets
 *  at "oa".
 *
 *  The array at "oa" holds an arbitrary length (signed) integer, split into
 *  octets, with the most significant value bits at the lowest address.
 *
 *  The octet array at "vl" is presumed to be large enough to hold the
 *  value.
 *
 * \returns The number of elements in "vl" used to hold the value.
 */
extern uint16_t vlint_from_octet_array(VLINT *vl, const uint8_t *oa, uint16_t oalen);

/**
 * Write an octet_array into vlint as a vldata octet string
 *
 * \param vl The storage for the vlint result
 * \param vl_max_len The number of locations of the storage at vl. If
 *        this is too small for the result then this function will raise
 *        a fault FAULT_HYDRA_VLINT_OVERFLOW and return zero.
 * \param oa The octet array to be converted
 * \param oalen The number of elements in the octet array
 *
 * \returns The number of elements in "vl" used to hold the value.
 */
/*extern uint16 vlint_string_from_octet_array(VLINT *vl,
                                            uint16 vl_max_len,
                                            const uint8 *oa,
                                            uint16 oalen);
*/
/**
 * read an octet array from a vlint
 *
 * Writes to "oa" the integer value held in "vl" up to a maximum
 * of "oa_size" octets.
 *
 * The (signed) integer returned in \p oa has its most significant octet at
 * its lowest address.
 *
 *
 * \returns The number of elements in "oa" used to hold the value.
 */
extern uint16_t vlint_to_octet_array(const VLINT *vl, uint8_t *oa, uint16_t oa_size);

/**
 * sign extend an integer in an octet array
 *
 *  Takes the signed integer held in "buf", ordered with its most significant
 *  octet at "buf", and shifts it "shift" locations, filling the vacated
 *  locations to extend the integer's sign.
 *
 *  The buffer at "buf" is presumed to be at least "noctets" plus "shift"
 *  elements long.
 *
 *  This function is intended to support \c vlint_from_octet_array() and
 *  \c vlint_to_octet_array().  For example, the value -1, held in a \c uint8[8],
 *  could be converted to a \c VLINT with \c vlint_from_octet_array().  When the
 *  value was recovered with \c vlint_to_octet_array() it would be returned
 *  in a \c uint8[1].  \c vlint_shift_octet_array() allows the missing 7 octets
 *  to be wasted again.
 *
 *  EXAMPLE
 *   If the function is called with "buf" holding {0x80, 0x11, 0x22} as:
 *
 *       \c vlint_shift_octet_array(buf, 3, 2);
 *
 *   then \c buf is returned as {0xff, 0xff, 0x80, 0x11, 0x22}.
 */
// void vlint_shift_octet_array(uint8 *buf, uint16 noctets, uint16 shift);

/**
 * read a vlint from a bmsg
 *
 * Read the sequence of octets forming a VLINT from \c bmsg and copy them
 * into \c val.
 *
 * The storage at \c val is presumed to be large enough to hold the VLINT.
 *
 * \returns TRUE if a valid VLINT was copied from \c bmsg to \c val, else FALSE.
 */
// extern bool vlint_read_from_bmsg(VLINT *val, BMSG *bmsg);

#endif  // FERMION_CONFIG_HCF
#endif  /* VLINT_H */
