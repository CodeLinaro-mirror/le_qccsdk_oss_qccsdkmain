/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __BOOT_ELF_HEADER_H__
#define __BOOT_ELF_HEADER_H__
/*===========================================================================

                ELF Header Definitions

DESCRIPTION
  This header file gives the definition of the structures in ELF header.

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

    This section contains comments describing changes made to this file.
    Notice that changes are listed in reverse chronological order.



when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/04/23   Bingzhe Initial revision
===========================================================================*/


/*
 *
 *                        ELF Header Definitions
 *                       ------------------------
 *
 * This section contains definitions necessary to parse an ELF header and
 * find ELF Header and Segment Header and extract length and offsets.
 *
 */

#include <stdint.h>

/* The first 52 bytes of the file is the ELF header  */

#define ELFINFO_MAGIC_SIZE (16)

typedef struct
{
  unsigned char e_ident[ELFINFO_MAGIC_SIZE]; /* Magic number and other info   */
  uint16_t        e_type;                /* Object file type                    */
  uint16_t        e_machine;             /* Architecture                        */
  uint32_t        e_version;             /* Object file version                 */
  uint32_t        e_entry;               /* Entry point virtual address         */
  uint32_t        e_phoff;               /* Program header table file offset    */
  uint32_t        e_shoff;               /* Section header table file offset    */
  uint32_t        e_flags;               /* Processor-specific flags            */
  uint16_t        e_ehsize;              /* ELF header size in bytes            */
  uint16_t        e_phentsize;           /* Program header table entry size     */
  uint16_t        e_phnum;               /* Program header table entry count    */
  uint16_t        e_shentsize;           /* Section header table entry size     */
  uint16_t        e_shnum;               /* Section header table entry count    */
  uint16_t        e_shstrndx;            /* Section header string table index   */
} boot_elf32_ehdr;

/* Fields in the e_ident array.  The ELFINFO_*_INDEX macros are
 * indices into the array.  The macros under each ELFINFO_* macro
 * is the values the byte may have.
 */

#define ELFINFO_MAG0_INDEX    0     /* File identification byte 0 index */
#define ELFINFO_MAG0          0x7f  /* Magic number byte 0              */

#define ELFINFO_MAG1_INDEX    1     /* File identification byte 1 index */
#define ELFINFO_MAG1         'E'    /* Magic number byte 1              */

#define ELFINFO_MAG2_INDEX    2     /* File identification byte 2 index */
#define ELFINFO_MAG2         'L'    /* Magic number byte 2              */

#define ELFINFO_MAG3_INDEX    3     /* File identification byte 3 index */
#define ELFINFO_MAG3         'F'    /* Magic number byte 3              */

#define ELFINFO_CLASS_INDEX   4     /* File class byte index            */

/* ELF Object Type */
#define BOOT_ELF_CLASS_32          1     /* 32-bit objects                   */


/* Version information */
#define ELFINFO_VERSION_INDEX 6    /* File version byte index          */
#define ELF_VERSION_CURRENT   1    /* Current version                  */

typedef enum
{
  PT_NULL = 0,
  PT_LOAD,
  PT_DYNAMIC,
  PT_INTERP,
  PT_NOTE,
  PT_SHLIB,
  PT_PHDR
} boot_elf_segment_type;


/* Program segment header.  */
typedef struct
{
  uint32_t p_type;                   /* Segment type */
  uint32_t p_offset;                 /* Segment file offset */
  uint32_t p_vaddr;                  /* Segment virtual address */
  uint32_t p_paddr;                  /* Segment physical address */
  uint32_t p_filesz;                 /* Segment size in file */
  uint32_t p_memsz;                  /* Segment size in memory */
  uint32_t p_flags;                  /* Segment flags */
  uint32_t p_align;                  /* Segment alignment */
} boot_elf32_phdr;


/* Segment flag bit definition */
typedef enum
{
  PF_X = 1,
  PF_W = 2,
  PF_R = 4
} boot_elf_segment_flags;


/* Section header.  */
typedef struct
{
  uint32_t	sh_name;		/* Section name */
  uint32_t	sh_type;		/* Section type */
  uint32_t	sh_flags;		/* Section flags */
  uint32_t	sh_addr;		/* Section addr */
  uint32_t	sh_offset;		/* Section offset */
  uint32_t	sh_size;		/* Section size */
  uint32_t	sh_link;		/* Section link */
  uint32_t	sh_info;		/* Section info */
  uint32_t	sh_addralign;	/* Section align */
  uint32_t	sh_entsize;	    /* Section ent size */
} boot_elf32_shdr;

#endif /* __BOOT_ELF_HEADER_H__ */


/* The latest ELF documentation is at http://www.caldera.com/developers/gabi/latest/ch5.pheader.html
 * and it talks about OS and PROCESSOR specific fields.
 * we use the bits (PF_MASKOS) in Elf32_PHdr.p_flags as recomended by ARM
 */

/* Definition for segment flags used in p_flag of program_segmen header
 *
 *                 Pool Indx    Segment type    Access type   Page/non page
 * bits in p_fag/-----27-----/----26-24-------/---- 23-21----/------20-------/
 */
/* Note: MI_PBT_MAX_SEGMENTS would impact OSBL memory usage */
#ifndef MI_PBT_MAX_SEGMENTS
#define MI_PBT_MAX_SEGMENTS 40
#endif

#define MAX_HASH_SEGMENT_SIZE (16*1024)
//MI_PBT_MAX_SEGMENTS * CRYPTO_DIGEST_BYTE_SIZE_SHA256 +sizeof(MBNHeader) + 2k * SECBOOT_TOTAL_MAX_CERTS

#define MI_PBT_NON_PAGED_SEGMENT   0x0
#define MI_PBT_PAGED_SEGMENT       0x1

#define MI_PBT_RW_SEGMENT          0x0
#define MI_PBT_RO_SEGMENT          0x1
#define MI_PBT_ZI_SEGMENT          0x2
#define MI_PBT_NOTUSED_SEGMENT     0x3
#define MI_PBT_SHARED_SEGMENT      0x4

#define MI_PBT_L4_SEGMENT          0x0
#define MI_PBT_AMSS_SEGMENT        0x1
#define MI_PBT_HASH_SEGMENT        0x2
#define MI_PBT_BOOT_SEGMENT        0x3
#define MI_PBT_L4BSP_SEGMENT       0x4
#define MI_PBT_SWAPPED_SEGMENT     0x5
#define MI_PBT_SWAP_POOL_SEGMENT   0x6
#define MI_PBT_PHDR_SEGMENT        0x7

#define MI_PBT_POOL_INDEX          0x1

#define MI_PBT_FLAG_PAGE_MODE_MASK        0x100000
#define MI_PBT_FLAG_ACCESS_TYPE_MASK      0xE00000
#define MI_PBT_FLAG_SEGMENT_TYPE_MASK     0x7000000
#define MI_PBT_FLAG_POOL_INDEX_MASK       0x8000000

#define MI_PBT_FLAG_PAGE_MODE_SHIFT       0x14
#define MI_PBT_FLAG_ACCESS_TYPE_SHIFT     0x15
#define MI_PBT_FLAG_SEGMENT_TYPE_SHIFT    0x18
#define MI_PBT_FLAG_POOL_INDEX_SHIFT      0x1B


#define MI_PBT_FLAGS_MASK                 0x0FF00000
#define MI_PBT_PHDR_FLAGS_SHIFT           0x14


#define MI_PBT_ELF_AMSS_NON_PAGED_RW_SEGMENT \
          (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT)

#define MI_PBT_ELF_AMSS_NON_PAGED_RO_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_RO_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) )

#define MI_PBT_ELF_AMSS_NON_PAGED_ZI_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_ZI_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) )

#define MI_PBT_ELF_AMSS_NON_PAGED_NOTUSED_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_NOTUSED_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) )

#define MI_PBT_ELF_AMSS_NON_PAGED_SHARED_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_SHARED_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) )


#define MI_PBT_ELF_AMSS_PAGED_RW_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_AMSS_PAGED_RO_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_RO_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT)    | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_AMSS_PAGED_ZI_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_ZI_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT)    | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_AMSS_PAGED_NOTUSED_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT)   | \
            (MI_PBT_NOTUSED_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_AMSS_PAGED_SHARED_SEGMENT \
          ( (MI_PBT_AMSS_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT)   | \
            (MI_PBT_SHARED_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )


#define MI_PBT_ELF_HASH_SEGMENT \
          ( (MI_PBT_HASH_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_RO_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) )

#define MI_PBT_ELF_BOOT_SEGMENT \
           ( (MI_PBT_BOOT_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
             (MI_PBT_RO_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT) )

#define MI_PBT_ELF_NON_PAGED_L4BSP_SEGMENT \
          (MI_PBT_L4BSP_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT)

#define MI_PBT_ELF_PAGED_L4BSP_SEGMENT \
           ( (MI_PBT_L4BSP_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
             (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_SWAPPED_PAGED_RO_SEGMENT_INDEX0 \
          ( (MI_PBT_SWAPPED_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_RO_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT)       | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_SWAPPED_PAGED_RO_SEGMENT_INDEX1 \
          ( (MI_PBT_SWAPPED_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_RO_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT)       | \
            (MI_PBT_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT)      | \
            (MI_PBT_POOL_INDEX << MI_PBT_FLAG_POOL_INDEX_SHIFT) )

#define MI_PBT_ELF_SWAP_POOL_NON_PAGED_ZI_SEGMENT_INDEX0 \
          ( (MI_PBT_SWAP_POOL_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_ZI_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT)         | \
            (MI_PBT_NON_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT) )

#define MI_PBT_ELF_SWAP_POOL_NON_PAGED_ZI_SEGMENT_INDEX1 \
          ( (MI_PBT_SWAP_POOL_SEGMENT << MI_PBT_FLAG_SEGMENT_TYPE_SHIFT) | \
            (MI_PBT_ZI_SEGMENT << MI_PBT_FLAG_ACCESS_TYPE_SHIFT)         | \
            (MI_PBT_NON_PAGED_SEGMENT << MI_PBT_FLAG_PAGE_MODE_SHIFT)    | \
            (MI_PBT_POOL_INDEX << MI_PBT_FLAG_POOL_INDEX_SHIFT))


#define MI_PBT_PAGE_MODE_VALUE(x) \
         ( ((x) & MI_PBT_FLAG_PAGE_MODE_MASK) >> \
           MI_PBT_FLAG_PAGE_MODE_SHIFT )

#define MI_PBT_ACCESS_TYPE_VALUE(x) \
         ( ((x) & MI_PBT_FLAG_ACCESS_TYPE_MASK) >> \
           MI_PBT_FLAG_ACCESS_TYPE_SHIFT )

#define MI_PBT_SEGMENT_TYPE_VALUE(x) \
         ( ((x) & MI_PBT_FLAG_SEGMENT_TYPE_MASK) >> \
            MI_PBT_FLAG_SEGMENT_TYPE_SHIFT )

#define MI_PBT_POOL_INDEX_VALUE(x) \
          ( ((x) & MI_PBT_FLAG_POOL_INDEX_MASK) >> \
            MI_PBT_FLAG_POOL_INDEX_SHIFT )


/* Segment memory attributes */
#define MI_PBT_MEM_READ_WRITE 0
#define MI_PBT_MEM_READ_ONLY  1

/* Size of computed HASH values for progressive boot segments */
#define MI_PROG_BOOT_DIGEST_SIZE  20

/* Standard ELF segment type definitions */
#define MI_PBT_ELF_PT_NULL    0
#define MI_PBT_ELF_PT_LOAD    1
#define MI_PBT_ELF_PT_DYNAMIC 2
#define MI_PBT_ELF_PT_INTERP  3
#define MI_PBT_ELF_PT_NOTE    4
#define MI_PBT_ELF_PT_SHLIB   5
#define MI_PBT_ELF_PT_PHDR    6
#define MI_PBT_ELF_TLS        7
