/*
*/

/*
 * Memory Spaces Definitions.
 */

MEMORY
{

/********************************************************************************************************************************************
*                                                                          RRAM (current) Partitioning.                                     *
*   +------------+----------+---------+---------+----------------+--------------+--------------+------------+------+----------------------+ *
*   | PBL Memory |    FDT   |  SBLA   |  SBLB   |   File System  |   CAL DATA   |   BDF DATA   |   REG DATA | INI  |  System Image        | *
*   |  16 KB     |   1 KB   |  16 KB  |  16 KB  |      32 KB     |     12 KB    |     24 KB    |     16 KB  | 8KB  |   1395 KB            | *
*   +------------+----------+---------+---------+----------------+--------------+--------------+------------+------+----------------------+ *
*                                                                                                                                           *
*********************************************************************************************************************************************/

  OTPMEM (rw) : ORIGIN = 0x001A0000, LENGTH = 0x00001000
  PBLMEM (xrw) : ORIGIN = 0x00200000, LENGTH = 0x00004000
  FDTMEM (rw) : ORIGIN = 0x00204000, LENGTH = 0x00000400
  SBLAMEM (xrw) : ORIGIN = 0x00204400, LENGTH = 0x00004000
  SBLBMEM (xrw) : ORIGIN = 0x00208400, LENGTH = 0x00004000
  FSMEM (rw) : ORIGIN = 0x0020C400, LENGTH = 0x00008000
  CALMEM (rw) : ORIGIN = 0x00214400, LENGTH = 0x00003000
  BDFMEM (r) : ORIGIN = 0x00217400, LENGTH = 0x00006000
  REGDBMEM (r) : ORIGIN = 0x0021D400, LENGTH = 0x00004000
  INIMEM (r) : ORIGIN = 0x00221400, LENGTH = 0x00002000
  RRAM_FLASH (xrw) : ORIGIN = 0x00223400, LENGTH = 0x0015CC00
  XIP_FLASH (xr) : ORIGIN = 0x03000000, LENGTH = 0x00100000

  RAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0x000A0000
}

/*Ram Bank address and their size*/
_ln_cMEM_Bank_size__ = 0x00020000;
_ln_cMEM_Bank_A_start_addr__ = ORIGIN(RAM);
_ln_cMEM_Bank_B_start_addr__ = _ln_cMEM_Bank_A_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_C_start_addr__ = _ln_cMEM_Bank_B_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_D_start_addr__ = _ln_cMEM_Bank_C_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_E_start_addr__ = _ln_cMEM_Bank_D_start_addr__ + _ln_cMEM_Bank_size__;

/****************************************************************************************************************************************************

Note:
+---------------------------------------------------------------------------------------------------------------+
|   * PS txt        |   Power Save code                                                                         |
|                   |   (includes SOCPM code)                                                                   |
|---------------------------------------------------------------------------------------------------------------|
|   * PS data       |   Power Save data                                                                         |
|                   |   (includes RRI table, HW descriptor, Packet Memory(beacon))                              |
|---------------------------------------------------------------------------------------------------------------|
|   * App txt       |   Customer RAM resident applications code                                                 |
|                   |   (includes Ping, TCP utility, UDP utility, WiFi App)                                     |
|---------------------------------------------------------------------------------------------------------------|
|   * App data      |   Customer RAM resident applications data                                                 |
|---------------------------------------------------------------------------------------------------------------|
|   * Perf txt      |   Performance code                                                                        |
|                   |   (includes FreeRTOS kernel, LWIP, Data Path, Nano lib)                                   |
|---------------------------------------------------------------------------------------------------------------|
|   * Perf data     |   Performance data                                                                        |
|---------------------------------------------------------------------------------------------------------------|
|   * Sleep data    |   Includes BSS, Logs, Statistics, Heap contents during the power save                     |
|   Retention Region|                                                                                           |
+---------------------------------------------------------------------------------------------------------------+

RRAM Starting Address   - 0x0020 0000
RAM A Starting Address  - 0x0000 0000
RAM B Starting Address  - 0x0002 0000
RAM C Starting Address  - 0x0004 0000
RAM D Starting Address  - 0x0006 0000
RAM E Starting Address  - 0x0008 0000
                                      RRAM            RAM A, B, C, D & E
                                +---------------+    +---------------+
                                |     PBL       |    |   ram ISR     |
                                +---------------+    |    vector     |
                                |     FDT       |    +---------------+
                                +---------------+    |   PS txt      |
                                |    SBL_A      |    +---------------+
                                +---------------+    |   PS data     |
                                |    SBL_B      |    +---------------+
                                +---------------+    | System Stack  |
                                |  File System  |    +---------------+
                                +---------------+    |   Sleep Data  |
                                |   CAL Data    |    |   retention   |
                                +---------------+    |    region     |
                                |     BDF       |    +---------------+
                                +---------------+    |   Perf txt    |
                                |  Regulatory   |    +---------------+
                                |     Data      |    |   Perf data   |
                                +---------------+    +---------------+
                                |    INI        |    |     App       |
                                +---------------+    |     txt       |
                                |   Flash ISR   |    |               |
                                |      vector   |    +---------------+
                                +---------------+    |   App data    |
                                |   PS txt      |    +---------------+
                                +---------------+    |     Packet    |
                                |   PS data     |    |     Memory    |
                                +---------------+    +---------------+
                                |   Perf txt    |    |    HW DS &    |
                                +---------------+    |   Descriptors |
                                |   Perf data   |    +---------------+
                                +---------------+    |      Heap     |
                                |   App txt     |    +---------------+
                                +---------------+
                                |   App data    |
                                +---------------+
                                |   WLAN Ctrl & |
                                |    System     |
                                |     text      |
                                +---------------+
                                |   WLAN Ctrl & |
                                |    System     |
                                |     data      |
                                +---------------+

linker symbol to address space interpretation
--------------------------------------------
_ln_RF_*    - represents address in RRAM_FLASH
_ln_RAM_*   - represents address in RAM

****************************************************************************************************************************************************/

 /* Mount Address for the File System */
_ln_FS_Start_Addr = ORIGIN(FSMEM) ;

/* FDT area */
_ln_FDT_Start_Addr = ORIGIN(FDTMEM) ;
_ln_FDT_Data_length = LENGTH(FDTMEM) ;

/* CAL data of the system */
_ln_CAL_Start_Addr = ORIGIN(CALMEM) ;
_ln_CAL_Data_length = LENGTH(CALMEM) ;

/* Board Data File */
_ln_BDF_Start_Addr = ORIGIN(BDFMEM) ;
_ln_BDF_Data_length = LENGTH(BDFMEM) ;

_ln_REGDB_Start_Addr = ORIGIN(REGDBMEM) ;
_ln_REGDB_Data_length = LENGTH(REGDBMEM) ;

/* Wi-Fi Config INI file */
_ln_INI_Start_Addr = ORIGIN(INIMEM) ;
_ln_INI_Data_length = LENGTH(INIMEM) ;

/* RRAM Main Region */
__rram_region_start_addr = ORIGIN(PBLMEM) ;
__rram_region_end_address = ORIGIN(PBLMEM) + LENGTH(PBLMEM) + LENGTH(FDTMEM) + LENGTH(SBLAMEM) + LENGTH(SBLBMEM)  + LENGTH(FSMEM) + LENGTH(CALMEM) + LENGTH(BDFMEM) + LENGTH(REGDBMEM) + LENGTH(INIMEM) + LENGTH(RRAM_FLASH);


/*Fermion OTP region address start*/
 __OTP_region_st_addr = ORIGIN(OTPMEM);
 __OTP_region_end_addr = ORIGIN(OTPMEM) + LENGTH(OTPMEM);

/*
 * The '__stack' definition is required by crt0, do not remove it.
 */
__stack = _ln_RAM_stack_start_addr ;
_estack = __stack; /* End of System Stack */

/*
 * Default stack sizes.
 * These are used by the startup in order to allocate stacks
 * for the different modes.
 */

__Main_Stack_Size = 3072 ;        /* 3 KB system stack */


PROVIDE ( _Main_Stack_Size = __Main_Stack_Size ) ;

__Main_Stack_Limit = __stack  - __Main_Stack_Size ;

/* "PROVIDE" allows to easily override these values from an
 * object file or the command line. */
PROVIDE ( _Main_Stack_Limit = __Main_Stack_Limit ) ;

/*
 * Default heap definitions, Required by _sbrk. Heap size set to 100
 */
PROVIDE ( _Heap_Begin = _noinit ) ;
PROVIDE ( _Heap_Limit = _end_noinit ) ;
/*
 * The entry point is informative, for debuggers and simulators,
 * since the Cortex-M vector points to it anyway.
 */
ENTRY(_start)


/* Sections Definitions */

SECTIONS
{

/* ------------------------------------- ISR Vector ---------------------------------------------*/
    /*
     * For Cortex-M devices, the beginning of the startup code is stored in
     * the .flash_isr_vector section, which goes to RRAM(FLASH).
     */
    .flash_isr_vector : ALIGN(4)
    {
        FILL(0xFF)
        _ln_RF_flash_isr_vector_start__ = . ;

       __vectors_start = ABSOLUTE(.) ;
        KEEP(*(.flash_isr_vector))         /* Interrupt vectors */

        . = ALIGN(4) ;

    } >RRAM_FLASH

    _ln_isr_vector_size__ = SIZEOF(.flash_isr_vector) ;


/* ---------------------------- Power Save/Minimal code - text section ------------------------- */

    _ln_RF_start_addr_ps_txt__ = LOADADDR(.__sect_ps_txt) ;
    _ln_RAM_start_addr_ps_txt__ = ADDR(.__sect_ps_txt);

    .__sect_ps_txt : ALIGN(4)
    {
        KEEP(*(.ram_isr_vector))            /* vector table with ram_minimum_code as reset handler */

        _ln_RAM_ferm_table_addr = ABSOLUTE(.);
        . += 4;
        _ln_RAM_ferm_multiuse_gpio_assert_info_addr = ABSOLUTE(.);
        . += 4;

    /* NOTE:
     * Previously, the warm boot/minimum code was part of the section
     * after_ram_vectors. It has been moved to the new section ram_minimum_entry
     * and placed just after ram_isr_vector to ensure that it is located in
     * appropriate CMEM bank/sub-bank to avoid power inrush issues on warm boot.
     * To ensure that it stays there,
     *  - no other functions must be added to the ram_minimum_entry section
     *  - no section must be placed in between ram_isr_vector and ram_minimum_entry
     */

        *(.ram_minimum_entry .ram_minimum_entry.*)    /* warm boot code */
        *(.after_ram_vectors .after_ram_vectors.*)    /* ISR */

        *nt_socpm_sleep.o(.text .text.*)
        *nt_minimum_code.o(.text .text.*)
        *nt_mini_beacon_process.o(.text .text.*)
        *printf*.o(.text .text.*)
        *divmod*.o(.text .text.*)
        *nt_socpm_sleep.o(.rodata .rodata.*)
        *nt_minimum_code.o(.rodata .rodata.*)
        *nt_mini_beacon_process.o(.rodata .rodata.*)
        *printf*.o(.rodata .rodata.*)
        *divmod*.o(.rodata .rodata.*)
        *(.__sect_ps_txt)

        . = ALIGN(4);
    } >RAM AT> RRAM_FLASH

    _ln_ps_txt_size__ = SIZEOF(.__sect_ps_txt) ;


/* ----------------------------- Power Save/Minimal code - data section ------------------------ */

    _ln_RF_start_addr_ps_data__ = LOADADDR(.__sect_ps_data) ;
    _ln_RAM_start_addr_ps_data__ =  ADDR(.__sect_ps_data);

    .__sect_ps_data : ALIGN(4)
    {

        *nt_socpm_sleep.o(.data .data.*)
        *nt_minimum_code.o(.data .data.*)
        *nt_mini_beacon_process.o(.data .data.*)
        *printf*.o(.data .data.*)
        *divmod*.o(.data .data.*)
        *(.__sect_ps_data)                 /* includes RRI table, HW descriptors, Packet Memory(beacon), Statistics, Logs */

        . = ALIGN(4) ;
    } >RAM AT> RRAM_FLASH

    _ln_ps_data_size__ = SIZEOF(.__sect_ps_data) ;

    ASSERT( (28K > (_ln_ps_data_size__)), "power save data exceeds the limit")

/* --------------------------------------- System Stack --------------------------------------- */

    .system_stack (NOLOAD) : ALIGN(4)
    {
        . += __Main_Stack_Size ;        /* 3KB Memory reserved */
        _ln_RAM_stack_start_addr = ABSOLUTE(.) ;
    } >RAM

    _ln_system_stack_size__ = SIZEOF(.system_stack);

/* ---------------------------- Sleep Data Retention section --------------------------------- */

    /* The primary uninitialised data section. */
    .__sect_sleep_data_retention (NOLOAD) : ALIGN(4)
    {
        _ln_bss_start__ = .;         /* standard newlib definition */

        *(.bss_begin .bss_begin.*)

        *(.bss .bss.*)
        *(COMMON)

        *(.bss_end .bss_end.*)
        . = ALIGN(4);
        _ln_bss_end__ = .;        /* standard newlib definition */

    } >RAM

    _ln_sect_sleep_data_retention_size__ = SIZEOF(.__sect_sleep_data_retention) ;

/*    ASSERT( (20K > (_ln_bss_size__)), "bss exceeds the limit")
The above ASSERT is temporarily disabled for the performance measurements, as it is known the BSS size will exceed 20K    */

/* ---------------------------------- Performance code - Text Section -------------------------------- */

    _ln_RF_start_addr_perf_txt__ = LOADADDR(.__sect_perf_txt) ;
    _ln_RAM_start_addr_perf_txt__ = ADDR(.__sect_perf_txt);

    .__sect_perf_txt : ALIGN(4)
    {

        *os/freertos/freertos_kernel\/*.o(.text .text.*)
        *os/freertos/libraries/3rdparty/lwip/src/api\/*.o(.text .text.*)
        *core/system/sys_src\/timer.o(.text .text.*)
        *core/system/sys_src\/qtmr.o(.text .text.*)

        EXCLUDE_FILE(*ipv4/icmp.o *ipv4/igmp.o *ipv4/dhcp.o *ipv6/icmp6.o *ipv6/nd6.o *ipv6/mld6.o *ipv6/dhcp6.o *dns.o) os/freertos/libraries/3rdparty/lwip/src/core\/*.o(.text .text.*)

        *os/freertos/libraries/3rdparty/lwip/src/netif\/*.o(.text .text.*)
        *os/freertos/libraries/3rdparty/lwip/src/portable\/*.o(.text .text.*)
        EXCLUDE_FILE(*mlme_al.o *datapath_stats.o *test_dpm.o) core/wifi/dpm\/*.o(.text .text.*)

        EXCLUDE_FILE(*pkcs5.o *crypto_mbedtls.o *arc4.o *cipher_wrap.o *error.o *pkcs12.o *ssl_ciphersuites.o *ssl_cli.o *ssl_srv.o *ssl_tls.o *version_features.o \
        *nist_kw.o *md_wrap.o *md.o *md5.o *oid.o *pem.o *pkcs5.o *pkparse.o \
        *certs.o *hmac_drbg.o *rsa.o *sha1.o *ssl_cookie.o *x509.o *x509write_crt.o \
        *ecp.o *entropy.o *pk.o *ecdh.o *aes.o *ctr_drbg.o *bignum.o *cmac.o *des.o \
        *ccm.o *pk.o *debug.o *asn1parse.o *asn1write.o *x509_crt.o *sha256.o *sha512.o) os/freertos/libraries/3rdparty/mbedtls/library\/*.o(.text .text.*)

        *libg_nano.a:*(.text .text.*)

        *os/freertos/freertos_kernel\/*.o(.rodata .rodata.* .constdata .constdata.*)
        *core/system/sys_src\/timer.o(.rodata .rodata.* .constdata .constdata.*)
        *core/system/sys_src\/qtmr.o(.rodata .rodata.* .constdata .constdata.*)

        *os/freertos/libraries/3rdparty/lwip/src/api\/*.o(.rodata .rodata.* .constdata .constdata.*)
        EXCLUDE_FILE(*ipv4/icmp.o *ipv4/igmp.o *ipv4/dhcp.o *ipv6/icmp6.o *ipv6/nd6.o *ipv6/mld6.o *ipv6/dhcp6.o *dns.o) os/freertos/libraries/3rdparty/lwip/src/core\/*.o(.rodata .rodata.* .constdata .constdata.*)
        *os/freertos/libraries/3rdparty/lwip/src/netif\/*.o(.rodata .rodata.* .constdata .constdata.*)
        *os/freertos/libraries/3rdparty/lwip/src/portable\/*.o(.rodata .rodata.* .constdata .constdata.*)
        EXCLUDE_FILE(*mlme_al.o *datapath_stats.o *test_dpm.o) core/wifi/dpm\/*.o(.rodata .rodata.* .constdata .constdata.*)

        EXCLUDE_FILE(*pkcs5.o *crypto_mbedtls.o *arc4.o *cipher_wrap.o *error.o *pkcs12.o *ssl_ciphersuites.o *ssl_cli.o *ssl_srv.o *ssl_tls.o *version_features.o \
        *nist_kw.o *md_wrap.o *md.o *md5.o *oid.o *pem.o *pkcs5.o *pkparse.o \
        *certs.o *hmac_drbg.o *rsa.o *sha1.o *ssl_cookie.o *x509.o *x509write_crt.o \
        *ecp.o *entropy.o *pk.o *ecdh.o *aes.o *ctr_drbg.o *bignum.o *cmac.o *des.o \
        *ccm.o *pk.o *debug.o *asn1parse.o *asn1write.o *x509_crt.o *sha256.o *sha512.o) os/freertos/libraries/3rdparty/mbedtls/library\/*.o(.rodata .rodata.* .constdata .constdata.*)

        *libg_nano.a:*(.rodata .rodata.* .constdata .constdata.*)

        . = ALIGN(4);
    } >RAM AT> RRAM_FLASH

    _ln_perf_txt_size__ = SIZEOF(.__sect_perf_txt) ;

/* ------------------------------- Performance code - Data Section ------------------------------------ */

    _ln_RF_start_addr_perf_data__ = LOADADDR(.__sect_perf_data) ;
    _ln_RAM_start_addr_perf_data__ = ADDR(.__sect_perf_data) ;

    .__sect_perf_data : ALIGN(4)
    {
        *os/freertos/freertos_kernel\/*.o(.data .data.*)
        *core/system/sys_src\/timer.o(.data .data.*)
        *core/system/sys_src\/qtmr.o(.data .data.*)

        *os/freertos/libraries/3rdparty/lwip/src/api\/*.o(.data .data.*)
        EXCLUDE_FILE(*ipv4/icmp.o *ipv4/igmp.o *ipv4/dhcp.o *ipv6/icmp6.o *ipv6/nd6.o *ipv6/mld6.o *ipv6/dhcp6.o *dns.o) os/freertos/libraries/3rdparty/lwip/src/core\/*.o(.data .data.*)
        *os/freertos/libraries/3rdparty/lwip/src/netif\/*.o(.data .data.*)
        *os/freertos/libraries/3rdparty/lwip/src/portable\/*.o(.data .data.*)
        EXCLUDE_FILE(*mlme_al.o *datapath_stats.o *test_dpm.o) core/wifi/dpm\/*.o(.data .data.*)
        EXCLUDE_FILE(*pkcs5.o *crypto_mbedtls.o *arc4.o *cipher_wrap.o *error.o *pkcs12.o *ssl_ciphersuites.o *ssl_cli.o *ssl_srv.o *ssl_tls.o *version_features.o \
        *nist_kw.o *md_wrap.o *md.o *md5.o *oid.o *pem.o *pkcs5.o *pkparse.o \
        *certs.o *hmac_drbg.o *rsa.o *sha1.o *ssl_cookie.o *x509.o *x509write_crt.o \
        *ecp.o *entropy.o *pk.o *ecdh.o *aes.o *ctr_drbg.o *bignum.o *cmac.o *des.o \
        *ccm.o *pk.o *debug.o *asn1parse.o *asn1write.o *x509_crt.o *sha256.o *sha512.o) os/freertos/libraries/3rdparty/mbedtls/library\/*.o(.data .data.*)
        *libg_nano.a:*(.data .data.*)

        . = ALIGN(4);
    } >RAM AT> RRAM_FLASH

    _ln_perf_data_size__ = SIZEOF(.__sect_perf_data) ;

/* ---------------------------------- RAM resident Apps text section --------------------------- */

    _ln_RF_start_addr_app_txt__ = LOADADDR(.__sect_app_txt) ;
    _ln_RAM_start_addr_app_txt__ = ADDR(.__sect_app_txt) ;

    .__sect_app_txt : ALIGN(4)
    {
        EXCLUDE_FILE(*nt_dhcps.o *nt_multicast.o *httpd.o) os/freertos/libraries/3rdparty/lwip/src/apps\/*.o(.text .text.*)
        *lwiperf.o(.text .text.*)
        *udp_perf_raw_client.o(.text .text.*)

          EXCLUDE_FILE(*nt_dhcps.o *nt_multicast.o *httpd.o) os/freertos/libraries/3rdparty/lwip/src/apps\/*.o(.rodata .rodata.* .constdata .constdata.*)
        *lwiperf.o(.rodata .rodata.* .constdata .constdata.*)
        *udp_perf_raw_client.o(.rodata .rodata.* .constdata .constdata.*)
        . = ALIGN(4) ;

    } >RAM AT>RRAM_FLASH

    _ln_app_txt_size__ = SIZEOF(.__sect_app_txt) ;

/* ----------------------------------- RAM resident Apps data section --------------------------- */

    _ln_RF_start_addr_app_data__ = LOADADDR(.__sect_app_data) ;
    _ln_RAM_start_addr_app_data__ = ADDR(.__sect_app_data) ;

    .__sect_app_data : ALIGN(4)
    {

        EXCLUDE_FILE(*nt_dhcps.o *nt_multicast.o *httpd.o) os/freertos/libraries/3rdparty/lwip/src/apps\/*.o(.data .data.*)
        *lwiperf.o(.data .data.*)
        *udp_perf_raw_client.o(.data .data.*)
        *apps/WiFi_App\/*.o(.data .data.*)


        . = ALIGN(4) ;

    } >RAM AT> RRAM_FLASH


    _ln_app_data_size__ = SIZEOF(.__sect_app_data) ;

/* ------------------------- XIP FLASH Text Section -------------------------------- */
    .__sect_xip_flash_txt : ALIGN(4)
    {
        _ln_xip_flash_text_start = ABSOLUTE(.);

        *apps/xip_flash_demo\/*.o(.text .text.*)

        *apps/xip_flash_demo\/*.o(.rodata .rodata.*)
        *apps/xip_flash_demo\/*.o(.constdata .constdata.*)


        . = ALIGN(4);
        _ln_xip_flash_text_end = . ;
    } >XIP_FLASH

/* ---------------------------- XIP FLASH Data Section ----------------------------- */
     _ln_RF_start_addr_xip_flash_data__ = LOADADDR(.xip_flash_ram_data);
     _ln_RAM_start_addr_xip_flash_data__ = ADDR(.xip_flash_ram_data);
    .xip_flash_ram_data : ALIGN(4)
    {
	    *apps/xip_flash_demo\/*.o(.data .data.*)
	    . = ALIGN(4);

    } >RAM AT> XIP_FLASH
     _ln_xip_flash_data_size__ = SIZEOF(.xip_flash_ram_data);

/* ------------------------- WLAN Control & System Text Section -------------------------------- */

    .text : ALIGN(4)
    {
        *(.after_vectors .after_vectors.*)    /* Startup code and ISR */

/* Warning : The text, rodata and constdata sections of the excluded files are placed into different sections intentionally.
* Changing them will have effect on the performance and system operation */

        *(EXCLUDE_FILE(*os/freertos/libraries/3rdparty/lwip\/*.o *core/wifi/dpm\/*.o *os/freertos/freertos_kernel\/*.o *neutrino_pbl/src\/*.o *neutrino_pbl/common\/*.o *neutrino_pbl/system\/*.o) .text .text.*)            /* all remaining code */
        *(EXCLUDE_FILE(*ping.o *libg_nano.a:*) .text .text.*)

         /* read-only data (constants) */
        *(EXCLUDE_FILE(*os/freertos/libraries/3rdparty/lwip\/*.o *core/wifi/dpm\/*.o *os/freertos/freertos_kernel\/*.o *neutrino_pbl/src\/*.o *neutrino_pbl/common\/*.o *neutrino_pbl/system\/*.o) .rodata .rodata.* .constdata .constdata.*)
        *(EXCLUDE_FILE(*ping.o *libg_nano.a:*) .rodata .rodata.* .constdata .constdata.*)
        *(EXCLUDE_FILE(*apps/xip_flash_demo\/*.o) .text .text.*)

        *(EXCLUDE_FILE(*apps/xip_flash_demo\/*.o) .rodata .rodata.* .constdata .constdata.*)


        *(vtable)                    /* C++ virtual tables */

        KEEP(*(.eh_frame*))

        /*
         * Stub sections generated by the linker, to glue together
         * ARM and Thumb code. .glue_7 is used for ARM code calling
         * Thumb code, and .glue_7t is used for Thumb code calling
         * ARM code. Apparently always generated by the linker, for some
         * architectures, so better leave them here.
         */
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
        _ln_text_end__ = . ;

    } >RRAM_FLASH

/* ---------------------------- WLAN Control & System Data Section ----------------------------- */
       .data : ALIGN(4)
       {
            *(.data_begin .data_begin.*)
            *startup.o(.data .data.*)        /* Boot related read-only initialized data */
            *(.data_end .data_end.*)
       } >RRAM_FLASH


     _ln_RF_start_addr_data__ = LOADADDR(.ram_data);
     _ln_RAM_start_addr_data__ = ADDR(.ram_data);
    .ram_data : ALIGN(4)
    {
        *(.data_begin .data_begin.*)

/* Warning : The data sections of the excluded files are placed into different sections intentionally.
* Changing them will have effect on the performance and system operation */

        *(EXCLUDE_FILE(*os/freertos/libraries/3rdparty/lwip\/*.o *core/wifi/dpm\/*.o *os/freertos/freertos_kernel\/*.o) .data .data.*)
        *(EXCLUDE_FILE(*ping.o *apps/WiFi_App\/*.o *libg_nano.a:*) .data .data.*)
        *(EXCLUDE_FILE(*apps/xip_flash_demo\/*.o).data .data.*)
        *(.data_end .data_end.*)
        . = ALIGN(4);

    } >RAM AT> RRAM_FLASH
     _ln_data_size__ = SIZEOF(.ram_data);

/* ----------------------------- Hardware Packet Memory ------------------------------------- */
/* Warning : Linker script reserves the memory for HW Packet Memory.
* The address is configured in hal_int_mmap.h file. */
    .__sect_hw_pktmem (NOLOAD) : ALIGN(0x80)
    {
        _ln_RAM_start_addr_hw_pktmem__ = ABSOLUTE(.) ;
        . += 32768 ;
       _ln_RAM_end_addr_hw_pktmem__ = ABSOLUTE(.) ;
    } >RAM

/* --------------------------- Hardware Descriptors/Data structures --------------------------- */
/* Warning : Linker script reserves the memory for HW Descriptors.
* The address is configured in hal_int_mmap.h file. */
    .__sect_hw_desc (NOLOAD) : ALIGN(4)
    {
       _ln_RAM_start_addr_hw_desc__ = ABSOLUTE(.) ;
 /* DPU descriptors + keys + mickeys + replay counters + TPE descriptors + stats + RPE descriptors + BTQM + SW templates +
 * CTS to self SW templates + PS poll SW templates + QOS NULL SW templates + Data NULL SW templates + TEMP SW templates + Beacon template
 + RXP address table + RRI region */
      . += 22504 ;
      _ln_RAM_end_addr_hw_desc__ = ABSOLUTE(.) ;
    } >RAM

/*  Default heap definitions required by _sbrk. Heap size set to 100 */

    .noinit (NOLOAD) : ALIGN(4)
    {
        _noinit = ABSOLUTE(.);

        *(.noinit .noinit.*)
        . += 100 ;

         . = ALIGN(4) ;
        _end_noinit = .;
    } > RAM
 /*   .__sect_qspi_mem_addr (NOLOAD) : ALIGN(4)
    {
        _ln_RAM_qspi_slv_mem_addr = ABSOLUTE(.);
        . += 4;
    } >RAM
*/
/* ----------------------------------------- Heap Section ------------------------------------- */
    .heap (NOLOAD) : ALIGN(4)
    {
        _ln_RAM_addr_heap_start__ = ABSOLUTE(.) ;
        *(.heap)
        . = ALIGN(4) ;
       _ln_RAM_addr_heap_end__ = ABSOLUTE(.) ;
    } >RAM

    ASSERT( ((LENGTH(RAM) - _ln_RAM_addr_heap_start__) > 170K), "ERROR : Heap is less than 170 KB")

    /* Mandatory to be word aligned, _sbrk assumes this */
    PROVIDE ( end = _end_noinit ); /* was _ebss */
    PROVIDE ( _end = _end_noinit );
    PROVIDE ( __end = _end_noinit );
    PROVIDE ( __end__ = _end_noinit );

    /* After that there are only debugging sections. */

    /* This can remove the debugging information from the standard libraries */


    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /*
     * DWARF debug sections.
     * Symbols in the DWARF debugging sections are relative to the beginning
     * of the section so we begin them at 0.
     */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
    .image_version 0 (INFO): { KEEP(*(.wlan_version_num)) }
     debug_strings 0x7000000 (INFO): { KEEP(*(.DBG_STRING)) }
}
