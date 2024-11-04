/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _ELF_H
#define _ELF_H

/* Standard ELF types.  */

#include <stdint.h>

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef    int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef    int32_t  Elf64_Sword;


/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef    int64_t  Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef    int64_t  Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;


/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)
#define ELF32_ST_BIND(info)        ((info) >> 4)
#define ELF32_ST_TYPE(info)        ((info) & 0xf)
typedef struct
{
  unsigned char    e_ident[EI_NIDENT];    /* Magic number and other info */
  Elf32_Half    e_type;                   /* Object file type */
  Elf32_Half    e_machine;                /* Architecture */
  Elf32_Word    e_version;                /* Object file version */
  Elf32_Addr    e_entry;                  /* Entry point virtual address */
  Elf32_Off    e_phoff;                   /* Program header table file offset */
  Elf32_Off    e_shoff;                   /* Section header table file offset */
  Elf32_Word    e_flags;                  /* Processor-specific flags */
  Elf32_Half    e_ehsize;                 /* ELF header size in bytes */
  Elf32_Half    e_phentsize;              /* Program header table entry size */
  Elf32_Half    e_phnum;                  /* Program header table entry count */
  Elf32_Half    e_shentsize;              /* Section header table entry size */
  Elf32_Half    e_shnum;                  /* Section header table entry count */
  Elf32_Half    e_shstrndx;               /* Section header string table index */
} Elf32_Ehdr;

typedef struct
{
  unsigned char    e_ident[EI_NIDENT];    /* Magic number and other info */
  Elf64_Half    e_type;                   /* Object file type */
  Elf64_Half    e_machine;                /* Architecture */
  Elf64_Word    e_version;                /* Object file version */
  Elf64_Addr    e_entry;                  /* Entry point virtual address */
  Elf64_Off    e_phoff;                   /* Program header table file offset */
  Elf64_Off    e_shoff;                   /* Section header table file offset */
  Elf64_Word    e_flags;                  /* Processor-specific flags */
  Elf64_Half    e_ehsize;                 /* ELF header size in bytes */
  Elf64_Half    e_phentsize;              /* Program header table entry size */
  Elf64_Half    e_phnum;                  /* Program header table entry count */
  Elf64_Half    e_shentsize;              /* Section header table entry size */
  Elf64_Half    e_shnum;                  /* Section header table entry count */
  Elf64_Half    e_shstrndx;               /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
  Elf32_Word    sh_name;                  /* Section name (string tbl index) */
  Elf32_Word    sh_type;                  /* Section type */
  Elf32_Word    sh_flags;                 /* Section flags */
  Elf32_Addr    sh_addr;                  /* Section virtual addr at execution */
  Elf32_Off    sh_offset;                 /* Section file offset */
  Elf32_Word    sh_size;                  /* Section size in bytes */
  Elf32_Word    sh_link;                  /* Link to another section */
  Elf32_Word    sh_info;                  /* Additional section information */
  Elf32_Word    sh_addralign;             /* Section alignment */
  Elf32_Word    sh_entsize;               /* Entry size if section holds table */
} Elf32_Shdr;

typedef struct
{
  Elf64_Word    sh_name;                  /* Section name (string tbl index) */
  Elf64_Word    sh_type;                  /* Section type */
  Elf64_Xword    sh_flags;                /* Section flags */
  Elf64_Addr    sh_addr;                  /* Section virtual addr at execution */
  Elf64_Off    sh_offset;                 /* Section file offset */
  Elf64_Xword    sh_size;                 /* Section size in bytes */
  Elf64_Word    sh_link;                  /* Link to another section */
  Elf64_Word    sh_info;                  /* Additional section information */
  Elf64_Xword    sh_addralign;            /* Section alignment */
  Elf64_Xword    sh_entsize;              /* Entry size if section holds table */
} Elf64_Shdr;
#define EI_OSABI             (7)            /* OS ABI identification */
#define ELFOSABI_NONE        (0)            /* UNIX System V ABI */
#define ELFOSABI_SYSV        (0)            /* Alias.  */
#define ELFOSABI_HPUX        (1)            /* HP-UX */
#define ELFOSABI_NETBSD      (2)            /* NetBSD.  */
#define ELFOSABI_GNU         (3)            /* Object uses GNU ELF extensions.  */
#define ELFOSABI_LINUX       (ELFOSABI_GNU) /* Compatibility alias.  */
#define ELFOSABI_SOLARIS     (6)            /* Sun Solaris.  */
#define ELFOSABI_AIX         (7)            /* IBM AIX.  */
#define ELFOSABI_IRIX        (8)            /* SGI Irix.  */
#define ELFOSABI_FREEBSD     (9)            /* FreeBSD.  */
#define ELFOSABI_TRU64       (10)           /* Compaq TRU64 UNIX.  */
#define ELFOSABI_MODESTO     (11)           /* Novell Modesto.  */
#define ELFOSABI_OPENBSD     (12)           /* OpenBSD.  */
#define ELFOSABI_ARM_AEABI   (64)           /* ARM EABI */
#define ELFOSABI_ARM         (97)           /* ARM */
#define ELFOSABI_STANDALONE  (255)          /* Standalone (embedded) application */
#define EM_ARM               (40)           /* ARM */

#define EI_CLASS             (4)            /* File class byte index */
#define ELFCLASSNONE         (0)            /* Invalid class */
#define ELFCLASS32           (1)            /* 32-bit objects */
#define ELFCLASS64           (2)            /* 64-bit objects */
#define ELFCLASSNUM          (3)

#define EI_DATA              (5)            /* Data encoding byte index */
#define ELFDATANONE          (0)            /* Invalid data encoding */
#define ELFDATA2LSB          (1)            /* 2's complement, little endian */
#define ELFDATA2MSB          (2)            /* 2's complement, big endian */
#define ELFDATANUM           (3)

#define ET_NONE       (0)        /* No file type */
#define ET_REL        (1)        /* Relocatable file */
#define ET_EXEC       (2)        /* Executable file */
#define ET_DYN        (3)        /* Shared object file */
#define ET_CORE       (4)        /* Core file */
#define    ET_NUM     (5)        /* Number of defined types */
#define ET_LOOS       (0xfe00)   /* OS-specific range start */
#define ET_HIOS       (0xfeff)   /* OS-specific range end */
#define ET_LOPROC     (0xff00)   /* Processor-specific range start */
#define ET_HIPROC     (0xffff)   /* Processor-specific range end */


#define EM_NONE           (0)    /* No machine */
#define EM_M32            (1)    /* AT&T WE 32100 */
#define EM_SPARC          (2)    /* SUN SPARC */
#define EM_386            (3)    /* Intel 80386 */
#define EM_68K            (4)    /* Motorola m68k family */
#define EM_88K            (5)    /* Motorola m88k family */
#define EM_IAMCU          (6)    /* Intel MCU */
#define EM_860            (7)    /* Intel 80860 */
#define EM_MIPS           (8)    /* MIPS R3000 big-endian */
#define EM_S370           (9)    /* IBM System/370 */
#define EM_MIPS_RS3_LE    (10)   /* MIPS R3000 little-endian */
                /* reserved 11-14 */

#define EM_X86_64     (62)      /* AMD x86-64 architecture */
#define EM_AARCH64    (183)    /* ARM AARCH64 */

#define EF_ARM_RELEXEC            (0x01)
#define EF_ARM_HASENTRY           (0x02)
#define EF_ARM_INTERWORK          (0x04)
#define EF_ARM_APCS_26            (0x08)
#define EF_ARM_APCS_FLOAT         (0x10)
#define EF_ARM_PIC                (0x20)
#define EF_ARM_ALIGN8             (0x40) /* 8-bit structure alignment is in use */
#define EF_ARM_NEW_ABI            (0x80)
#define EF_ARM_OLD_ABI            (0x100)
#define EF_ARM_SOFT_FLOAT         (0x200)
#define EF_ARM_VFP_FLOAT          (0x400)
#define EF_ARM_MAVERICK_FLOAT     (0x800)

#define EF_ARM_EABIMASK    (0XFF000000)
#define SHT_SYMTAB         (2)           /* Symbol table */
#define SHT_DYNSYM         (11)          /* Dynamic linker symbol table */
typedef struct
{
  Elf64_Word    st_name;        /* Symbol name (string tbl index) */
  unsigned char    st_info;     /* Symbol type and binding */
  unsigned char st_other;       /* Symbol visibility */
  Elf64_Section    st_shndx;    /* Section index */
  Elf64_Addr    st_value;       /* Symbol value */
  Elf64_Xword    st_size;       /* Symbol size */
} Elf64_Sym;
typedef struct
{
  Elf32_Word    st_name;        /* Symbol name (string tbl index) */
  Elf32_Addr    st_value;       /* Symbol value */
  Elf32_Word    st_size;        /* Symbol size */
  unsigned char    st_info;     /* Symbol type and binding */
  unsigned char    st_other;    /* Symbol visibility */
  Elf32_Section    st_shndx;    /* Section index */
} Elf32_Sym;

typedef struct
{
  Elf32_Word    p_type;         /* Segment type */
  Elf32_Off    p_offset;        /* Segment file offset */
  Elf32_Addr    p_vaddr;        /* Segment virtual address */
  Elf32_Addr    p_paddr;        /* Segment physical address */
  Elf32_Word    p_filesz;       /* Segment size in file */
  Elf32_Word    p_memsz;        /* Segment size in memory */
  Elf32_Word    p_flags;        /* Segment flags */
  Elf32_Word    p_align;        /* Segment alignment */
} Elf32_Phdr;

#endif /* _ELF_H */
